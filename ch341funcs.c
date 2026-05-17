//
// ch341eeprom programmer
//
//  Programming tool for the 24Cxx serial EEPROMs using the Winchiphead CH341A IC
// (c) December 2011 asbokid <ballymunboy@gmail.com>, extended by Precise

//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ch341eeprom.h"

extern FILE *debugout, *verbout;
uint32_t getnextpkt = 0;                            // set by the callback function
uint32_t syncackpkt = 0;                            // synch / ack flag used by BULK OUT cb function
uint32_t byteoffset = 0;
extern uint8_t *readbuf;

// --------------------------------------------------------------------------
struct libusb_device_handle *ch341configure(uint16_t vid, uint16_t pid) {
    struct libusb_device *dev;
    struct libusb_device_handle *devHandle;
    int32_t ret = 0;                    // set to < 0 to indicate USB errors
    uint32_t i = 0;
    int32_t currentConfig = 0;
    uint8_t desc[0x12];

    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Couldn't initialise libusb\n");
        return NULL;
    }

#if LIBUSB_API_VERSION >= 0x01000106
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, 3);
#else
    libusb_set_debug(NULL, 3);                  // maximum debug logging level
#endif

    fprintf(verbout, "Searching for WCH CH341a [%04x:%04x]\n", vid, pid);
    devHandle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (!devHandle) {
        fprintf(stderr, "Device CH341a [%04x:%04x] not found\n", vid, pid);
        return NULL;
    }

    dev = libusb_get_device(devHandle);
    fprintf(verbout, "Found device on bus %d address %d\n",
            libusb_get_bus_number(dev), libusb_get_device_address(dev));

    if (libusb_kernel_driver_active(devHandle, 0)) {
        ret = libusb_detach_kernel_driver(devHandle, 0);
        if (ret) fprintf(stderr, "Failed to detach kernel driver\n");
        else fprintf(verbout, "Detached kernel driver\n");
    }

    ret = libusb_get_configuration(devHandle, &currentConfig);
    if (ret == 0 && currentConfig != 1)
        libusb_set_configuration(devHandle, 1);

    ret = libusb_claim_interface(devHandle, 0);
    if (ret) {
        fprintf(stderr, "Failed to claim interface\n");
        return NULL;
    }

    ret = libusb_get_descriptor(devHandle, LIBUSB_DT_DEVICE, 0, desc, sizeof(desc));
    if (ret > 0)
        fprintf(verbout, "CH341 reported revision %d.%02d\n", desc[12], desc[13]);

    return devHandle;
}

// --------------------------------------------------------------------------
int32_t ch341setstream(struct libusb_device_handle *devHandle, uint32_t speed) {
    uint8_t buf[3] = { mCH341A_CMD_I2C_STREAM,
                       (uint8_t)(mCH341A_CMD_I2C_STM_SET | (speed & 3)),
                       mCH341A_CMD_I2C_STM_END };
    int transferred;
    int ret = libusb_bulk_transfer(devHandle, BULK_WRITE_ENDPOINT, buf, 3, &transferred, DEFAULT_TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "Failed to set I2C speed: %s\n", strerror(-ret));
        return -1;
    }
    return 0;
}

