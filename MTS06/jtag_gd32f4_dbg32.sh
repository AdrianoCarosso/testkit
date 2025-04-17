#../src/openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "init; reset halt" &> /dev/null &
if [ "$1" != "" ] ; then
cp $1 code.gd32f4.bin
fi
./openocd32 -f interface/ftdi/RF_ft2232.cfg -f target/stm32f4x.cfg &> /dev/null &
sleep 1.5
echo "CPU $CPU --- USE 'shutdown' FROM TELNET TO EXIT"
telnet 127.0.0.1 4444
