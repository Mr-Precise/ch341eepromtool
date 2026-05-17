// libUSB driver for the ch341a in i2c mode
//
// Copyright 2011 asbokid <ballymunboy@gmail.com>, extended by Precise
//

#ifndef CH341EEPROM_H
#define CH341EEPROM_H

#define CH341TOOLVERSION "0.5.1"

#define USB_LOCK_VENDOR  0x1a86
#define USB_LOCK_PRODUCT 0x5512

#define MAX_EEPROM_SIZE  131072   // for 24c1024

#define BULK_WRITE_ENDPOINT 0x02
#define BULK_READ_ENDPOINT  0x82
#define DEFAULT_INTERFACE   0x00
#define DEFAULT_CONFIGURATION 0x01
#define DEFAULT_TIMEOUT 300

#define IN_BUF_SZ 0x100
#define EEPROM_WRITE_BUF_SZ 0x2b
#define EEPROM_READ_BULKIN_BUF_SZ 0x20
#define EEPROM_READ_BULKOUT_BUF_SZ 0x65

/* CH341A commands */
#define	  mCH341A_CMD_I2C_STREAM      0xAA
#define   mCH341A_CMD_I2C_STM_STA     0x74
#define   mCH341A_CMD_I2C_STM_STO     0x75
#define   mCH341A_CMD_I2C_STM_OUT     0x80
#define   mCH341A_CMD_I2C_STM_IN      0xC0
#define   mCH341A_CMD_I2C_STM_SET     0x60
#define   mCH341A_CMD_I2C_STM_MS      0x50
#define   mCH341A_CMD_I2C_STM_END     0x00

/* I2C speeds */
#define CH341_I2C_LOW_SPEED      0   // 20 kHz
#define CH341_I2C_STANDARD_SPEED 1   // 100 kHz
#define CH341_I2C_FAST_SPEED     2   // 400 kHz
#define CH341_I2C_HIGH_SPEED     3   // 750 kHz

/* Fixed read setup command for 24c64 (original) – we'll keep it generic */
#define CH341_EEPROM_READ_SETUP_CMD "\xaa\x74\x83\xa0\x00\x00\x74\x81\xa1\xe0\x00\x00\x06\x04\x00\x00" \
                                    "\x00\x00\x00\x00\x40\x00\x00\x00\x11\x4d\x40\x77\xcd\xab\xba\xdc" \
                                    "\xaa\xe0\x00\x00\xc4\xf1\x12\x00\x11\x4d\x40\x77\xf0\xf1\x12\x00" \
                                    "\xd9\x8b\x41\x7e\x00\xf0\xfd\x7f\xf0\xf1\x12\x00\x5a\x88\x41\x7e" \
                                    "\xaa\xe0\x00\x00\x2a\x88\x41\x7e\x06\x04\x00\x00\x11\x4d\x40\x77" \
                                    "\xe8\xf3\x12\x00\x14\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" \
                                    "\xaa\xdf\xc0\x75\x00"

#define CH341_EEPROM_READ_NEXT_CMD "\xaa\x74\x83\xa0\x00\x00\x74\x81\xa1\xe0\x00\x00\x10\x00\x00\x00" \
                                   "\x00\x00\x00\x00\x8c\xf1\x12\x00\x01\x00\x00\x00\x00\x00\x00\x00" \
                                   "\xaa\xe0\x00\x00\x4c\xf1\x12\x00\x5d\x22\xd7\x5a\xdc\xf1\x12\x00" \
                                   "\x8f\x04\x44\x7e\x30\x88\x41\x7e\xff\xff\xff\xff\x2a\x88\x41\x7e" \
                                   "\xaa\xe0\x00\x7e\x00\x00\x00\x00\x69\x0e\x3c\x00\x12\x01\x19\x00" \
                                   "\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x9c\x2e\x68\x00" \
                                   "\xaa\xdf\xc0\x75\x00"
#define CH341_EEPROM_READ_CMD_SZ 0x65

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define TRUE  1
#define FALSE 0

struct EEPROM {
    char *name;
    uint32_t size;
    uint16_t page_size;
    uint8_t addr_size;   // bytes used for address (1 or 2)
    uint8_t addr;        // chip select bits (A2,A1,A0)
};

extern const struct EEPROM eepromlist[];

extern uint8_t *readbuf;

int32_t ch341readEEPROM(struct libusb_device_handle *devHandle, uint8_t *buf, uint32_t bytes);
int32_t ch341writeEEPROM(struct libusb_device_handle *devHandle, uint8_t *buf, uint32_t bytes);
struct libusb_device_handle *ch341configure(uint16_t vid, uint16_t pid);
int32_t ch341setstream(struct libusb_device_handle *devHandle, uint32_t speed);
int32_t parseEEPsize(char *eepromname, struct EEPROM *eeprom);

void cbBulkIn(struct libusb_transfer *transfer);
void cbBulkOut(struct libusb_transfer *transfer);

#endif
