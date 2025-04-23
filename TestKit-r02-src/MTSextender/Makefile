######################################################
# MakeOptions for this task
# Target CPU; arm7 

TASK=main.$(DBG)

######################################################
# select device and options

ifndef CPU
CPU=AT91SAM7A3
#CPU=AT91SAM7S256
#CPU=LPC17XX
endif

# DEBUG may be:         1, 0
# Set default value if undeclared
ifndef DEBUG
DEBUG=0
endif

ifeq ($(DEBUG),0)
DBG=PROD
DBGMSG=none (production)
else
DBG=CBUG
DBGMSG=YES
endif

######################################################
# select directory options
COS=~/test_kit/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi
COSPREF=/bin/arm-none-eabi-
OOCDMTS=$(COS)/tools/OpenOCD/openocd -p 28 -l nul

CC=$(COS)$(COSPREF)gcc
AR=$(COS)$(COSPREF)ar
LK=$(COS)$(COSPREF)gcc
OC=$(COS)$(COSPREF)objcopy
DP=$(COS)$(COSPREF)objdump
SZ=$(COS)$(COSPREF)size
STRIP=$(COS)$(COSPREF)strip

#OOCD=$(COS)/tools/OpenOCD/openocd -p 160
OOCD=$(COS)/tools/openocd-0.7.0/openocd -p 160

CP=@cp -p
RM=@rm -f
RD=@rm -fr
WC=@wc

KERNELDIR=../kernel
DRIVERDIR=../driver
LIBS=Libs
OBJS=Objs
LIBEXT=$(DBG).a
OBJEXT=$(DBG).o

######################################################
# select CPU options

ifeq ($(CPU),LPC17XX)

FULLARM=-mthumb -mcpu=cortex-m3 -msoft-float
THUMB=-mthumb -mcpu=cortex-m3 -msoft-float
DRVDIR=DRV_LPC17XX
STARTUP=$(LIBS)/RTXCstartup_lpc17.$(DBG).o $(LIBS)/RTXCutil.$(DBG).o

else

FULLARM=-mcpu=arm7tdmi -msoft-float
THUMB=-mthumb -mcpu=arm7tdmi -msoft-float
#THUMB=-mcpu=arm7tdmi -msoft-float
DRVDIR=DRV_AT91SAM7
STARTUP=$(LIBS)/RTXCstartup.$(DBG).o $(LIBS)/RTXCutil.$(DBG).o $(LIBS)/RTXCsys32.$(DBG).o

endif

######################################################
# select compile/link options
#

# lib
ARFLAGS=rc

# compiler
# -mthumb-interwork
OPTIM=-O2 -fomit-frame-pointer -fpack-struct
OPTIM=-O2 -fomit-frame-pointer -fno-strict-aliasing
#CCFLAGS=-c -g -I$(KERNELDIR) -I$(DRIVERDIR)  -I./ -D$(DBG) -DUSE_$(CPU) $(OPTIM) -nostartfiles -Wall -Werror $(THUMB)
CCFLAGS=-c -g -I$(KERNELDIR) -I$(DRIVERDIR)  -I./ -D$(DBG) -DUSE_$(CPU) $(OPTIM) -nostartfiles -Wall $(THUMB)
CCFLAGS32=-c -g -I$(KERNELDIR) -I$(DRIVERDIR) -I./ -D$(DBG) -DUSE_$(CPU) $(OPTIM) -nostartfiles -Wall -Werror $(FULLARM)

# linker
LIB_RTXC=rtxc.$(DBG)
LIB_DRV=drv.$(DBG)
LDLIBS=-L$(LIBS) -l$(LIB_RTXC) -l$(LIB_DRV) -lm
LDFLAGS=-nostartfiles $(THUMB)
MAP=-Wl,-Map,$(TASK).map -Wl,--cref

# loader
LD_FILE=_$(CPU)-ROM.ld

# HEX file
OCFLAGS= --srec-len 32

INCLUDE_ASSIGN= assign.h

######################################################
# check local/public directories

dummy1 := $(shell test -d $(OBJS) || mkdir -p $(OBJS))
dummy2 := $(shell test -d $(LIBS) || mkdir -p $(LIBS))
#dummy3 := $(shell echo "Compile options: CPU=$(CPU) $(THUMB), DEBUG=$(DBGMSG)" >/dev/console)

