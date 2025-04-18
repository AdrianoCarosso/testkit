#!/bin/sh
#
case "$1" in
    -b)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg
	;;

    -o)
	cd ~/test_kit/MTS06
	#sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/stm32f4x.cfg -c init -c "reset init" -c "adapter assert srst"
	sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/stm32f4x.cfg
	;;

    -s)
	cd ~/test_kit/MTS06
	if [ "$2" != "" ] ; then
		cp $2 code.gd32f4.bin
	fi
	sudo openocd -f SendOOCD.gd32f4.cfg $3
	;;
    -k)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program zephyr.hex verify" -c reset -c exit
	;;

    -z)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program Button0832.hex verify" -c reset -c exit
	;;

    -p)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program merged.hex verify" -c reset -c exit
	;;

    *)
      echo "usage: $0	-b open sw session to nrf52"
      echo "		-o open jtag session to stm32 (use <adapter assert srst> command before program nrf52)"
      echo "		-s send code.gd32f4.bin to GD32F407 cpu"
      echo "		-k send zephyr.hex to nrf52832 cpu"
      echo "		-z send Button0832.hex to nrf52832 cpu"
      echo "		-p send merged.hex to nrf52832 cpu"
      exit 0
	;;
esac


exit



