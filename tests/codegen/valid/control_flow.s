.text
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 0
  addi t0, s0, -12
  sw a0, 0(t0)
  li a0, 0
  addi t0, s0, -16
  sw a0, 0(t0)
  addi a0, s0, -12
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
main_for_head_0:
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 4
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, t0, a0
  beqz a0, main_for_end_2
  addi a0, s0, -16
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -16
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
main_for_post_1:
  addi a0, s0, -12
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
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
  j main_for_head_0
main_for_end_2:
main_do_head_3:
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
  sub a0, t0, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  sw a0, 0(t0)
main_do_continue_4:
  addi a0, s0, -16
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 3
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, a0, t0
  bnez a0, main_do_head_3
main_do_end_5:
  addi a0, s0, -16
  lw a0, 0(a0)
  mv t0, a0
  li t1, 3
  beq t0, t1, main_case_7
  j main_default_8
main_case_7:
  j main_switch_end_6
main_default_8:
  j main_switch_end_6
main_switch_end_6:
  j done
done:
  addi a0, s0, -16
  lw a0, 0(a0)
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  call rars_exit2
