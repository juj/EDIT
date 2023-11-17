  .zeropage __rc2
  .zeropage __rc3
  .zeropage __rc4

; # void __memset(char *ptr, char value, size_t num);
; # ptr: (__rc2, __rc3), num: (X, __rc4), value: (A)
.section  .text.__memset,"ax",@progbits
.globl __memset
.type __memset, @function
__memset:
  INX            ;# Pre-increment low byte of num to shift it up by one so we can detect wraparounds via DEX instruction easier
  INC __rc4      ;# Shift high byte of num up by one as well for the same effect
  LDY #0         ;# Initialize write offset pointer to zero

__memset_write_byte:
  DEX            ;# Dec low byte of num. If result is zero, the real value has wrapped around (since we shifted up by 1 above)
  BNE 1f         ;# If X is nonzero after decrement, no wraparound yet, so skip past decrementing high byte
  DEC __rc4      ;# Carry decrement to high byte.
  BNE 1f         ;# Since we pre-shifted above, when __rc4 reaches zero here, we are done
  RTS            ;# All done!
1:

  STA (__rc2),y  ;# Fill one byte to the destination pointer

  INY            ;# Increment write offset pointer
  BNE __memset_write_byte ;# If write offset pointer didn't wrap, skip incrementing high byte
  INC __rc3      ;# On wraparound of offset ptr, carry increment to high byte of destination address
  JMP __memset_write_byte
