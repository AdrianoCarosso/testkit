#!/bin/sh	
# libftdi is required
#
# SEGGER J-Link
# sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/gd32f30x.cfg
# sudo openocd -f tcl/interface/ftdi/swd-resistor-hack2.cfg -f tcl/target/nrf52.cfg
#
# reset halt
# flash write_image erase /home/name/ncs/app/BlueNor_240123/Source/BLE-MTS06/build/zephyr/zephyr.elf
#./configure --enable-usb-blaster --enable-jlink LDFLAGS="-static"
#./configure --enable-usb-blaster --enable-jlink CFLAGS="-static"
#export LDFLAGS="--static"; ./configure --enable-jlink
#export CFLAGS="--static"; ./configure --enable-jlink
#export CFLAGS="--static"; ./configure --disable-jlink
#./configure --enable-usb-blaster --enable-jlink CFLAGS="--enable-static --disable-shared"

#./configure --enable-internal-libjaylink CFLAGS="--static"
#./configure --enable-internal-libjaylink CFLAGS="--static /usr/local/lib/libjaylink.a /usr/local/lib/libftdi1.a"
#./configure CFLAGS="--static /usr/local/lib/libjaylink.a"
#./configure --enable-internal-libjaylink LDFLAGS="--static /usr/local/lib/libjaylink.a /usr/local/lib/libftdi1.a"
#export LDFLAGS="--static /usr/local/lib/libjaylink.a /usr/local/lib/libftdi1.a"; ./configure --enable-internal-libjaylink
#./configure --enable-static LDFLAGS="--static /usr/local/lib/libjaylink.a /usr/local/lib/libftdi1.a"
#./configure --enable-static --disable-shared --enable-internal-libjaylink LDFLAGS="--static /usr/local/lib/libjaylink.a /usr/local/lib/libftdi1.a"
#./configure --enable-static --disable-shared --enable-jlink=no LDFLAGS="--static /usr/local/lib/libjaylink.a /usr/local/lib/libftdi1.a"
#./configure --enable-static --disable-shared --enable-jlink=no LDFLAGS="--static /usr/local/lib/libftdi1.a"
#./configure --enable-jlink=no --enable-ftdi --enable-internal-libjaylink LDFLAGS="--static /usr/local/lib/libftdi1.a"
./configure --disable-jlink --enable-ftdi --enable-internal-libjaylink

exit

#gcc -Wall -Wstrict-prototypes -Wformat-security -Wshadow -Wextra -Wno-unused-parameter -Wbad-function-cast -Wcast-align -Wredundant-decls -Wpointer-arith -Wundef -Werror -g -O2 --static -o src/openocd src/main.o  /usr/local/lib/libftdi1.a src/.libs/libopenocd.a -L/usr/lib64 -L/usr/local/lib -lusb-1.0 -lm -ljim -lssl -lcrypto -lz -lutil -ldl
gcc -Wall -Wstrict-prototypes -Wformat-security -Wshadow -Wextra -Wno-unused-parameter -Wbad-function-cast -Wcast-align -Wredundant-decls -Wpointer-arith -Wundef -Werror -g -O2 --static -o src/openocd src/main.o src/.libs/libopenocd.a /usr/local/lib/libftdi1.a /usr/local/lib/libusb-1.0.a -L/usr/lib64 -L/usr/local/lib  -lm -ljim -lssl -lcrypto -lz -lutil -ldl
