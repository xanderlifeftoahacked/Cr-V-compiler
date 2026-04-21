.text
.globl main
main:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 0
  sw a0, -12(s0)
  li a0, 0
  sw a0, -16(s0)
  li a0, 0
  sw a0, -12(s0)
main_for_head_0:
  lw a0, -12(s0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 4
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, t0, a0
  beqz a0, main_for_end_1
  lw a0, -16(s0)
  addi sp, sp, -4
  sw a0, 0(sp)
  lw a0, -12(s0)
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  sw a0, -16(s0)
  lw a0, -12(s0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  lw t0, 0(sp)
  addi sp, sp, 4
  add a0, t0, a0
  sw a0, -12(s0)
  j main_for_head_0
main_for_end_1:
main_do_head_2:
  lw a0, -16(s0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 1
  lw t0, 0(sp)
  addi sp, sp, 4
  sub a0, t0, a0
  sw a0, -16(s0)
  lw a0, -16(s0)
  addi sp, sp, -4
  sw a0, 0(sp)
  li a0, 3
  lw t0, 0(sp)
  addi sp, sp, 4
  slt a0, a0, t0
  bnez a0, main_do_head_2
main_do_end_3:
  lw a0, -16(s0)
  mv t0, a0
  li t1, 3
  beq t0, t1, main_case_5
  j main_default_6
main_case_5:
  j main_switch_end_4
main_default_6:
  j main_switch_end_4
main_switch_end_4:
  j done
done:
  lw a0, -16(s0)
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  li a7, 10
  ecall

