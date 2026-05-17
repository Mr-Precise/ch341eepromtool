//
//   ch341eeprom programmer
//
//   Programming tool for the 24Cxx serial EEPROMs using the Winchiphead CH341A IC
//
//   (c) December 2011 asbokid <ballymunboy@gmail.com>, extended by Precise
//
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
#include <getopt.h>
#include "ch341eeprom.h"

#ifdef _WIN32
#define NULL_DEVICE "nul"
#else
#define NULL_DEVICE "/dev/null"
#endif

FILE *debugout, *verbout;
uint8_t *readbuf = NULL;

// EEPROM database
const struct EEPROM eepromlist[] = {
    { "24c01",   128,    8, 1, 0 },
    { "24c02",   256,    8, 1, 0 },
    { "24c04",   512,   16, 1, 0 },
    { "24c08",  1024,   16, 1, 0 },
    { "24c16",  2048,   16, 1, 0 },
    { "24c32",  4096,   32, 2, 0 },
    { "24c64",  8192,   32, 2, 0 },
    { "24c128",16384,   32, 2, 0 },
    { "24c256",32768,   32, 2, 0 },
    { "24c512",65536,   32, 2, 0 },
    { "24c1024",131072, 32, 2, 0 },
    { 0, 0, 0, 0, 0 }
};

