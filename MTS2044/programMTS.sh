#!/bin/sh
#
#sudo ~/Documenti/MtsTestKit/dati_TestKit/Applicativi/CORTEX/sendverCORTEXTK1.sh ~/Documenti/MtsTestKit/dati_TestKit/2044_M4/Versioni/c2044v411 PROD assoreta

exit

case "$1" in
    -b)
	cd ~/test_kit/MTS2044_M4
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg
	;;

    -p)
	# example
	# ./programMTS.sh -p ./fw/merged.hex
	cd ~/test_kit/MTS2044_M4
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg -c init -c "reset init" -c halt -c "nrf5 mass_erase" -c "program $2 verify" -c reset -c exit
	;;

    -s)
	# example
	# ./programMTS.sh -s ./fw/M2054v413c.PROD.bin
	cd ~/test_kit/MTS06
	sudo openocd -c "set FW_NAME $2" -f loadFW.gd32f4.cfg
	;;
    *)
      echo "usage: $0	-b open sw session to nrf52"
      echo "		-p <arg.hex> send arg.hex to nrf52832 cpu"
      echo "		-s <arg.bin> send arg.bin to GD32F407 cpu"
      exit 0
	;;
esac

exit



