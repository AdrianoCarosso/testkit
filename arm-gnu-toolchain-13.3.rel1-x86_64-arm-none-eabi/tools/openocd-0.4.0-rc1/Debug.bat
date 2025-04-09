@echo off
rem JTAG on USB port
start "ARM debugger" /MIN C:\brignolo\Programmers\openocd-0.4.0-rc1\openocd -p 160 -f C:\brignolo\Programmers\openocd-0.4.0-rc1\oocd_ft2232.cfg
echo USE 'shutdown' FROM TELNET TO EXIT
telnet 127.0.0.1 4444

