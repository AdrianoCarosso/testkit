TARGET=..\openocd.exe
OBJEXT=o
SRCEXT=c

FTDI=..\..\FTDI\CDM\i386\ftd2xx.lib
# FTDI=-L..\..\libftdi-0.10 -lftdi -L..\..\libusb-0.1.12.1 -lusb

LIBRARIES = flash\libflash.a helper\libhelper.a jtag\libjtag.a pld\libpld.a server\libserver.a target\libtarget.a xsvf\libxsvf.a svf\libsvf.a

OBJECTS = main.$(OBJEXT) openocd.$(OBJEXT) startup_tcl.$(OBJEXT) hello.$(OBJEXT)

SOURCES = main.$(SRCEXT) openocd.$(SRCEXT) startup_tcl.$(SRCEXT) hello.$(SRCEXT)

CC=\mingw\bin\gcc
AR=\mingw\bin\ar
STRIP=\mingw\bin\strip -s
CFLAGS=-O2 -DHAVE_CONFIG_H -DPKGBLDREV=\"1086-TEST\" -I.. -I. -Iflash -Ihelper -Ijtag -Ipld -Iserver -Itarget -Ixsvf
LFLAGS=-Lserver -lserver -Lflash -Ljtag -ljtag -lflash -Lhelper -lhelper -Lpld -lpld -Ltarget -ltarget -Lxsvf -lxsvf -Lsvf -lsvf \
       $(FTDI) -lwsock32 -liberty
       
$(TARGET): $(OBJECTS) $(LIBRARIES)
        $(CC) -o $(TARGET) $(OBJECTS) $(LFLAGS)
        $(STRIP) $(TARGET)
        
clean:
        if exist $(TARGET) del /f /q $(TARGET)
        -del *.$(OBJEXT)

install: $(TARGET)
        copy $(TARGET) C:\util\gcc-cm3-4.4.2\tools\OpenOCD\$(TARGET)

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

