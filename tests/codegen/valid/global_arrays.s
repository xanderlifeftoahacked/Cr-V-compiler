.data
.globl xs
xs:
  .word 2
  .word 4
  .word 6
  .word 8
.globl flags
flags:
  .byte 1
  .byte 2
  .byte 3
.globl total
total:
  .word 0
.text
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  la a0, xs
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
  la a0, xs
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
  la a0, xs
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
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
  la a0, flags
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, flags
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lb a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, flags
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
  lw t0, 0(sp)
  addi sp, sp, 4
  sb a0, 0(t0)
  la a0, total
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, xs
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 2
  li t0, 4
  mul a0, a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, flags
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
  la a0, total
  lw a0, 0(a0)
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  li a7, 10
  ecall
