/*
 RESTORE IRQ key handler resides at the highest area of memory
*/

SECTIONS
{
  .restore_irq_overlay 0xCC00 : { 
    *(.restore_irq_overlay .restore_irq_overlay.*)
  } >c_readonly
}
INSERT AFTER .noinit;






/*
__stack = 0xCC00;
*/





/*
__restore_overlay_start = .;
.restore_irq_overlay 0xCC00 : AT ( ADDR(.noinit) + SIZEOF(.noinit)) { 
  __restore_overlay_dst = .;
  *(.restore_irq_overlay .restore_irq_overlay.*)
  __restore_overlay_dst_end = .;
} >c_readonly
*/

/* XXX IF USING CUSTOM FONT
.restore_irq_overlay 0xC800 : { 
  *(.restore_irq_overlay .restore_irq_overlay.*)
} >c_readonly
*/

/*
.restore_irq_overlay 0xCC00 : { 
  *(.restore_irq_overlay .restore_irq_overlay.*)
} >c_readonly

*/