TEMPFILE = ./SendOOCD.script

######################################################
# select directory options

#ALL: $(TASK).bin $(TASK).srec
all: $(TASK).bin $(TASK).srec
	@echo "Compile options: CPU=$(CPU) $(THUMB), DEBUG=$(DBGMSG)"


srec: $(TASK).srec


dump: $(TASK).dump


tot:
	$(WC) -lc ../kernel/*.h ../driver/*.h *.h ../kernel/*.c ../driver/*.c *.c


clean:
	-$(RD) $(OBJS)
	-$(RD) $(LIBS)
	-$(RM) *.map
	-$(RM) *.srec
	-$(RM) *.bin
	-$(RM) *.dump
	-$(RM) *.bin

# erase first 128KB: flash erase 0 0 17
chip: $(TASK).bin
	echo "halt" >$(TEMPFILE)
	echo "sleep 10" >>$(TEMPFILE)
	echo "soft_reset_halt" >>$(TEMPFILE)
	echo "sleep 10" >>$(TEMPFILE)
	echo "flash probe 0" >>$(TEMPFILE)
	echo "flash write_bank 0 $(TASK).bin 0x0" >>$(TEMPFILE)
	echo "resume" >>$(TEMPFILE)
	echo "reset" >>$(TEMPFILE)
	echo "shutdown" >>$(TEMPFILE)
	$(OOCD) -f SendOOCD.AT91.cfg
	$(RM) $(TEMPFILE)

chipreset:
	echo "reset" >$(TEMPFILE)
	echo "shutdown" >>$(TEMPFILE)
	$(OOCD) -f SendOOCD.AT91.cfg
	$(RM) $(TEMPFILE)

setserial:
	echo "shutdown" >$(TEMPFILE)
	$(OOCDMTS) -f SendOOCD.AT91.cfg
	$(RM) $(TEMPFILE)

######################################################
# KERNEL section

#
# Library files
#

$(LIBS)/lib$(LIB_RTXC).a: $(OBJS)/rtxcinit.$(OBJEXT) $(OBJS)/rtxcapi.$(OBJEXT) \
	       $(OBJS)/rtxc.$(OBJEXT) $(OBJS)/lock.$(OBJEXT)
	-$(RM) $(LIBS)/lib$(LIB_RTXC).a
	$(AR) $(ARFLAGS) $(LIBS)/lib$(LIB_RTXC).a $(OBJS)/rtxcinit.$(OBJEXT) $(OBJS)/rtxcapi.$(OBJEXT) \
	       $(OBJS)/rtxc.$(OBJEXT) $(OBJS)/lock.$(OBJEXT)

#
# RTXC interrupt enable/disable utilities
#
$(OBJS)/lock.$(OBJEXT): $(KERNELDIR)/lock.S
	$(CC) $(CCFLAGS) $(KERNELDIR)/lock.S -o $*.o

#
# RTXC init
#
$(OBJS)/rtxcinit.$(OBJEXT): $(KERNELDIR)/rtxcinit.c $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(KERNELDIR)/rtxcinit.c -o $*.o

#
# RTXC C interface
#
$(OBJS)/rtxcapi.$(OBJEXT): $(KERNELDIR)/rtxcapi.c $(KERNELDIR)/rtxcapi.h \
			$(KERNELDIR)/rtxstruc.h $(KERNELDIR)/rtxcarg.h \
			$(KERNELDIR)/typedef.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(KERNELDIR)/rtxcapi.c -o $*.o

#
# RTXC core file
#
$(OBJS)/rtxc.$(OBJEXT): $(KERNELDIR)/rtxc.c $(KERNELDIR)/rtxcapi.h \
			$(KERNELDIR)/rtxstruc.h $(KERNELDIR)/rtxcarg.h $(KERNELDIR)/typedef.h \
			$(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(KERNELDIR)/rtxc.c -o $*.o


######################################################
# DRIVER section

#
# Library files
#

$(LIBS)/lib$(LIB_DRV).a: $(OBJS)/cclock.$(OBJEXT) $(OBJS)/cpart.$(OBJEXT) $(OBJS)/cqueue.$(OBJEXT) \
			$(OBJS)/cres.$(OBJEXT) $(OBJS)/csema.$(OBJEXT) $(OBJS)/flashapi.$(OBJEXT) \
			$(OBJS)/extapi.$(OBJEXT) $(OBJS)/cvtdate.$(OBJEXT) \
			$(OBJS)/rtxcbug.$(OBJEXT) $(OBJS)/cbugutil.$(OBJEXT) \
			$(OBJS)/stdio_vsscanf.$(OBJEXT) $(OBJS)/stdio_vsprintf.$(OBJEXT) \
			$(OBJS)/stdio_printf.$(OBJEXT) \
			$(OBJS)/stdio_gets.$(OBJEXT) $(OBJS)/stdio_puts.$(OBJEXT) \
			$(OBJS)/stdio_strtod.$(OBJEXT) \
			$(OBJS)/drv_clk.$(OBJEXT) $(OBJS)/drv_uart.$(OBJEXT) \
			$(OBJS)/drv_can.$(OBJEXT) $(OBJS)/drv_spi.$(OBJEXT) \
			$(OBJS)/drv_twi.$(OBJEXT) $(OBJS)/drv_usb.$(OBJEXT) \
			$(OBJS)/drv_adc.$(OBJEXT) $(OBJS)/drv_dio.$(OBJEXT) \
			$(OBJS)/XYZaccel.$(OBJEXT) \
			$(OBJS)/stdio_strtol.$(OBJEXT) $(OBJS)/stdio_strtoll.$(OBJEXT)
	-$(RM) $(LIBS)/lib$(LIB_DRV).a
	$(AR) $(ARFLAGS) $(LIBS)/lib$(LIB_DRV).a \
		$(OBJS)/cclock.$(OBJEXT) $(OBJS)/cpart.$(OBJEXT) $(OBJS)/cqueue.$(OBJEXT) \
		$(OBJS)/cres.$(OBJEXT) $(OBJS)/csema.$(OBJEXT) \
		$(OBJS)/extapi.$(OBJEXT) $(OBJS)/cvtdate.$(OBJEXT) $(OBJS)/flashapi.$(OBJEXT) \
		$(OBJS)/rtxcbug.$(OBJEXT) $(OBJS)/cbugutil.$(OBJEXT) \
		$(OBJS)/stdio_vsscanf.$(OBJEXT) $(OBJS)/stdio_vsprintf.$(OBJEXT) \
		$(OBJS)/stdio_printf.$(OBJEXT) \
		$(OBJS)/stdio_gets.$(OBJEXT) $(OBJS)/stdio_puts.$(OBJEXT) \
		$(OBJS)/stdio_strtod.$(OBJEXT) \
		$(OBJS)/drv_clk.$(OBJEXT) $(OBJS)/drv_uart.$(OBJEXT) \
		$(OBJS)/drv_can.$(OBJEXT) $(OBJS)/drv_spi.$(OBJEXT) \
		$(OBJS)/drv_twi.$(OBJEXT) $(OBJS)/drv_usb.$(OBJEXT) \
		$(OBJS)/drv_adc.$(OBJEXT) $(OBJS)/drv_dio.$(OBJEXT) \
		$(OBJS)/XYZaccel.$(OBJEXT) \
		$(OBJS)/stdio_strtol.$(OBJEXT) $(OBJS)/stdio_strtoll.$(OBJEXT)

#
# Compile basic utilities
#

$(LIBS)/RTXCstartup.boot.o: $(DRIVERDIR)/$(DRVDIR)/RTXCstartup.S
	$(CC) $(CCFLAGS) -DMAKEBOOT $(DRIVERDIR)/$(DRVDIR)/RTXCstartup.S -o $(LIBS)/RTXCstartup.boot.o

$(LIBS)/RTXCstartup.$(DBG).o: $(DRIVERDIR)/$(DRVDIR)/RTXCstartup.S
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/RTXCstartup.S -o $(LIBS)/RTXCstartup.$(DBG).o

$(LIBS)/RTXCutil.$(DBG).o: $(DRIVERDIR)/$(DRVDIR)/RTXCutil.c
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/RTXCutil.c -o $(LIBS)/RTXCutil.$(DBG).o

$(LIBS)/RTXCsys32.$(DBG).o: $(DRIVERDIR)/$(DRVDIR)/RTXCsys32.c
	$(CC) $(CCFLAGS32) $(DRIVERDIR)/$(DRVDIR)/RTXCsys32.c -o $(LIBS)/RTXCsys32.$(DBG).o

$(LIBS)/RTXCstartup_lpc17.$(DBG).o: $(DRIVERDIR)/$(DRVDIR)/RTXCstartup_lpc17.c
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/RTXCstartup_lpc17.c -o $(LIBS)/RTXCstartup_lpc17.$(DBG).o

#
# system tables
#

$(OBJS)/cclock.$(OBJEXT): $(DRIVERDIR)/cclock.c $(DRIVERDIR)/cclock.h $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/cclock.c -o $*.o

$(OBJS)/cpart.$(OBJEXT): $(DRIVERDIR)/cpart.c $(DRIVERDIR)/cpart.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/cpart.c -o $*.o

$(OBJS)/cqueue.$(OBJEXT): $(DRIVERDIR)/cqueue.c $(DRIVERDIR)/cqueue.h $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/cqueue.c -o $*.o

$(OBJS)/cres.$(OBJEXT): $(DRIVERDIR)/cres.c $(DRIVERDIR)/cres.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/cres.c -o $*.o

$(OBJS)/csema.$(OBJEXT): $(DRIVERDIR)/csema.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/csema.c -o $*.o

#
# drivers
#

$(OBJS)/drv_clk.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_clk.c $(DRIVERDIR)/cclock.h $(DRIVERDIR)/csema.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_clk.c -o $*.o

$(OBJS)/drv_uart.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_uart.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                            $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_uart.c -o $*.o

$(OBJS)/drv_spi.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_spi.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_spi.c -o $*.o

$(OBJS)/drv_twi.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_twi.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_twi.c -o $*.o

$(OBJS)/drv_usb.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_usb.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_usb.c -o $*.o

$(OBJS)/drv_adc.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_adc.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_adc.c -o $*.o

$(OBJS)/drv_can.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_can.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_can.c -o $*.o

$(OBJS)/drv_dio.$(OBJEXT): $(DRIVERDIR)/$(DRVDIR)/drv_dio.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
                           $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(KERNELDIR)/rtxcapi.h $(KERNELDIR)/rtxcopts.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/$(DRVDIR)/drv_dio.c -o $*.o

$(OBJS)/XYZaccel.$(OBJEXT): $(DRIVERDIR)/XYZaccel.c
	$(CC) $(CCFLAGS) $(DRIVERDIR)/XYZaccel.c -o $*.o


#
# rtxcbug
#

$(OBJS)/rtxcbug.$(OBJEXT): $(DRIVERDIR)/rtxcbug.c $(DRIVERDIR)/csema.h $(DRIVERDIR)/cres.h \
			$(DRIVERDIR)/cqueue.h $(DRIVERDIR)/rtxcbug.h \
			$(KERNELDIR)/enable.h $(DRIVERDIR)/cvtdate.h $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/rtxcbug.c -o $*.o

$(OBJS)/cbugutil.$(OBJEXT): $(DRIVERDIR)/cbugutil.c $(DRIVERDIR)/rtxcbug.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/cbugutil.c -o $*.o

#
# utilities
#

$(OBJS)/flashapi.$(OBJEXT): $(DRIVERDIR)/flashapi.c $(KERNELDIR)/rtxcapi.h $(ASS_INC) $(KERNELDIR)/enable.h \
			$(DRIVERDIR)/cclock.h $(DRIVERDIR)/csema.h $(DRIVERDIR)/cqueue.h \
		                  $(DRIVERDIR)/extapi.h assign.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/flashapi.c -o $*.o

$(OBJS)/cvtdate.$(OBJEXT): $(DRIVERDIR)/cvtdate.c $(DRIVERDIR)/cvtdate.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/cvtdate.c -o $*.o

$(OBJS)/extapi.$(OBJEXT): $(DRIVERDIR)/extapi.c $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN) $(DRIVERDIR)/cvtdate.h \
			$(DRIVERDIR)/csema.h $(DRIVERDIR)/cres.h $(DRIVERDIR)/cqueue.h \
			$(DRIVERDIR)/rtxcbug.h $(KERNELDIR)/enable.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/extapi.c -o $*.o

#
# stdio_console
#

$(OBJS)/stdio_vsscanf.$(OBJEXT): $(DRIVERDIR)/stdio_vsscanf.c $(DRIVERDIR)/stdio_console.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_vsscanf.c -o $*.o

$(OBJS)/stdio_vsprintf.$(OBJEXT): $(DRIVERDIR)/stdio_vsprintf.c $(DRIVERDIR)/stdio_console.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_vsprintf.c -o $*.o

$(OBJS)/stdio_printf.$(OBJEXT): $(DRIVERDIR)/stdio_printf.c $(DRIVERDIR)/stdio_console.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_printf.c -o $*.o

$(OBJS)/stdio_gets.$(OBJEXT): $(DRIVERDIR)/stdio_gets.c $(DRIVERDIR)/stdio_console.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_gets.c -o $*.o

$(OBJS)/stdio_puts.$(OBJEXT): $(DRIVERDIR)/stdio_puts.c $(DRIVERDIR)/stdio_console.h
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_puts.c -o $*.o

$(OBJS)/stdio_strtol.$(OBJEXT): $(DRIVERDIR)/stdio_strtol.c $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_strtol.c -o $*.o

$(OBJS)/stdio_strtoll.$(OBJEXT): $(DRIVERDIR)/stdio_strtoll.c $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_strtoll.c -o $*.o

$(OBJS)/stdio_strtod.$(OBJEXT): $(DRIVERDIR)/stdio_strtod.c $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) $(DRIVERDIR)/stdio_strtod.c -o $*.o


######################################################
# build local binary files

$(TASK).bin: $(OBJS)/$(TASK).elf
	$(OC) --output-target=binary $(OBJS)/$(TASK).elf $(TASK).bin
	$(SZ) $(OBJS)/$(TASK).elf

$(TASK).srec: $(OBJS)/$(TASK).elf
	$(OC) --output-target=srec $(OCFLAGS) $(OBJS)/$(TASK).elf $(TASK).srec
	$(SZ) $(OBJS)/$(TASK).elf

$(OBJS)/$(TASK).elf: $(OBJS)/main.$(OBJEXT) $(OBJS)/tasks.$(OBJEXT) \
			$(STARTUP) \
			$(LIBS)/lib$(LIB_RTXC).a \
			$(LIBS)/lib$(LIB_DRV).a \
			Makefile
	$(LK) $(LDFLAGS) $(MAP) -T $(LD_FILE) -o $(OBJS)/$(TASK).elf \
			$(STARTUP) \
			$(OBJS)/main.$(OBJEXT) $(OBJS)/tasks.$(OBJEXT) \
			$(LDLIBS)

$(TASK).dump: $(OBJS)/$(TASK).elf
	$(DP) -sSd $(OBJS)/$(TASK).elf > $(TASK).dump

boot.bin: $(OBJS)/boot.elf
	$(OC) --output-target=binary $(OBJS)/boot.elf boot.bin
	$(SZ) $(OBJS)/boot.elf

$(OBJS)/boot.elf: $(LIBS)/RTXCstartup.boot.o \
			$(LIBS)/RTXCsys32.PROD.o \
			$(LIBS)/RTXCutil.PROD.o \
			Makefile
	$(LK) $(LDFLAGS) -T $(LD_FILE) -o $(OBJS)/boot.elf \
			$(LIBS)/RTXCstartup.boot.o \
			$(LIBS)/RTXCsys32.PROD.o \
			$(LIBS)/RTXCutil.PROD.o

######################################################
# Compile user tasks

$(OBJS)/main.$(OBJEXT): main.c $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) main.c -o $*.o

$(OBJS)/tasks.$(OBJEXT): tasks.c $(DRIVERDIR)/extapi.h $(INCLUDE_ASSIGN)
	$(CC) $(CCFLAGS) tasks.c -o $*.o

