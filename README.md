# AnalogX
Firmware for AnalogX - A 4 channel analog to CAN bus interface board

### Toolchain Setup
Steps for compiling firmware
* Download [the official
  toolchain](https://launchpad.net/gcc-arm-embedded/+milestone/4.9-2014-q4-major)
* Extract the tarball to a directory of your choice
* Add the 'bin' directory from within the newly extracted directory to
  your system PATH.

### Compiling Firmware
From the root of the project, simply run `make`.  This will build the package.

### Writing firmware
The STM32F042 processor is programmed via ARM SWD; we recommend the ST Link V2. 
* SWD pads are provided on the bottom of the board.  These pads are near the center of the board and correspond to the standard SWD connections:
** SWDIO
** SWCLK
** RST
** GND
** 3.3v
