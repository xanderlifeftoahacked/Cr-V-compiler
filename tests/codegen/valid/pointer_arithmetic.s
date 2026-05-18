.text
.globl main
main:
  addi sp, sp, -48
  sw s0, 44(sp)
  sw ra, 40(sp)
  addi s0, sp, 48
  li a0, 10
  addi t0, s0, -24
  sw a0, 0(t0)
  li a0, 20
  addi t0, s0, -20
  sw a0, 0(t0)
  li a0, 30
  addi t0, s0, -16
  sw a0, 0(t0)
  li a0, 40
  addi t0, s0, -12
  sw a0, 0(t0)
  addi a0, s0, -24
  addi t0, s0, -28
  sw a0, 0(t0)
  addi a0, s0, -28
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  lw t0, 0(sp)
  addi sp, sp, 4
  li t1, 4
  mul a0, a0, t1
  add a0, t0, a0
  addi t0, s0, -32
  sw a0, 0(t0)
  addi a0, s0, -32
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  sub a0, t0, a0
  li t1, 4
  div a0, a0, t1
  addi t0, s0, -36
  sw a0, 0(t0)
  addi a0, s0, -28
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  lw t0, 0(sp)
  addi sp, sp, 4
  li t1, 4
  mul a0, a0, t1
  add a0, t0, a0
  lw a0, 0(a0)
  addi t0, s0, -40
  sw a0, 0(t0)
  addi a0, s0, -36
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -40
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 48
  li a7, 10
  ecall
