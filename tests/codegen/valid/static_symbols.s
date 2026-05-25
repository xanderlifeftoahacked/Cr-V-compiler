.data
__crv_static_0_value:
  .word 3
.text
__crv_static_0_helper:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  la a0, __crv_static_0_value
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  j __crv_static_0_helper_epilogue
  li a0, 0
__crv_static_0_helper_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 4
  addi sp, sp, -4
  sw a0, 0(sp)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call __crv_static_0_helper
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  call rars_exit2
