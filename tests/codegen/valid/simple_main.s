.text
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 1
  sw a0, -12(s0)
  lw a0, -12(s0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  sw a0, -12(s0)
  lw a0, -12(s0)
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  li a7, 10
  ecall

