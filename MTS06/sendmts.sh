#!/bin/sh
#
case "$1" in
    -b)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg
	;;

    -o)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/stm32f4x.cfg
	;;

    -s)
	cd ~/test_kit/MTS06
	if [ "$2" != "" ] ; then
		cp $2 code.gd32f4.bin
	fi
	sudo openocd -f SendOOCD.gd32f4.cfg $3
	;;
    -p)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program zephyr.hex verify" -c reset -c exit
	;;


    *)
      echo "usage: $0	-b open sw session to nrf52 (use <adapter assert srst> command when test-kit connected "
      echo "		-o open jtag session to stm32"
      echo "		-s send code.gd32f4.bin to GD32F407 cpu"
      echo "		-p send zephyr.hex to nrf52832 cpu"
      exit 0
	;;
esac


exit



