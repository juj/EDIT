

SECTIONS {
    /* BASIC program that jumps to _start. */
    .basic_header : { *(.basic_header) }

/* These scripts place C read-only sections in "c_readonly" and C writeable sections
 * in "c_writeable", with initializers LMA in "c_readonly". ZP VMAs are in "zp"; ZP LMAs are
 * like regular sections. */
  INCLUDE zp.ld
  INCLUDE text.ld
  INCLUDE rodata.ld
  INCLUDE data.ld
  INCLUDE bss.ld
  INCLUDE noinit.ld

/*
 RESTORE IRQ key handler resides at the highest area of memory
*/

  .restore_irq_overlay : { 
    . = 0xCC00;
    *(.restore_irq_overlay .restore_irq_overlay.*)
  } >c_readonly

}


/********
SECTIONS {

.restore_irq_overlay : { 

  . = 0xCC00;
  *(.restore_irq_overlay .restore_irq_overlay.*)

} >c_readonly

}

*******/