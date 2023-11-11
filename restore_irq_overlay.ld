/*
 RESTORE IRQ key handler resides at the highest area of memory
*/

/* Use C800 so that stack ceiling will be at C800, growing down,
   IRQ overlay will be at [C800,CC00[, and [CC00,D000[ will be the graphics memory.
   TODO: HACK We need +16 bytes because of something? __heap_start, or similar? */
SECTIONS
{
  __restore_overlay_start = __heap_start+16; /*ADDR(.noinit) + SIZEOF(.noinit) + 16;*/
  .restore_irq_overlay 0xC800 : AT ( __restore_overlay_start) { 
/*  .restore_irq_overlay 0xC800 : AT ( ADDR(.noinit) + SIZEOF(.noinit) + 16) { */
    __restore_overlay_dst = .;
    *(.restore_irq_overlay .restore_irq_overlay.*)
    __restore_overlay_dst_end = .;
  } >c_readonly
}
INSERT AFTER .noinit;






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
