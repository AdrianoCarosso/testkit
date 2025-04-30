#!/bin/sh
#
case "$1" in
    -b)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg
	;;

    -g)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/gd32f30x.cfg
	;;

    -k)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program fw/zephyr.hex verify" -c reset -c exit
	;;

    -o)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/stm32f4x.cfg
	;;

    -p)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program fw/merged.hex verify" -c reset -c exit
	;;

    -s)
	cd ~/test_kit/MTS06
	sudo openocd -f loadFW.gd32f4.cfg
	;;
    -z)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program fw/Button0832.hex verify" -c reset -c exit
	;;

    *)
      echo "usage: $0	-b open sw session to nrf52"
      echo "		-g open jtag session with gd32f4x"
      echo "		-k send zephyr.hex to nrf52832 cpu"
      echo "		-o open jtag session to stm32 (use <adapter assert srst> command before program nrf52)"
      echo "		-p send merged.hex to nrf52832 cpu"
      echo "		-s send code.gd32f4.bin to GD32F407 cpu"
      echo "		-z send Button0832.hex to nrf52832 cpu"
      exit 0
	;;
esac


exit



