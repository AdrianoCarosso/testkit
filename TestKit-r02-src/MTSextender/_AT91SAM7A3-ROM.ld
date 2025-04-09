
/* Memory Definitions */

MEMORY
{
  CODE (rx) : ORIGIN = 0x00000000, LENGTH = 256k
  DATA (rw) : ORIGIN = 0x00200000, LENGTH = 32k
  STACK (rw) : ORIGIN = 0x00208000,LENGTH = 0
}


/* Section Definitions */

SECTIONS
{
  /* first section is .text which is used for code */
  /* . = 0x0000000; */
  .text : {
        *RTXCstartup*.o (.text)
  } >CODE

  .text :
  {
    *(.text)                   /* remaining code */

    *(.glue_7t) *(.glue_7)

  } >CODE =0

  . = ALIGN(4);

  /* .rodata section which is used for read-only data (constants) */

  .rodata :
  {
    *(.rodata*)
  } >CODE

  . = ALIGN(4);

  _etext = . ;
  PROVIDE (etext = .);

  /* .data section which is used for initialized data */

  .data : AT (_etext)
  {
    _data = . ;
    *(.data)
    SORT(CONSTRUCTORS)
  } >DATA
  . = ALIGN(4);

  _edata = . ;
   PROVIDE (edata = .);

  /* .bss section which is used for uninitialized data */

  .bss ADDR(.data) + SIZEOF(.data) :
  {
    __bss_start = . ;
    __bss_start__ = . ;
    *(.bss)
    *(COMMON)
    
    . = ALIGN(4);
    
    __bss_end = . ;
    __bss_end__ = . ;
/*    _end = .;         */

    __noinit_start = . ;
    *(.noinit)
    __noinit_end = . ;

/*    _end = .; */
/*    end = .; */

  } >DATA

  /* top of ram */
  
  .int_data :
  {
    *(.internal_ram_top)
  } >STACK

/*  PROVIDE (ram_end = .); */
  ram_end = ADDR(.int_data) ;
}

