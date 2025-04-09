TARGET=libsvf.a
OBJEXT=o
SRCEXT=c

OBJECTS = svf.$(OBJEXT)

SOURCES = svf.$(SRCEXT)

CC=\mingw\bin\gcc
AR=\mingw\bin\ar
CFLAGS=-O2 -DHAVE_CONFIG_H -I. -I..\.. -I.. -I..\target -I..\helper -I..\jtag -std=c99

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

