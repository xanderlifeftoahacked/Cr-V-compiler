.text
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 42
  addi sp, sp, -4
  sw a0, 0(sp)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call rars_print_int
  li a0, 10
  addi sp, sp, -4
  sw a0, 0(sp)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call rars_print_char
  li a0, 0
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  call rars_exit2
