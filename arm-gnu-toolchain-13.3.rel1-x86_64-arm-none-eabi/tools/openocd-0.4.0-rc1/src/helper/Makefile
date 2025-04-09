TARGET=libhelper.a
OBJEXT=o
SRCEXT=c

#OBJECTS = binarybuffer.o command.o configuration.o fileio.o interpreter.o \
#          log.o replacements.o time_support.o options.o
#
#SOURCES = binarybuffer.c command.c configuration.c fileio.c interpreter.c \
#          log.c replacements.c time_support.c options.c
#
OBJECTS = binarybuffer.o options.o jim.o \
          jim-eventloop.o configuration.o log.o command.o time_support_common.o \
          replacements.o fileio.o time_support.o membuf.o

SOURCES = binarybuffer.c options.c jim.c \
          jim-eventloop.c configuration.c log.c command.c time_support_common.c \
          replacements.c fileio.c time_support.c membuf.c

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

