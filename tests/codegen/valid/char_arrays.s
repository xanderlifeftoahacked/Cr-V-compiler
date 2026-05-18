.text
.globl main
main:
  addi sp, sp, -32
  sw s0, 28(sp)
  sw ra, 24(sp)
  addi s0, sp, 32
  li a0, 97
  addi t0, s0, -12
  sb a0, 0(t0)
  li a0, 98
  addi t0, s0, -11
  sb a0, 0(t0)
  li a0, 99
  addi t0, s0, -10
  sb a0, 0(t0)
  li a0, 100
  addi t0, s0, -9
  sb a0, 0(t0)
  li a0, 0
  addi t0, s0, -16
  sw a0, 0(t0)
  li a0, 0
  addi t0, s0, -20
  sw a0, 0(t0)
main_while_head_0:
  addi a0, s0, -16
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 4
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, t0, a0
  beqz a0, main_while_end_1
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -20
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -16
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lb a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -16
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -16
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  j main_while_head_0
main_while_end_1:
  addi a0, s0, -12
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 65
  lw t0, 0(sp)
  addi sp, sp, 4
  sb a0, 0(t0)
  addi a0, s0, -20
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lb a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 32
  li a7, 10
  ecall
