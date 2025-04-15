//------------------------------------------------------------------------------
// lock.S

#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5<<3)

//------------------------------------------------------------------------------
// enable interrupts

        .text
        .align  0
        .code   16
        .thumb_func
        .globl  _enable
        .func   _enable
_enable:

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        mov r0, #0
        msr basepri, r0

#else   // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      pc
        nop

        .code   32
        .arm
        .align  0

_enable32:
        mrs     r0,cpsr
        bic     r0,r0,#0x80
        msr     cpsr_c,r0

#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      lr

        .endfunc

//------------------------------------------------------------------------------
// disable interrupts

        .text
        .align  0

        .code   16
        .thumb_func
        .globl  _disable
        .func   _disable
_disable:

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
        msr basepri, r0

#else   // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      pc
        nop

        .code   32
        .arm
        .align  0

_disable32:
        mrs     r0,cpsr
        orr     r0,r0,#0x80
        msr     cpsr_c,r0

#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      lr

        .endfunc

//------------------------------------------------------------------------------
// get interrupt status

        .text
        .align  0

        .code   16
        .thumb_func
        .globl  _pushccr
        .func   _pushccr
_pushccr:

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        mrs r0, basepri

#else   // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      pc
        nop

        .code   32
        .arm
        .align  0

_pushccr32:
        mrs     r0,cpsr

#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      lr

        .endfunc

//------------------------------------------------------------------------------
// set interrupt status

        .text
        .align  0
        .code   16
        .thumb_func
        .globl  _popccr
        .func   _popccr
_popccr:

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        msr basepri, r0

#else   // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      pc
        nop

        .code   32
        .arm
        .align  0

_popccr32:
        msr     cpsr_c,r0

#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

        bx      lr

        .endfunc

        .end

