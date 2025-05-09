#!/bin/sh
#
case "$1" in
    -b)
	cd ~/test_kit/MTS06
	sudo openocd -f tcl/interface/ftdi/MTS16_BLE.cfg -f tcl/target/nrf52.cfg
	;;

    -g)
	#cd ~/test_kit/MTS06
	#sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/stm32f4x.cfg
	sudo openocd -f tcl/interface/ftdi/test-kit2.cfg -f tcl/target/gd32f30x.cfg -c init -c "reset init" -c halt -c "mmw 0xE0042008 0x00001800 0"
	# "mmw 0xE0042008 0x00001800 0" to disable WatchDog
	;;

    -p)
	# example
	# ./programMTS.sh -p ./fw/merged.hex
	cd ~/test_kit/MTS06
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
      echo "		-g open jtag session with gd32f4x"
      echo "		-p <arg.hex> send arg.hex to nrf52832 cpu"
      echo "		-s <arg.bin> send arg.bin to GD32F407 cpu"
      #echo "ttyUSB0------DM Port, on EG91 is AT Port"
      #echo "ttyUSB1------NEMA Port"
      #echo "ttyUSB2------AT Port"
      #echo "ttyUSB3------Modem Port"
      #echo "ttyUSB4------Wireless Ethernet"
      exit 0
	;;
esac


exit

#Qflash_V7.0

