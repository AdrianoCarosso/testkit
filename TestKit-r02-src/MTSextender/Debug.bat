@echo off
start "ARM debugger" /MIN C:\util\gcc-arm-3.4.5\tools\OpenOCD\openocd -p 160 -f c:\util\gcc-arm-3.4.5\tools\OpenOCD\oocd_ft2232.cfg
echo USE 'shutdown' FROM TELNET TO EXIT
telnet 127.0.0.1 4444