int main(int argc, char **argv) {
    int eepromsize = 0, bytesread = 0;
    uint8_t debug = 0, verbose = 0;
    struct libusb_device_handle *devHandle = NULL;
    char *filename = NULL, eepromname[12] = {0};
    char operation = 0;
    uint32_t speed = CH341_I2C_STANDARD_SPEED;
    struct EEPROM eeprom_info;
    FILE *fp;

    static char version_msg[] =
        "ch341eeprom - I2C EEPROM programmer for CH341a\n"
        "Version " CH341TOOLVERSION " (c) 2011 asbokid, extended by Precise\n\n"
        "This program comes with ABSOLUTELY NO WARRANTY.\n"
        "This is free software, and you are welcome to redistribute it\n"
        "under the terms of the GNU GPL v3.\n";

    static char usage_msg[] =
        "Usage: %s [options]\n"
        "  -h, --help               show this help\n"
        "  -v, --verbose            verbose output\n"
        "  -d, --debug              debug output\n"
        "  -s, --size <model>       EEPROM model (24c01 .. 24c1024)\n"
        "  -e, --erase              erase EEPROM (fill with 0xFF)\n"
        "  -p, --speed <low|fast|high>  set I2C speed (default standard 100kHz)\n"
        "  -w, --write <file>       write EEPROM from file\n"
        "  -r, --read  <file>       read EEPROM to file\n"
        "  -V, --verify <file>      verify EEPROM against file\n"
        "\nExample: %s -v -s 24c64 -w image.bin\n";

    static struct option longopts[] = {
        {"help",    no_argument,       0, 'h'},
        {"verbose", no_argument,       0, 'v'},
        {"debug",   no_argument,       0, 'd'},
        {"erase",   no_argument,       0, 'e'},
        {"size",    required_argument, 0, 's'},
        {"speed",   required_argument, 0, 'p'},
        {"write",   required_argument, 0, 'w'},
        {"read",    required_argument, 0, 'r'},
        {"verify",  required_argument, 0, 'V'},
        {0,0,0,0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "hvdes:p:w:r:V:", longopts, NULL);
        if (c == -1) break;
        switch (c) {
            case 'h':
                printf("%s", version_msg);
                printf(usage_msg, argv[0], argv[0]);
                return 0;
            case 'v': verbose = 1; break;
            case 'd': debug = 1; break;
            case 'e': if (!operation) operation = 'e'; else goto conflict; break;
            case 's':
                if ((eepromsize = parseEEPsize(optarg, &eeprom_info)) > 0)
                    strncpy(eepromname, optarg, 11);
                break;
            case 'p':
                if (strcmp(optarg, "low") == 0) speed = CH341_I2C_LOW_SPEED;
                else if (strcmp(optarg, "fast") == 0) speed = CH341_I2C_FAST_SPEED;
                else if (strcmp(optarg, "high") == 0) speed = CH341_I2C_HIGH_SPEED;
                else speed = CH341_I2C_STANDARD_SPEED;
                break;
            case 'w':
                if (!operation) { operation = 'w'; filename = strdup(optarg); }
                else goto conflict;
                break;
            case 'r':
                if (!operation) { operation = 'r'; filename = strdup(optarg); }
                else goto conflict;
                break;
            case 'V':
                if (!operation) { operation = 'V'; filename = strdup(optarg); }
                else goto conflict;
                break;
            default:
                fprintf(stderr, usage_msg, argv[0], argv[0]);
                return 1;
        }
    }
    if (operation == 0 || eepromsize <= 0) {
        fprintf(stderr, "Missing operation or EEPROM size\n");
        fprintf(stderr, usage_msg, argv[0], argv[0]);
        return 1;
    }

    debugout = debug ? stdout : fopen(NULL_DEVICE, "w");
    verbout  = verbose ? stdout : fopen(NULL_DEVICE, "w");

    readbuf = malloc(MAX_EEPROM_SIZE);
    if (!readbuf) { perror("malloc"); return 1; }

    devHandle = ch341configure(USB_LOCK_VENDOR, USB_LOCK_PRODUCT);
    if (!devHandle) goto err;

    if (ch341setstream(devHandle, speed) < 0) goto err;

    static const char *speed_names[] = { "20", "100", "400", "750" };
    fprintf(verbout, "I2C speed set to %s kHz\n", speed_names[speed]);

    switch (operation) {
        case 'r':   // read
            memset(readbuf, 0xFF, eepromsize);
            if (ch341readEEPROM(devHandle, readbuf, eepromsize) < 0) goto err;
            fp = fopen(filename, "wb");
            if (!fp) { perror("fopen write"); goto err; }
            fwrite(readbuf, 1, eepromsize, fp);
            fclose(fp);
            printf("Read %d bytes to %s\n", eepromsize, filename);
            break;
        case 'w':   // write
            fp = fopen(filename, "rb");
            if (!fp) { perror("fopen read"); goto err; }
            bytesread = fread(readbuf, 1, MAX_EEPROM_SIZE, fp);
            fclose(fp);
            if (bytesread < eepromsize) printf("Padding to %d bytes\n", eepromsize);
            if (bytesread > eepromsize) printf("Truncating to %d bytes\n", eepromsize);
            if (ch341writeEEPROM(devHandle, readbuf, eepromsize) < 0) goto err;
            printf("Wrote %d bytes to %s\n", eepromsize, eepromname);
            break;
        case 'e':   // erase
            memset(readbuf, 0xFF, eepromsize);
            if (ch341writeEEPROM(devHandle, readbuf, eepromsize) < 0) goto err;
            printf("Erased %s (%d bytes)\n", eepromname, eepromsize);
            break;
        case 'V':   // verify
            memset(readbuf, 0xFF, eepromsize);
            if (ch341readEEPROM(devHandle, readbuf, eepromsize) < 0) goto err;
            fp = fopen(filename, "rb");
            if (!fp) { perror("fopen verify"); goto err; }
            uint8_t *filebuf = malloc(eepromsize);
            if (!filebuf) { perror("malloc"); fclose(fp); goto err; }
            size_t n = fread(filebuf, 1, eepromsize, fp);
            fclose(fp);
            if (n != (size_t)eepromsize) {
                fprintf(stderr, "File size mismatch (%zu vs %d)\n", n, eepromsize);
                free(filebuf);
                goto err;
            }
            int mismatch = 0;
            for (int i = 0; i < eepromsize; i++) {
                if (readbuf[i] != filebuf[i]) {
                    printf("Verify failed at offset %d: EEPROM=0x%02X, file=0x%02X\n",
                           i, readbuf[i], filebuf[i]);
                    mismatch = 1;
                    break;
                }
            }
            if (!mismatch) printf("Verification passed (%d bytes)\n", eepromsize);
            free(filebuf);
            break;
        default:
            fprintf(stderr, "Unknown operation\n");
            goto err;
    }

    libusb_release_interface(devHandle, 0);
    libusb_close(devHandle);
    libusb_exit(NULL);
    free(readbuf);
    if (filename) free(filename);
    return 0;

conflict:
    fprintf(stderr, "Conflicting options\n");
err:
    if (readbuf) free(readbuf);
    if (filename) free(filename);
    if (devHandle) {
        libusb_release_interface(devHandle, 0);
        libusb_close(devHandle);
        libusb_exit(NULL);
    }
    return 1;
}