// --------------------------------------------------------------------------
// Read EEPROM (original logic, but with percentage progress)
int32_t ch341readEEPROM(struct libusb_device_handle *devHandle, uint8_t *buffer, uint32_t bytestoread) {
    uint8_t outBuf[EEPROM_READ_BULKOUT_BUF_SZ];
    uint8_t inBuf[IN_BUF_SZ];
    int32_t ret, readpktcount = 0;
    struct libusb_transfer *xferIn, *xferOut;
    struct timeval tv = {0, 100};                   // our async polling intervals

    byteoffset = 0;
    readbuf = buffer;

    xferIn = libusb_alloc_transfer(0);
    xferOut = libusb_alloc_transfer(0);
    if (!xferIn || !xferOut) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    memset(inBuf, 0, EEPROM_READ_BULKIN_BUF_SZ);
    memcpy(outBuf, CH341_EEPROM_READ_SETUP_CMD, EEPROM_READ_BULKOUT_BUF_SZ);

    libusb_fill_bulk_transfer(xferIn, devHandle, BULK_READ_ENDPOINT, inBuf,
                              EEPROM_READ_BULKIN_BUF_SZ, cbBulkIn, NULL, DEFAULT_TIMEOUT);
    libusb_fill_bulk_transfer(xferOut, devHandle, BULK_WRITE_ENDPOINT, outBuf,
                              EEPROM_READ_BULKOUT_BUF_SZ, cbBulkOut, NULL, DEFAULT_TIMEOUT);

    libusb_submit_transfer(xferIn);
    libusb_submit_transfer(xferOut);

    while (byteoffset < bytestoread) {
        fprintf(stdout, "Read %d%% [%d/%d] bytes   \r",
                (int)(100 * byteoffset / bytestoread), byteoffset, bytestoread);
        ret = libusb_handle_events_timeout(NULL, &tv);
        if (ret < 0 || getnextpkt == -1) {
            fprintf(stderr, "USB error\n");
            goto err;
        }
        if (getnextpkt == 1) {                      // callback function reports a new BULK IN packet received
            getnextpkt = 0;                         //   reset the flag
            readpktcount++;                         //   increment the read packet counter
            byteoffset += EEPROM_READ_BULKIN_BUF_SZ;
            libusb_submit_transfer(xferIn);
            if (readpktcount == 4 && byteoffset < bytestoread) {
                readpktcount = 0;
                memcpy(outBuf, CH341_EEPROM_READ_NEXT_CMD, CH341_EEPROM_READ_CMD_SZ);
                outBuf[4] = (byteoffset >> 8) & 0xFF;
                outBuf[5] = byteoffset & 0xFF;
                libusb_fill_bulk_transfer(xferOut, devHandle, BULK_WRITE_ENDPOINT, outBuf,
                                          EEPROM_READ_BULKOUT_BUF_SZ, cbBulkOut, NULL, DEFAULT_TIMEOUT);
                libusb_submit_transfer(xferOut);
            }
        }
    }
    fprintf(stdout, "Read 100%% [%d/%d] bytes   \n", byteoffset, bytestoread);
    libusb_free_transfer(xferIn);
    libusb_free_transfer(xferOut);
    return 0;
err:
    libusb_free_transfer(xferIn);
    libusb_free_transfer(xferOut);
    return -1;
}

// --------------------------------------------------------------------------
void cbBulkIn(struct libusb_transfer *transfer) {
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        memcpy(readbuf + byteoffset, transfer->buffer, transfer->actual_length);
        getnextpkt = 1;
    } else {
        getnextpkt = -1;
    }
}

void cbBulkOut(struct libusb_transfer *transfer) {
    syncackpkt = 1;
}

// --------------------------------------------------------------------------
// Write EEPROM with page size awareness
int32_t ch341writeEEPROM(struct libusb_device_handle *devHandle, uint8_t *buffer, uint32_t bytesum) {
    uint8_t outBuf[256];
    uint8_t cmd[32];
    uint32_t offset = 0, remaining = bytesum;
    int ret;

    while (remaining > 0) {
        uint16_t page = 32;  // fixed page size for 24c32/64
        uint16_t chunk = (remaining < page) ? remaining : page;
        uint8_t *ptr = cmd;

        // Build I2C write command
        *ptr++ = mCH341A_CMD_I2C_STREAM;
        *ptr++ = mCH341A_CMD_I2C_STM_STA;
        *ptr++ = mCH341A_CMD_I2C_STM_OUT | (2 + chunk); // device address + 2 addr bytes + data
        *ptr++ = 0xA0;  // EEPROM device address
        *ptr++ = (offset >> 8) & 0xFF;
        *ptr++ = offset & 0xFF;
        memcpy(ptr, buffer + offset, chunk);
        ptr += chunk;
        *ptr++ = mCH341A_CMD_I2C_STM_STO;
        *ptr++ = mCH341A_CMD_I2C_STM_END;

        // Send packet
        ret = libusb_bulk_transfer(devHandle, BULK_WRITE_ENDPOINT, cmd, ptr - cmd, NULL, DEFAULT_TIMEOUT);
        if (ret < 0) {
            fprintf(stderr, "Write failed at offset %u\n", offset);
            return -1;
        }

        // Wait for write cycle (delay 10 ms)
        outBuf[0] = mCH341A_CMD_I2C_STREAM;
        outBuf[1] = mCH341A_CMD_I2C_STM_MS | 10;  // 10 ms delay
        outBuf[2] = mCH341A_CMD_I2C_STM_END;
        libusb_bulk_transfer(devHandle, BULK_WRITE_ENDPOINT, outBuf, 3, NULL, DEFAULT_TIMEOUT);

        offset += chunk;
        remaining -= chunk;
        fprintf(stdout, "Written %d%% [%d/%d] bytes   \r",
                (int)(100 * offset / bytesum), offset, bytesum);
    }
    fprintf(stdout, "Written 100%% [%d/%d] bytes   \n", offset, bytesum);
    return 0;
}

// --------------------------------------------------------------------------
int32_t parseEEPsize(char *eepromname, struct EEPROM *eeprom) {
    for (int i = 0; eepromlist[i].size; i++) {
        if (strcasecmp(eepromlist[i].name, eepromname) == 0) {
            *eeprom = eepromlist[i];
            return eepromlist[i].size;
        }
    }
    return -1;
}
