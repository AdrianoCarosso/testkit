#!/bin/sh
sudo openocd -f interface/ftdi/MTS16_BLE.cfg -f target/nrf52.cfg

exit
cp SendOOCD.script.1 SendOOCD.script.bk
./openocd -p 160 -f SendOOCD.LPC17XX.cfg
cp SendOOCD.script.2 SendOOCD.script.bk
./openocd -p 130 -l nul -f SendOOCD.LPC17XX.cfg

