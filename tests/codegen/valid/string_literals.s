.data
__crv_str_0:
  .byte 71
  .byte 0
__crv_str_1:
  .byte 65
  .byte 0
__crv_str_2:
  .byte 97
  .byte 122
  .byte 0
  .align 2
.data
.globl global_text
global_text:
  .word __crv_str_0
.globl global_buf
global_buf:
  .byte 104
  .byte 105
  .byte 0
  .byte 0
.text
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  la a0, __crv_str_1
  addi t0, s0, -12
  sw a0, 0(t0)
  li a0, 98
  addi t0, s0, -16
  sb a0, 0(t0)
  li a0, 99
  addi t0, s0, -15
  sb a0, 0(t0)
  li a0, 0
  addi t0, s0, -14
  sb a0, 0(t0)
  li a0, 0
  addi t0, s0, -13
  sb a0, 0(t0)
  la a0, global_text
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  lb a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, global_buf
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
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
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
  addi a0, s0, -16
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
  addi sp, sp, -4
  sw a0, 0(sp)
  la a0, __crv_str_2
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
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  call rars_exit2
