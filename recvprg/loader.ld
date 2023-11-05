SECTIONS {

.restore_irq_overlay : { 

  . = 0xCC00;
  *(.restore_irq_overlay .restore_irq_overlay.*)

} >c_readonly

}
