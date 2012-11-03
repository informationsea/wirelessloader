wirelessloader
==============

Bootloader for AVR devices, optimized to write via wireless usart access.

System requirements
-----------------

### PC

* Windows
* Mac OS X
* Linux

### AVR Devices

* ATmega328P
* ATmega644P

You can port to other devices with bootloader support easily,
especially to family devices with above.

Build bootloader
----------------

1. Change to `avr` directory
2. run `make` to build with default configuration
    * Currently only 38400 is supported by driver program
3. run `./build.sh` to build for supported devices

Build driver program for bootloader
-----------------------------------

1. You have to install gcc to build this.
    * I recommend MinGW to build on Windows
    * Install command line tools to build on OS X
    * Install `build-essential` to build on Ubuntu
2. Change to `pc` directory
3. run `make` to build
4. run `sudo make install` to install your Unix like system
    * copy `avrwrite.exe` to your path on Windows

Write bootloader to AVR
-----------------------

1. Build bootloader for your device.
    * Don't forget change `F_CPU` and `BAUDRATE`, `MCU`, `BOOTLOADER_ADDRESS`
2. Write `serialloader.hex` with your favorite AVR writer.
3. Change fuse bits to load bootloader.
    * Read chapter "Boot Loader Support", section "ATmegaXX Boot Loader Parameters" carefully
    * 1 word = 2 bytes
    * The value of `BOOTLOADER_ADDRESS` should be double of "Boot Reset Address"
    * This bootloader require 2kB of flash memory
4. Reset your device
    * Bootloader automatically lock your device to protect bootloader program.

Write your AVR program with bootloader
--------------------------------------
1. You have to disconnect your serial terminal, which will be used to write.
2. run `avrwrite -p [serialport] [yourprogram.hex]`
    * Example for Windows `avrwrite -p COM4 small.hex`
    * Example for Linux `avrwrite -p /dev/ttyUSB0 small.hex`
    * Example for OS X `avrwrite -p /dev/cu.usbserialXXX small.hex`
3. Reconnect serial terminal
4. Send `X` to exit bootloader and run your program.
    * If you want to customize bootloader exit condition, you should edit `main` function at `avr/bootloader.c`
    * You can run your program after writing soon with `-u` option.

### Auto disconnect, write program and re-connect with Tera Term
[Tera Term](http://sourceforge.jp/projects/ttssh2/) is one of most useful terminal application to talk with serial devices.
I wrote useful macro to work with wirelessloader. You can find them at `pc/for teraterm`.


TODO
----
* read fuse bits
* read eeprom
* change baudrate
* a lots!

License
-------

### wirelessloader - Bootloader for AVR devices, optimized to write via wireless usart access.

Copyright (C) 2012  Y.Okamura

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
 any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

### avr-libc

Copyright (C) 2012 avr-libc project
