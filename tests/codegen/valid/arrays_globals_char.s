.data
.globl g
g:
  .word 5
.globl marker
marker:
  .byte 65
.globl numbers
numbers:
  .word 1
  .word 2
  .word 3
.globl letters
letters:
  .byte 120
  .byte 121
  .byte 122
.text
.globl main
main:
  addi sp, sp, -32
  sw s0, 28(sp)
  sw ra, 24(sp)
  addi s0, sp, 32
  li a0, 4
  addi t0, s0, -20
  sw a0, 0(t0)
  li a0, 5
  addi t0, s0, -16
  sw a0, 0(t0)
  li a0, 6
  addi t0, s0, -12
  sw a0, 0(t0)
  li a0, 97
  addi t0, s0, -22
  sb a0, 0(t0)
  li a0, 98
  addi t0, s0, -21
  sb a0, 0(t0)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, numbers
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
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
  addi a0, s0, -22
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, marker
  lb a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  sb a0, 0(t0)
  la a0, g
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -20
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -22
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lb a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, letters
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
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
  la a0, g
  lw a0, 0(a0)
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 32
  li a7, 10
  ecall
