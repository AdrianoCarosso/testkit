TARGET=libpld.a
OBJEXT=o
SRCEXT=c

OBJECTS = pld.o virtex2.o xilinx_bit.o

SOURCES = pld.c virtex2.c xilinx_bit.c

CC=\mingw\bin\gcc
AR=\mingw\bin\ar
CFLAGS=-O2 -DHAVE_CONFIG_H -I. -I.. -I..\.. -I..\target -I..\helper -I..\jtag -std=c99

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

