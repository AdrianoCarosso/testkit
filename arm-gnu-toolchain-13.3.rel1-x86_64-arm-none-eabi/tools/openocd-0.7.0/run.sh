#!/bin/bash
sudo ./openocd -p 160 -f ./oocd_stm32f4x.cfg -f stm32f4x.cfg

exit
# JTAG on USB port
su -c "./openocd -p 160 -f ./oocd_stm32f4x.cfg &> /dev/null &"
sleep 0.3
echo "USE -shutdown- FROM TELNET TO EXIT"
telnet 127.0.0.1 4444

