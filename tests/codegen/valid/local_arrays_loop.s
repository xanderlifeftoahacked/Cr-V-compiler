.text
.globl main
main:
  addi sp, sp, -32
  sw s0, 28(sp)
  sw ra, 24(sp)
  addi s0, sp, 32
  li a0, 1
  addi t0, s0, -24
  sw a0, 0(t0)
  li a0, 2
  addi t0, s0, -20
  sw a0, 0(t0)
  li a0, 3
  addi t0, s0, -16
  sw a0, 0(t0)
  li a0, 4
  addi t0, s0, -12
  sw a0, 0(t0)
  li a0, 0
  addi t0, s0, -28
  sw a0, 0(t0)
  li a0, 0
  addi t0, s0, -32
  sw a0, 0(t0)
main_while_head_0:
  addi a0, s0, -28
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 4
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, t0, a0
  beqz a0, main_while_end_1
  addi a0, s0, -24
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
  lw a0, 0(a0)
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -24
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
  lw a0, 0(a0)
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -32
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -32
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -24
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
  lw a0, 0(a0)
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -28
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
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
  addi a0, s0, -32
  lw a0, 0(a0)
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 32
  li a7, 10
  ecall
