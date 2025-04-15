//------------------------------------------------------------------------------
// RTXCstartup.S
//------------------------------------------------------------------------------

	.equ AIC_IVR,         (256)
	.equ AIC_FVR,         (260) 
	.equ AIC_EOICR,       (304)
	.equ AT91C_BASE_AIC,  (0xFFFFF000)

//------------------------------------------------------------------------------
//- Top of Stack Definition
//-------------------------
//- Interrupt, Supervisor and System Stacks are located at the top of internal
//- memory.
//------------------------------------------------------------------------------

	.EQU		IRQ_STACK_SIZE,    ((2*4)+4)
	.EQU		SVC_STACK_SIZE,    ((2*4)+4)
	.EQU		SYSKERNEL_STACK_SIZE, (640)

        .EQU		ARM_MODE_FIQ,       0x11
        .EQU		ARM_MODE_IRQ,       0x12
        .EQU		ARM_MODE_SVC,       0x13
        .EQU		ARM_MODE_SYS,       0x1f

        .EQU		I_BIT,              0x80
        .EQU		F_BIT,              0x40
        .EQU		T_BIT,              0x20

//------------------------------------------------------------------------------
// Section Definition
// .internal_ram_top 	Top_Stack: used by the cstartup for vector initalisation
//			management defined by ld and affect from ldscript
//------------------------------------------------------------------------------
	.section 	.internal_ram_top
	.code   32
	.align 	0
	.global	Top_Stack
Top_Stack:
	
//------------------------------------------------------------------------------
// Area Definition
//------------------------------------------------------------------------------
// .text is used instead of .section .text so it works with arm-aout too.

        .section 	.reset
        .text
        .global _startup
        .func   _startup
_startup:
reset: 

//------------------------------------------------------------------------------
//- Exception vectors
//--------------------
//- These vectors can be read at address 0 or at RAM address
//- They ABSOLUTELY requires to be in relative addresssing mode in order to
//- guarantee a valid jump. For the moment, all are just looping.
//- If an exception occurs before remap, this would result in an infinite loop.
//- To ensure if a exeption occurs before start application to infinite loop.
//------------------------------------------------------------------------------

// 0x00 Reset handler ----------------------------------------------------------
                B           InitReset           /* 0x00 Reset handler */

// 0x04 Undefined Instruction --------------------------------------------------
#if defined(CBUG) && !defined(MAKEBOOT)
                B           DataUndef           /* 0x04 Undefined Instruction */
#else // CBUG
undefvec:
                B           undefvec            /* 0x04 Undefined Instruction */
#endif // CBUG

// 0x08 Software Interrupt -----------------------------------------------------
#if defined(MAKEBOOT)
swivec:
                B           swivec              /* 0x08 Software Interrupt */
#else // MAKEBOOT
swivec:
                B           SWI_Handler_Entry   /* 0x08 Software Interrupt */
#endif // MAKEBOOT

// 0x0C Prefetch Abort ---------------------------------------------------------
pabtvec:
                B           pabtvec             /* 0x0C Prefetch Abort */

// 0x10 Data Abort -------------------------------------------------------------
#if defined(CBUG) && !defined(MAKEBOOT)
                B           DataAbort           /* 0x10 Data Abort */
#else // CBUG
dabtvec:
                B           dabtvec             /* 0x10 Data Abort */
#endif // CBUG

// 0x14 reserved ---------------------------------------------------------------
rsvdvec:
                B           rsvdvec             /* 0x14 reserved */

// 0x18 IRQ --------------------------------------------------------------------
#if defined(MAKEBOOT)
irqvec:
                B           irqvec              /* 0x18 IRQ */
#else // MAKEBOOT
irqvec:
                B           IRQ_Handler_Entry   /* 0x18 IRQ */
#endif // MAKEBOOT

// 0x1c FIQ --------------------------------------------------------------------
#if defined(MAKEBOOT)
fiqvec:
                B           fiqvec              /* 0x1c FIQ */
#else // MAKEBOOT
fiqvec:
// No jump, code follows
//                B           FIQ_Handler_Entry

//------------------------------------------------------------------------------
//- Function             : FIQ_Handler_Entry
//- Treatments           : FIQ Controller Interrupt Handler.
//- Called Functions     : AIC_FVR[interrupt]
//------------------------------------------------------------------------------

FIQ_Handler_Entry:

