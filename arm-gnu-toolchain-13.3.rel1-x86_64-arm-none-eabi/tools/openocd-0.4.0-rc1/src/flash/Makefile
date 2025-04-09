TARGET=libflash.a
OBJEXT=o
SRCEXT=c

OBJECTS = common.$(OBJEXT) \
mflash.$(OBJEXT) \
nand\arm_io.$(OBJEXT) \
nand\at91sam9.$(OBJEXT) \
nand\core.$(OBJEXT) \
nand\davinci.$(OBJEXT) \
nand\driver.$(OBJEXT) \
nand\ecc.$(OBJEXT) \
nand\ecc_kw.$(OBJEXT) \
nand\fileio.$(OBJEXT) \
nand\lpc3180.$(OBJEXT) \
nand\mx3.$(OBJEXT) \
nand\nonce.$(OBJEXT) \
nand\orion.$(OBJEXT) \
nand\s3c2410.$(OBJEXT) \
nand\s3c2412.$(OBJEXT) \
nand\s3c2440.$(OBJEXT) \
nand\s3c2443.$(OBJEXT) \
nand\s3c24xx.$(OBJEXT) \
nand\tcl.$(OBJEXT) \
nor\aduc702x.$(OBJEXT) \
nor\at91sam3.$(OBJEXT) \
nor\at91sam7.$(OBJEXT) \
nor\avrf.$(OBJEXT) \
nor\cfi.$(OBJEXT) \
nor\core.$(OBJEXT) \
nor\drivers.$(OBJEXT) \
nor\ecos.$(OBJEXT) \
nor\faux.$(OBJEXT) \
nor\lpc2000.$(OBJEXT) \
nor\lpc288x.$(OBJEXT) \
nor\lpc2900.$(OBJEXT) \
nor\non_cfi.$(OBJEXT) \
nor\ocl.$(OBJEXT) \
nor\pic32mx.$(OBJEXT) \
nor\stellaris.$(OBJEXT) \
nor\stm32x.$(OBJEXT) \
nor\str7x.$(OBJEXT) \
nor\str9x.$(OBJEXT)\
nor\str9xpec.$(OBJEXT) \
nor\tcl.$(OBJEXT) \
nor\tms470.$(OBJEXT)

SOURCES = common.$(SRCEXT) \
mflash.$(SRCEXT) \
nand\arm_io.$(SRCEXT) \
nand\at91sam9.$(SRCEXT) \
nand\core.$(SRCEXT) \
nand\davinci.$(SRCEXT) \
nand\driver.$(SRCEXT) \
nand\ecc.$(SRCEXT) \
nand\ecc_kw.$(SRCEXT) \
nand\fileio.$(SRCEXT) \
nand\lpc3180.$(SRCEXT) \
nand\mx3.$(SRCEXT) \
nand\nonce.$(SRCEXT) \
nand\orion.$(SRCEXT) \
nand\s3c2410.$(SRCEXT) \
nand\s3c2412.$(SRCEXT) \
nand\s3c2440.$(SRCEXT) \
nand\s3c2443.$(SRCEXT) \
nand\s3c24xx.$(SRCEXT) \
nand\tcl.$(SRCEXT) \
nor\aduc702x.$(SRCEXT) \
nor\at91sam3.$(SRCEXT) \
nor\at91sam7.$(SRCEXT) \
nor\avrf.$(SRCEXT) \
nor\cfi.$(SRCEXT) \
nor\core.$(SRCEXT) \
nor\drivers.$(SRCEXT) \
nor\ecos.$(SRCEXT) \
nor\faux.$(SRCEXT) \
nor\lpc2000.$(SRCEXT) \
nor\lpc288x.$(SRCEXT) \
nor\lpc2900.$(SRCEXT) \
nor\non_cfi.$(SRCEXT) \
nor\ocl.$(SRCEXT) \
nor\pic32mx.$(SRCEXT) \
nor\stellaris.$(SRCEXT) \
nor\stm32x.$(SRCEXT) \
nor\str7x.$(SRCEXT) \
nor\str9x.$(SRCEXT)\
nor\str9xpec.$(SRCEXT) \
nor\tcl.$(SRCEXT) \
nor\tms470.$(SRCEXT)


CC=\mingw\bin\gcc
AR=\mingw\bin\ar
CFLAGS=-O2 -DHAVE_CONFIG_H -I. -I.. -I..\.. -I..\target -I..\helper -I..\jtag -I..\jtag\drivers

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

