#!/bin/bash

# JTAG on USB port
su -c "/opt/gcc32-arm-3-4.5/tools/OpenOCD/openocd -p 160 -f /opt/gcc32-arm-3-4.5/tools/OpenOCD/oocd_ft2232.cfg &> /dev/null &"
sleep 0.3
echo "USE -shutdown- FROM TELNET TO EXIT"
telnet 127.0.0.1 4444

