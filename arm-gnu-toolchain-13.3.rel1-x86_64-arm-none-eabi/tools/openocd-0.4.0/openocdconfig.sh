#!/bin/sh	
#
# sudo src/openocd -p 160 -f tcl/interface/RF_ft2232.cfg -f tcl/target/stm32f4x.cfg
# openocd -f tcl/interface/ftdi/swd-resistor-hack2.cfg -f tcl/target/nrf52.cfg
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
./configure --enable-ft2232_libftdi CFLAGS="-Wno-error=redundant-decls -Wno-error=unused-but-set-variable -Wno-error=implicit-fallthrough -Wno-error=sizeof-pointer-memaccess -Wno-error=duplicate-decl-specifier -Wno-error=tautological-compare -Wno-error=shift-overflow -Wno-error=stringop-overflow -Wno-error=unused-const-variable -Wno-error=enum-conversion"

exit

