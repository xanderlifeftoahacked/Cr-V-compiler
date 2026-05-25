.text
.globl main
main:
  addi sp, sp, -64
  sw s0, 60(sp)
  sw ra, 56(sp)
  addi s0, sp, 64
  li a0, 0
  addi t0, s0, -12
  sw a0, 0(t0)
  li a0, 0
  addi t0, s0, -16
  sw a0, 0(t0)
main_while_head_0:
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 8
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, t0, a0
  beqz a0, main_while_end_1
  addi a0, s0, -12
  mv t2, a0
  lw a0, 0(a0)
  mv t3, a0
  addi a0, a0, 1
  sw a0, 0(t2)
  mv a0, t3
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 3
  lw t0, 0(sp)
  addi sp, sp, 4
  sub a0, t0, a0
  seqz a0, a0
  beqz a0, main_endif_3
  j main_while_head_0
main_endif_3:
  addi a0, s0, -16
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  j main_while_head_0
main_while_end_1:
  li a0, 5
  addi t0, s0, -20
  sw a0, 0(t0)
  addi a0, s0, -20
  mv t2, a0
  lw a0, 0(a0)
  mv t3, a0
  addi a0, a0, 1
  sw a0, 0(t2)
  mv a0, t3
  addi t0, s0, -24
  sw a0, 0(t0)
  addi a0, s0, -20
  mv t2, a0
  lw a0, 0(a0)
  addi a0, a0, 1
  sw a0, 0(t2)
  addi t0, s0, -28
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 10
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 3
  lw t0, 0(sp)
  addi sp, sp, 4
  sub a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  lw t0, 0(sp)
  addi sp, sp, 4
  mul a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 4
  lw t0, 0(sp)
  addi sp, sp, 4
  div a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 5
  lw t0, 0(sp)
  addi sp, sp, 4
  rem a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 8
  lw t0, 0(sp)
  addi sp, sp, 4
  or a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 14
  lw t0, 0(sp)
  addi sp, sp, 4
  and a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 3
  lw t0, 0(sp)
  addi sp, sp, 4
  xor a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  lw t0, 0(sp)
  addi sp, sp, 4
  sll a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 3
  lw t0, 0(sp)
  addi sp, sp, 4
  sra a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  li a0, 1
  addi t0, s0, -40
  sw a0, 0(t0)
  li a0, 2
  addi t0, s0, -36
  sw a0, 0(t0)
  li a0, 3
  addi t0, s0, -32
  sw a0, 0(t0)
  addi a0, s0, -40
  addi t0, s0, -44
  sw a0, 0(t0)
  addi a0, s0, -44
  mv t2, a0
  lw a0, 0(a0)
  mv t3, a0
  li t1, 4
  add a0, a0, t1
  sw a0, 0(t2)
  mv a0, t3
  addi a0, s0, -44
  lw a0, 0(a0)
  lw a0, 0(a0)
  addi t0, s0, -48
  sw a0, 0(t0)
  addi a0, s0, -44
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  li t1, 4
  mul a0, a0, t1
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -44
  lw a0, 0(a0)
  lw a0, 0(a0)
  addi t0, s0, -52
  sw a0, 0(t0)
  addi a0, s0, -44
  mv t2, a0
  lw a0, 0(a0)
  mv t3, a0
  li t1, 4
  sub a0, a0, t1
  sw a0, 0(t2)
  mv a0, t3
  addi a0, s0, -44
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  li t1, 4
  mul a0, a0, t1
  lw t0, 0(sp)
  addi sp, sp, 4
  sub a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
  addi a0, s0, -44
  lw a0, 0(a0)
  lw a0, 0(a0)
  addi t0, s0, -56
  sw a0, 0(t0)
  addi a0, s0, -16
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -24
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -28
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -20
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -48
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -52
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -56
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 64
  call rars_exit2
