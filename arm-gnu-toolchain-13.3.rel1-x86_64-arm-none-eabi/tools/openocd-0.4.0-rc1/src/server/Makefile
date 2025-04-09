TARGET=libserver.a
OBJEXT=o
SRCEXT=c

OBJECTS = gdb_server.o server.o telnet_server.o tcl_server.o server_stubs.o

SOURCES = gdb_server.c server.c telnet_server.c tcl_server.c server_stubs.c

CC=\mingw\bin\gcc
AR=\mingw\bin\ar
CFLAGS=-O2 -DHAVE_CONFIG_H -I. -I.. -I..\.. -I..\target -I..\helper -I..\jtag -I..\flash -std=c99

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

