TARGET=libjtag.a
OBJEXT=o
SRCEXT=c

FTDI=-I..\..\..\FTDI\CDM
# FTDI=-I..\jtag -I..\..\..\libftdi-0.10 -I..\..\..\libusb-0.1.12.1

# at91rm9200.o ep93xx.o usbprog.o
OBJECTS = drivers\amt_jtagaccel.o drivers\bitbang.o drivers\bitq.o drivers\ft2232.o drivers\gw16012.o \
          drivers\parport.o drivers\driver.o \
          commands.o core.o interface.o interfaces.o tcl.o

# at91rm9200.c ep93xx.c usbprog.c
SOURCES = drivers\amt_jtagaccel.c drivers\bitbang.c drivers\bitq.c drivers\ft2232.c drivers\gw16012.c \
          driver\parport.c drivers\driver.c \
          commands.c core.c interface.c interfaces.c tcl.c

CC=\mingw\bin\gcc
AR=\mingw\bin\ar
CFLAGS=$(FTDI) -O2 -DHAVE_CONFIG_H -I. -I.. -I..\.. -I..\target -I..\helper -std=c99

$(TARGET): $(OBJECTS)
        if exist $(TARGET) del /f /q $(TARGET)
        $(AR) rc $(TARGET) $(OBJECTS)

clean:
        if exist $(TARGET) del /f /q $(TARGET)
        -del *.$(OBJEXT)

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