/*- Switch in SVC/User Mode to allow User Stack access for C code 	*/
/* because the FIQ is not yet acknowledged*/

/*- Save and r0 in FIQ_Register */
        mov         r9,r0
        ldr         r0 , [r8, #AIC_FVR]
        msr         CPSR_c,#I_BIT | F_BIT | ARM_MODE_SVC

/*- Save scratch/used registers and LR in User Stack */
        stmfd       sp!, { r1-r3, r12, lr}

/*- Branch to the routine pointed by the AIC_FVR */
        mov         r14, pc
        bx          r0

/*- Restore scratch/used registers and LR from User Stack */
        ldmia       sp!, { r1-r3, r12, lr}

/*- Leave Interrupts disabled and switch back in FIQ mode */
        msr         CPSR_c, #I_BIT | F_BIT | ARM_MODE_FIQ

/*- Restore the R0 ARM_MODE_SVC register */
        mov         r0,r9

/*- Restore the Program Counter using the LR_fiq directly in the PC */
        subs        pc,lr,#4
	.align 0

#endif // MAKEBOOT

//------------------------------------------------------------------------------
// Insert here common static values

        .global Global_RAM_TOP
Global_RAM_TOP:
	.word          Top_Stack

        .global Global_KERNEL_TOP
Global_KERNEL_TOP:
	.word          Top_Stack - IRQ_STACK_SIZE - SVC_STACK_SIZE

        .global Global_IRQ_STACK_SIZE
Global_IRQ_STACK_SIZE:
	.word          IRQ_STACK_SIZE

        .global Global_SVC_STACK_SIZE
Global_SVC_STACK_SIZE:
	.word          SVC_STACK_SIZE

        .global Global_SYSKERNEL_STACK_SIZE
Global_SYSKERNEL_STACK_SIZE:
	.word          SYSKERNEL_STACK_SIZE

//#if defined(CBUG) && !defined(MAKEBOOT)  // _FR_ 13/02/09
#if !defined(MAKEBOOT)
        .global Global_STACK_FILLER
Global_STACK_FILLER:
        .word   0x0ad50ad5
#endif // MAKEBOOT

//------------------------------------------------------------------------------
//- Stack Sizes Definition
//------------------------
//- Interrupt Stack requires 3 words x 8 priority level x 4 bytes when using
//- the vectoring. This assume that the IRQ management.
//- The Interrupt Stack must be adjusted depending on the interrupt handlers.
//- Fast Interrupt not requires stack If in your application it required you must
//- be definehere.
//- The System stack size is not defined and is limited by the free internal
//- SRAM.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//- Setup the stack for each mode
//- at reset we are in supervisor SVC

InitReset:
        ldr     r13,Global_RAM_TOP      /* stack in internal RAM */
        mov     r0,r13

/*- Set up Fast Interrupt Mode and set FIQ Mode Stack*/
        msr     CPSR_c, #ARM_MODE_FIQ | I_BIT | F_BIT
/*- Init the FIQ register*/
        ldr     r8, =AT91C_BASE_AIC

/*- Set up Interrupt Mode and set IRQ Mode Stack*/
        msr     CPSR_c, #ARM_MODE_IRQ | I_BIT | F_BIT
        mov     r13, r0                     /* Init stack IRQ */
        sub     r0, r0, #IRQ_STACK_SIZE
        
/*- Set up Supervisor Mode and set SVC Mode Stack*/
        msr     CPSR_c, #ARM_MODE_SVC | I_BIT | F_BIT
        mov     r13, r0                     /* Init stack SVC */
/*- User stack below SVC and SYSKERNEL */
        sub     r0, r0, #SVC_STACK_SIZE
        ldr     r1, Global_SYSKERNEL_STACK_SIZE
        sub     r0, r0, r1

/*- Set up Supervisor Mode and set SYS Mode Stack*/
        msr     CPSR_c, #ARM_MODE_SYS | I_BIT | F_BIT
        mov     r13, r0                     /* Init stack SYS */

/*- Call AT91F_WATCHDOGinit, we need to init watchdog (if desired) */
        bl      AT91F_WATCHDOGinit

/*- Call AT91F_CLOCKinit, we need to perform next loops at hi speed */
        mov     r0, #0                  /* set default clock: EXT XTAL */
/*****  bl      AT91F_CLOCKinit         <---- old 32 bit style   ****/
        ldr	r1,=AT91F_CLOCKinit

/* next opcode is equivalent to: */
/*      ldr r14,=CLOCKinit_thumb_ret */
        mov     r14, pc
        bx	r1

/* Assembler-specific instruction to switch to Thumb */
        .code   16
        .thumb

CLOCKinit_thumb_ret:
        bx      pc
        nop

        .code   32
        .arm
        .align  0
CLOCKinit_thumb_out:

/*- Relocate .data section (Copy from ROM to RAM) */
        LDR     R1, =_etext
        LDR     R2, =_data
        LDR     R3, =_edata
LoopRel:
        CMP     R2, R3
        LDRLO   R0, [R1], #4
        STRLO   R0, [R2], #4
        BLO     LoopRel

/*- Clear .bss section (Zero init) */
        MOV     R0, #0
        LDR     R1, =__bss_start
        LDR     R2, =__bss_end
LoopZI:
        CMP     R1, R2
        STRLO   R0, [R1], #4
        BLO     LoopZI

/*- Clear STACK, only if stack analysis really desired */
#if defined(CBUG) && !defined(MAKEBOOT)
        LDR     R0, Global_STACK_FILLER
        LDR     R1, =__noinit_end
        LDR     R2, Global_RAM_TOP
LoopSI:
        CMP     R1, R2
        STRLO   R0, [R1], #4
        BLO     LoopSI
#endif // CBUG

/*- Enable FIQ, keeo IRQ disabled and STAY in SYS Mode */
        msr     CPSR_c, #ARM_MODE_SYS | I_BIT

#if defined(MAKEBOOT)
        b       exit
#else // MAKEBOOT
/*- Jump to maintask. It never returns, but just in case it returns to 'exit()' */
	ldr	lr,=exit
	ldr	r0,=maintask
	bx	r0
#endif // MAKEBOOT

        .size   _startup, . - _startup
        .endfunc
		
	
/* "exit" dummy added by mthomas to avoid sbrk write read etc. needed
   by the newlib default "exit" */
        .global exit
        .func   exit
exit:
        b    .
	.size   exit, . - exit
        .endfunc

#if !defined(MAKEBOOT)
//------------------------------------------------------------------------------
//- Manage IRQ exception
//---------------
//- This module uses 2 word in IRQ stack, "frame" in SYS stack, than switch to SVR stack
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//- Function             : IRQ_Handler_Entry
//- Treatments           : IRQ Controller Interrupt Handler.
//- Called Functions     : AIC_IVR[interrupt]
//------------------------------------------------------------------------------
        .global IRQ_Handler_Entry
        .func   IRQ_Handler_Entry

IRQ_Handler_Entry:
/* portSAVE_CONTEXT begin */

        /* Push R0 as we are going to use the register. */
        STMDB   SP!, {R0}

        /* Set R0 to point to the task stack pointer. */
        STMDB   SP, {SP}^
        SUB     SP, SP, #4
        LDMIA	SP!, {R0}

        /* same offset as SWI */
        SUB     LR, LR, #4
        /* Push the return address onto the stack. */
        STMDB	R0!, {LR}

        /* Now we have saved LR we can use it instead of R0. */
        MOV     LR, R0

        /* Pop R0 so we can save it onto the system mode stack. */
        LDMIA	SP!, {R0}

        /* Push all the system mode registers onto the task stack. */
        STMDB   LR,{R0-LR}^
        SUB     LR, LR, #60

        /* Push the SPSR onto the task stack. */
        MRS     R0, SPSR
        STMDB   LR!, {R0}

        /* R0 is the frame pointer */
        MOV     R0, LR

/* portSAVE_CONTEXT end */

/*- Read interrupt vector */
        ldr         r14, =AT91C_BASE_AIC
	ldr         r1 , [r14, #AIC_IVR]
/*- Write in the IVR to support Protect Mode  */
/*- No effect in Normal Mode  */
/*- De-assert the NIRQ and clear the source in Protect Mode */
//	str         r14, [r14, #AIC_IVR]

/*- Keep Interrupt disabled and Switch in SYS Mode */
        msr         CPSR_c, #I_BIT | ARM_MODE_SYS

/* Check current nesting level */
        ldr     r2, =isrcnt
        ldrb    r3, [r2]

        /* only if 'isrcnt' is zero */
        cmp     r3, #0
        ldreq   r13, Global_KERNEL_TOP     /* stack in internal RAM */
        subne   r13, r13, #17*4
        
        add     r3, r3, #1
        strb    r3, [r2]

/*- Enable interrupt and STAY in SYS Mode */
/*- Enabled 21/Aug/2006 */
        msr         CPSR_c, #ARM_MODE_SYS

/*- Branch to the routine pointed by the AIC_IVR  */

/* next opcode is equivalent to: */
/*      ldr r14,=IRQ_thumb_ret */
        mov     r14, pc

        bx      r1

/*- Assembler-specific instruction to switch to Thumb */
        .code   16
        .thumb

IRQ_thumb_ret:
        bx      pc
        nop

        .code   32
        .arm
        .align  0
IRQ_thumb_out:

/*- Disable Interrupt and GO in IRQ mode */
        msr     CPSR_c, #I_BIT | ARM_MODE_IRQ

/* Check current nesting level */
        ldr     r2, =isrcnt
        ldrb    r3, [r2]
        sub     r3, r3, #1
        strb    r3, [r2]

/*- Mark the End of Interrupt on the AIC */
        ldr         r14, =AT91C_BASE_AIC
        str         r14, [r14, #AIC_EOICR]

/* portRESTORE_CONTEXT begin */

        /* R0 is the frame pointer */
        MOV     LR, R0

        /* Get the SPSR from the stack. */
        LDMFD   LR!, {R0}
        MSR     SPSR, R0

        /* Restore all system mode registers for the task. */
        LDMFD   LR, {R0-R14}^
        NOP

        /* Restore the return address. */
        LDR     LR, [LR, #+60]

        /* And return  */
        MOVS    PC, LR

/* portRESTORE_CONTEXT end */

        .size   IRQ_Handler_Entry, . - IRQ_Handler_Entry
        .endfunc
        
//------------------------------------------------------------------------------
// Manage SWI exception
// This module uses 2 words in SVC stack, "frame" in USER stack, than switch to SYSKERNEL stack

        .global SWI_Handler_Entry
        .func   SWI_Handler_Entry

SWI_Handler_Entry:

/* portSAVE_CONTEXT begin */

        /* Push R0 as we are going to use the register. */
        STMDB  SP!, {R0}

        /* Set R0 to point to the task stack pointer. */
        STMDB   SP, {SP}^
        SUB     SP, SP, #4
        LDMIA	SP!, {R0}

        /* Push the return address onto the stack. */
        STMDB	R0!, {LR}

        /* Now we have saved LR we can use it instead of R0. */
        MOV     LR, R0

        /* Pop R0 so we can save it onto the system mode stack. */
        LDMIA	SP!, {R0}

        /* Push all the system mode registers onto the task stack. */
        STMDB   LR,{R0-LR}^
        SUB     LR, LR, #60

        /* Push the SPSR onto the task stack. */
        MRS     R0, SPSR
        STMDB   LR!, {R0}

        /* R0 is the frame pointer */
        MOV     R0, LR

/* portSAVE_CONTEXT end */

/*- Keep interrupt disabled, go in SYS Mode */
        msr     CPSR_c, #I_BIT | ARM_MODE_SYS
        
        ldr     r13, Global_KERNEL_TOP     /* stack in internal RAM */

/* Set current nesting level */
        ldr     r1, =isrcnt
        mov     r2, #1
        strb    r2, [r1]

/*- Enable Interrupt and STAY in SYS Mode */
        msr     CPSR_c, #ARM_MODE_SYS

/* call user SWI routine in SYS mode */

        ldr	r1,=rtxc

/* next opcode is equivalent to: */
/*      ldr r14,=SWI_thumb_ret */
        mov     r14, pc

        bx	r1

/* Assembler-specific instruction to switch to Thumb */
        .code   16
        .thumb

SWI_thumb_ret:
        bx      pc
        nop

        .code   32
        .arm
        .align  0
SWI_thumb_out:

/*- Disable Interrupt and GO in SVC mode */
        msr     CPSR_c, #I_BIT | ARM_MODE_SVC

/* Set current nesting level */
        ldr     r1, =isrcnt
        mov     r2, #0
        strb    r2, [r1]

/* portRESTORE_CONTEXT begin */

        /* R0 is the frame pointer */
        MOV     LR, R0

        /* Get the SPSR from the stack. */
        LDMFD   LR!, {R0}
        MSR     SPSR, R0

        /* Restore all system mode registers for the task. */
        LDMFD   LR, {R0-R14}^
        NOP

        /* Restore the return address. */
        LDR     LR, [LR, #+60]

        /* And return  */
        MOVS    PC, LR

/* portRESTORE_CONTEXT end */

        .size   SWI_Handler_Entry, . - SWI_Handler_Entry
        .endfunc

//---------------------------------------------------------------
// EXCEPTION_VECTORs

        .global AT91F_Default_FIQ_handler
        .func   AT91F_Default_FIQ_handler
AT91F_Default_FIQ_handler:
        b       AT91F_Default_FIQ_handler
        .size   AT91F_Default_FIQ_handler, . - AT91F_Default_FIQ_handler
        .endfunc

        .global AT91F_Default_IRQ_handler
        .func   AT91F_Default_IRQ_handler
AT91F_Default_IRQ_handler:
        b       AT91F_Default_IRQ_handler
        .size   AT91F_Default_IRQ_handler, . - AT91F_Default_IRQ_handler
        .endfunc

        .global AT91F_Spurious_handler
        .func   AT91F_Spurious_handler
AT91F_Spurious_handler:
        b       AT91F_Spurious_handler
        .size   AT91F_Spurious_handler, . - AT91F_Spurious_handler
        .endfunc

#endif // MAKEBOOT

//---------------------------------------------------------------
// Data Abort (only if CBUG enabled)

#if defined(CBUG) && !defined(MAKEBOOT)
DataAbort:
        LDR     R13, =regcopy           // temp for registers
        STMIA   R13,{R0-LR}^

        SUB     R0, R14, #8             // PC of aborted instruction
        MOV     R1, #0                  // Abort type
/*- Disable interrupts, Set up Supervisor Mode and set SYS Mode Stack*/
        MSR     CPSR_c, #ARM_MODE_SYS | I_BIT | F_BIT
//        LDR     r13, Global_RAM_TOP     // stack in internal RAM
        LDR     R13, =__bss_end
        B       AT91F_DataAbort         // User handler
#endif // CBUG

//---------------------------------------------------------------
// Data Undef (only if CBUG enabled)

#if defined(CBUG) && !defined(MAKEBOOT)
DataUndef:
        LDR     R13, =regcopy           // temp for registers
        STMIA   R13,{R0-LR}^

        SUB     R0, R14, #4             // PC of aborted instruction
        MOV     R1, #1                  // Abort type
/*- Disable interrupts, Set up Supervisor Mode and set SYS Mode Stack*/
        MSR     CPSR_c, #ARM_MODE_SYS | I_BIT | F_BIT
//        LDR     r13, Global_RAM_TOP     // stack in internal RAM
        LDR     R13, =__bss_end
        B       AT91F_DataAbort         // User handler
#endif // CBUG

//---------------------------------------------------------------
// Code Upgrade

#if !defined(MAKEBOOT)
        .text
        .code 16
        .thumb_func
        .global CodeUpgrade
        .func   CodeUpgrade
CodeUpgrade:
        bx      pc
        nop
        .code   32                      // enter 32 bit mode

/*- Disable interrupts, Set up Supervisor Mode and set SYS Mode Stack*/
CodeUpgrade32:
        msr     CPSR_c, #ARM_MODE_SYS | I_BIT | F_BIT
        ldr     r13, Global_RAM_TOP     // stack in internal RAM
        
/*- Relocate .ProgramCopy section (Copy from ROM to RAM) */
        LDR     R1, =ProgramCopy        // function ProgramCopy begin
        SUB     R1, R1, #1              // from thumb address
        LDR     R2, =__bss_start
        LDR     R3, =ProgramCopyEnd     // function ProgramCopy end
        SUB     R3, R3, #1              // from thumb address
LoopPrgCpy:
        CMP     R1, R3
        LDRLO   R0, [R1], #4
        STRLO   R0, [R2], #4
        BLO     LoopPrgCpy

        LDR     r13, Global_RAM_TOP     // stack in internal RAM

        LDR     LR, =__bss_start
        ADD     LR, LR, #1
        BX      LR

        .size   CodeUpgrade, . - CodeUpgrade
        .endfunc
#endif // MAKEBOOT

        .end

