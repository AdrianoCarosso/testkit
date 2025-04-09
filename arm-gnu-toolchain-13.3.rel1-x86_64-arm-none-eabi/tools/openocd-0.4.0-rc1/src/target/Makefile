TARGET=libtarget.a
OBJEXT=o
SRCEXT=c

# oocd_trace.o
OBJECTS = target.$(OBJEXT) register.$(OBJEXT) avrt.$(OBJEXT) \
	breakpoints.$(OBJEXT) armv4_5.$(OBJEXT) embeddedice.$(OBJEXT) \
	etm.$(OBJEXT) arm7tdmi.$(OBJEXT) arm9tdmi.$(OBJEXT) \
	arm_jtag.$(OBJEXT) arm7_9_common.$(OBJEXT) algorithm.$(OBJEXT) \
	arm920t.$(OBJEXT) arm720t.$(OBJEXT) armv4_5_mmu.$(OBJEXT) \
	armv4_5_cache.$(OBJEXT) arm_disassembler.$(OBJEXT) \
	arm966e.$(OBJEXT) arm926ejs.$(OBJEXT) feroceon.$(OBJEXT) \
	etb.$(OBJEXT) xscale.$(OBJEXT) arm_simulator.$(OBJEXT) \
	image.$(OBJEXT) armv7m.$(OBJEXT) armv7a.$(OBJEXT) cortex_m3.$(OBJEXT) \
	cortex_a8.$(OBJEXT) etm_dummy.$(OBJEXT) \
	target_request.$(OBJEXT) trace.$(OBJEXT) arm11.$(OBJEXT) \
	arm11_dbgtap.$(OBJEXT) mips_m4k.$(OBJEXT) mips_ejtag.$(OBJEXT) \
	mips32.$(OBJEXT) mips32_dmaacc.$(OBJEXT) mips32_pracc.$(OBJEXT) \
	fa526.$(OBJEXT) dsp563xx.$(OBJEXT) dsp563xx_once.$(OBJEXT) arm_adi_v5.$(OBJEXT) \
	arm_dpm.$(OBJEXT) testee.$(OBJEXT) arm_semihosting.$(OBJEXT)

# oocd_trace.c
SOURCES = target.$(SRCEXT) register.$(SRCEXT) avrt.$(SRCEXT) \
	breakpoints.$(SRCEXT) armv4_5.$(SRCEXT) embeddedice.$(SRCEXT) \
	etm.$(SRCEXT) arm7tdmi.$(SRCEXT) arm9tdmi.$(SRCEXT) \
	arm_jtag.$(SRCEXT) arm7_9_common.$(SRCEXT) algorithm.$(SRCEXT) \
	arm920t.$(SRCEXT) arm720t.$(SRCEXT) armv4_5_mmu.$(SRCEXT) \
	armv4_5_cache.$(SRCEXT) arm_disassembler.$(SRCEXT) \
	arm966e.$(SRCEXT) arm926ejs.$(SRCEXT) feroceon.$(SRCEXT) \
	etb.$(SRCEXT) xscale.$(SRCEXT) arm_simulator.$(SRCEXT) \
	image.$(SRCEXT) armv7m.$(SRCEXT) armv7a.$(SRCEXT) cortex_m3.$(SRCEXT) \
	cortex_a8.$(SRCEXT) etm_dummy.$(SRCEXT) \
	target_request.$(SRCEXT) trace.$(SRCEXT) arm11.$(SRCEXT) \
	arm11_dbgtap.$(SRCEXT) mips_m4k.$(SRCEXT) mips_ejtag.$(SRCEXT) \
	mips32.$(SRCEXT) mips32_dmaacc.$(SRCEXT) mips32_pracc.$(SRCEXT) \
	fa526.$(SRCEXT) dsp563xx.$(SRCEXT) dsp563xx_once.$(SRCEXT) arm_adi_v5.$(SRCEXT) \
	arm_dpm.$(SRCEXT) testee.$(SRCEXT) arm_semihosting.$(SRCEXT)

CC=\mingw\bin\gcc
AR=\mingw\bin\ar
CFLAGS=-O2 -DHAVE_CONFIG_H -I. -I.. -I..\.. -I..\target -I..\helper -I..\jtag -I..\flash

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

