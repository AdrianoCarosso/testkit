
if OOCD_TRACE
OOCD_TRACE_FILES = oocd_trace.c
else
OOCD_TRACE_FILES =
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src

BIN2C		= $(top_builddir)/src/helper/bin2char$(EXEEXT_FOR_BUILD)

DEBUG_HANDLER	= $(srcdir)/xscale/debug_handler.bin
EXTRA_DIST = \
	startup.tcl \
	$(DEBUG_HANDLER)

DEBUG_HEADER	= xscale_debug.h
BUILT_SOURCES = $(DEBUG_HEADER)
CLEANFILES = $(DEBUG_HEADER)

$(DEBUG_HEADER): $(BIN2C) $(DEBUG_HANDLER)
	$(BIN2C) < $(DEBUG_HANDLER) xscale_debug_handler > xscale_debug.h

METASOURCES = AUTO
noinst_LTLIBRARIES = libtarget.la
libtarget_la_SOURCES = \
	$(TARGET_CORE_SRC) \
	$(ARM_DEBUG_SRC) \
	$(ARMV4_5_SRC) \
	$(ARMV6_SRC) \
	$(ARMV7_SRC) \
	$(ARM_MISC_SRC) \
	$(MIPS32_SRC) \
	avrt.c \
	dsp563xx.c \
	dsp563xx_once.c

TARGET_CORE_SRC = \
	algorithm.c \
	register.c \
	image.c \
	breakpoints.c \
	target.c \
	target_request.c \
	testee.c

ARMV4_5_SRC = \
	armv4_5.c \
	armv4_5_mmu.c \
	armv4_5_cache.c \
	$(ARM7_9_SRC)

ARM7_9_SRC = \
	arm7_9_common.c \
	arm7tdmi.c \
	arm720t.c \
	arm9tdmi.c \
	arm920t.c \
	arm966e.c \
	arm926ejs.c \
	feroceon.c

ARM_MISC_SRC = \
	fa526.c \
	xscale.c

ARMV6_SRC = \
	arm11.c \
	arm11_dbgtap.c

ARMV7_SRC = \
	armv7m.c \
	cortex_m3.c \
	armv7a.c \
	cortex_a8.c

ARM_DEBUG_SRC = \
	arm_dpm.c \
	arm_jtag.c \
	arm_disassembler.c \
	arm_simulator.c \
	arm_semihosting.c \
	arm_adi_v5.c \
	embeddedice.c \
	trace.c \
	etb.c \
	etm.c \
	$(OOCD_TRACE_FILES) \
	etm_dummy.c

MIPS32_SRC = \
	mips32.c \
	mips_m4k.c \
	mips32_pracc.c \
	mips32_dmaacc.c \
	mips_ejtag.c


noinst_HEADERS = \
	algorithm.h \
	arm.h \
	arm_dpm.h \
	arm_jtag.h \
	arm_adi_v5.h \
	arm_disassembler.h \
	arm_opcodes.h \
	arm_simulator.h \
	arm_semihosting.h \
	arm7_9_common.h \
	arm7tdmi.h \
	arm720t.h \
	arm9tdmi.h \
	arm920t.h \
	arm926ejs.h \
	arm966e.h \
	arm11.h \
	arm11_dbgtap.h \
	armv4_5.h \
	armv4_5_mmu.h \
	armv4_5_cache.h \
	armv7a.h \
	armv7m.h \
	avrt.h \
	dsp563xx.h \
	dsp563xx_once.h \
	breakpoints.h \
	cortex_m3.h \
	cortex_a8.h \
	embeddedice.h \
	etb.h \
	etm.h \
	etm_dummy.h \
	image.h \
	mips32.h \
	mips_m4k.h \
	mips_ejtag.h \
	mips32_pracc.h \
	mips32_dmaacc.h \
	oocd_trace.h \
	register.h \
	target.h \
	target_type.h \
	trace.h \
	target_request.h \
	trace.h \
	xscale.h \
	xscale_debug.h

#nobase_dist_pkglib_DATA =
#nobase_dist_pkglib_DATA += ecos/at91eb40a.elf

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
