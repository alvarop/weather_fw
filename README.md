
## Setup Instructions

### Dependencies
Make sure you have python and pipenv installed.
You'll also need ARM's GCC compiler (version 6, 7 is yet not supported by Mbed OS)

### Setup python environment
Run `pipenv install --two` to setup (Mbed tools don't like Python 3 yet)

### Setup GCC_ARM_PATH
`pipenv run mbed-cli config -G GCC_ARM_PATH /path/to/gcc-arm-none-eabi-6-2017-q2-update/bin/`

## Compiling/flashing
To compile, run `pipenv run mbed compile`
To compile and flash, run `pipenv run mbed compile --flash`

## Debug

### Makefiles
To create makefiles, run `pipenv run mbed export -i gcc_arm`. Now you can build with `make`

### OpenOCD
To connect debugger with openocd, run:

`openocd -f /usr/share/openocd/scripts/board/st_nucleo_l476rg.cfg -f /usr/share/openocd/scripts/interface/stlink-v2-1.cfg -c init -c "reset init"`

To use gdb, run: `/path/to/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-gdb`