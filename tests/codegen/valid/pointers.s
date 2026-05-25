.text
.globl first
first:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw a0, 0(a0)
  j first_epilogue
  li a0, 0
first_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl sum_ptr
sum_ptr:
  addi sp, sp, -32
  sw s0, 28(sp)
  sw ra, 24(sp)
  addi s0, sp, 32
  sw a0, -12(s0)
  sw a1, -16(s0)
  li a0, 0
  addi t0, s0, -20
  sw a0, 0(t0)
  li a0, 0
  addi t0, s0, -24
  sw a0, 0(t0)
sum_ptr_while_head_0:
  addi a0, s0, -20
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -16
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, t0, a0
  beqz a0, sum_ptr_while_end_1
  addi a0, s0, -24
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -24
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -20
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
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -20
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
  j sum_ptr_while_head_0
sum_ptr_while_end_1:
  addi a0, s0, -24
  lw a0, 0(a0)
  j sum_ptr_epilogue
  li a0, 0
sum_ptr_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 32
  ret
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
  addi a0, s0, -24
  addi t0, s0, -28
  sw a0, 0(t0)
  li a0, 10
  addi t0, s0, -32
  sw a0, 0(t0)
  addi a0, s0, -28
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 5
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -28
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
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
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call first
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -32
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -24
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 4
  addi sp, sp, -4
  sw a0, 0(sp)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a1, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call sum_ptr
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 32
  call rars_exit2
