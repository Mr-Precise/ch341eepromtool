### About

Beta libusb code for EEPROM programmers based on the WinChipHead CH341a IC

### Author

Written by asbokid and released under the terms of the GNU GPL, version 3, or later.

Copyright Dec 2011, asbokid <ballymunboy@gmail.com>

Maintenance and build system improvements by community contributors.

### Licence

This is free software: you can redistribute it and/or modify it under the terms of the latest GNU General Public License as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;  
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  
If not, see <http://www.gnu.org/licenses/>.

### Requirements

gcc or MinGW  
GNU make or CMake (3.7+)  
libusb-1.0 and the libusb library development files.  
On Debian-based distros: `libusb-1.0-0-dev`  
Cross-compiling (MinGW): install libusb manually.

### Building with Make

Basic build:  
`make`

Build with test image generator:  
`make BUILD_MKTESTIMG=1`

Install (may require superuser):  
`sudo make install`

Uninstall:  
`sudo make uninstall`

Respect DESTDIR:  
`make install DESTDIR=/path/to/altroot`

### Building with CMake

Configure and build:
```
mkdir build && cd build
cmake ..
make
```

Enable test image generator:
```
cmake -DBUILD_MKTESTIMG=ON ..
make
```

Install to /usr/local (prefix can be changed with -DCMAKE_INSTALL_PREFIX):  
`sudo make install`

Or using CMake's built-in installer:  
`cmake --install . --prefix /usr/local`

Manual libusb paths (when pkg-config is not available):  
`cmake -DLIBUSB_INCLUDE_DIR=/path/to/include -DLIBUSB_LIBRARY=/path/to/libusb-1.0.lib ..`

### Running

`$ ./ch341eeprom`

ch341eeprom - an i2c EEPROM programming tool for the WCH CH341a IC  
Version 0.5 copyright (c) 2011  asbokid <ballymunboy@gmail.com>

This program comes with absolutely no warranty; This is free software,  
and you are welcome to redistribute it under certain conditions:  
GNU GPL v3 License: http://www.gnu.org/licenses/gpl.html

```
Usage:
 -h, --help             display this text
 -v, --verbose          verbose output
 -d, --debug            debug output
 -s, --size             size of EEPROM {24c32|24c64}
 -e, --erase            erase EEPROM (fill with 0xff)
 -w, --write <filename> write EEPROM with image from filename
 -r, --read  <filename> read EEPROM and save image to filename
```

Example:  ch341eeprom -v -s 24c64 -w bootrom.bin

`$ sudo ./ch341eeprom -v -s 24c64 -e`

Searching USB buses for WCH CH341a i2c EEPROM programmer [1a86:5512]  
Found [1a86:5512] as device [7] on USB bus [2]  
Opened device [1a86:5512]  
Claimed device interface [0]  
Device reported its revision [3.03]  
Configured USB device  
Set i2c bus speed to [100kHz]  
Erased [8192] bytes of [24c64] EEPROM  
Closed USB device

`$ sudo ./ch341eeprom -v -s 24c64 -r output.bin`

Searching USB buses for WCH CH341a i2c EEPROM programmer [1a86:5512]  
Found [1a86:5512] as device [7] on USB bus[2]  
Opened device [1a86:5512]  
Claimed device interface [0]  
Device reported its revision [3.03]  
Configured USB device  
Set i2c bus speed to [100kHz]  
Read [8192] bytes from [24c64] EEPROM  
Wrote [8192] bytes to file [output.bin]  
Closed USB device

`$ xxd -l 128 output.bin`

```
0000000: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000010: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000020: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000030: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000040: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000050: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000060: ffff ffff ffff ffff ffff ffffffff ffff  ................
0000070: ffff ffff ffff ffff ffff ffffffff ffff  ................
```
`$ ./mktestimg > testimg24c64.bin`

`$ xxd -l 128 testimg24c64.bin`
```
0000000: 0000 0000 0000 0000 0000 00000000 0000  ................
0000010: 1111 1111 1111 1111 1111 11111111 1111  ................
0000020: 2222 2222 2222 2222 2222 22222222 2222  """"""""""""""""
0000030: 3333 3333 3333 3333 3333 33333333 3333  3333333333333333
0000040: 4444 4444 4444 4444 4444 44444444 4444  DDDDDDDDDDDDDDDD
0000050: 5555 5555 5555 5555 5555 55555555 5555  UUUUUUUUUUUUUUUU
0000060: 6666 6666 6666 6666 6666 66666666 6666  ffffffffffffffff
0000070: 7777 7777 7777 7777 7777 77777777 7777  wwwwwwwwwwwwwwww
```
`$ sudo ./ch341eeprom -v -s 24c64 -w testimg24c64.bin`

Searching USB buses for WCH CH341a i2cEEPROM programmer [1a86:5512]  
Found [1a86:5512] as device [7] on USB bus[2]  
Opened device [1a86:5512]  
Claimed device interface [0]
Device reported its revision [3.03]  
Configured USB device  
Set i2c bus speed to [100kHz]  
Read [8192] bytes from file [testimg24c64bin]  
Wrote [8192] bytes to [24c64] EEPROM  
Closed USB device  

`$ sudo ./ch341eeprom -v -s 24c64 -r output.bin`

Searching USB buses for WCH CH341a i2cEEPROM programmer [1a86:5512]  
Found [1a86:5512] as device [7] on USB bus[2]  
Opened device [1a86:5512]  
Claimed device interface [0]  
Device reported its revision [3.03]  
Configured USB device  
Set i2c bus speed to [100kHz]  
Read [8192] bytes from [24c64] EEPROM  
Wrote [8192] bytes to file [output.bin]  
Closed USB device  

`$ xxd -l 128 output.bin`
```
0000000: 0000 0000 0000 0000 0000 00000000 0000  ................
0000010: 1111 1111 1111 1111 1111 11111111 1111  ................
0000020: 2222 2222 2222 2222 2222 22222222 2222  """"""""""""""""
0000030: 3333 3333 3333 3333 3333 33333333 3333  3333333333333333
0000040: 4444 4444 4444 4444 4444 44444444 4444  DDDDDDDDDDDDDDDD
0000050: 5555 5555 5555 5555 5555 55555555 5555  UUUUUUUUUUUUUUUU
0000060: 6666 6666 6666 6666 6666 66666666 6666  ffffffffffffffff
0000070: 7777 7777 7777 7777 7777 77777777 7777  wwwwwwwwwwwwwwww
```

### Concluding Notes

The code handles the 3 byte addressing used by EEPROMS of 32kbit and greater (24c32-)  
It uses asynchronous USB transfers but should be portable to Microsoft Windows.

Test image generator (mktestimg) can be built optionally.

All comments and contributions welcomed!

asbokid <ballymunboy@gmail.com> - Dec 2011
Maintained at https://github.com/Mr-Precise/ch341eepromtool
