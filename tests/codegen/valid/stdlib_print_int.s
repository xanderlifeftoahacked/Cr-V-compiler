.text
.globl print_int
print_int:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 1
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j print_int_epilogue
  li a0, 0
print_int_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl print_int_hex
print_int_hex:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 34
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j print_int_hex_epilogue
  li a0, 0
print_int_hex_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl print_int_binary
print_int_binary:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 35
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j print_int_binary_epilogue
  li a0, 0
print_int_binary_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl print_uint
print_uint:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 36
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j print_uint_epilogue
  li a0, 0
print_uint_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl read_int
read_int:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 5
  mv a7, a0
  ecall
  j read_int_epilogue
  li a0, 0
read_int_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl putchar
putchar:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 11
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j putchar_epilogue
  li a0, 0
putchar_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl getchar
getchar:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 12
  mv a7, a0
  ecall
  j getchar_epilogue
  li a0, 0
getchar_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl sbrk
sbrk:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 9
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j sbrk_epilogue
  li a0, 0
sbrk_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl time_ms_low
time_ms_low:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  li a0, 30
  mv a7, a0
  ecall
  j time_ms_low_epilogue
  li a0, 0
time_ms_low_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl sleep_ms
sleep_ms:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 32
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j sleep_ms_epilogue
  li a0, 0
sleep_ms_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl rand_seed
rand_seed:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  sw a1, -16(s0)
  li a0, 40
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -16
  lw a0, 0(a0)
  mv a1, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j rand_seed_epilogue
  li a0, 0
rand_seed_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl rand_int
rand_int:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 41
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j rand_int_epilogue
  li a0, 0
rand_int_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl rand_range
rand_range:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  sw a1, -16(s0)
  li a0, 42
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -16
  lw a0, 0(a0)
  mv a1, a0
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j rand_range_epilogue
  li a0, 0
rand_range_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  ret
.globl exit
exit:
  addi sp, sp, -16
  sw s0, 12(sp)
  sw ra, 8(sp)
  addi s0, sp, 16
  sw a0, -12(s0)
  li a0, 93
  addi sp, sp, -4
  sw a0, 0(sp)
  addi a0, s0, -12
  lw a0, 0(a0)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a7, t0
  ecall
  j exit_epilogue
  li a0, 0
exit_epilogue:
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
  li a0, 42
  addi sp, sp, -4
  sw a0, 0(sp)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call print_int
  li a0, 10
  addi sp, sp, -4
  sw a0, 0(sp)
  lw t0, 0(sp)
  addi sp, sp, 4
  mv a0, t0
  call putchar
  li a0, 0
  j main_epilogue
  li a0, 0
main_epilogue:
  lw ra, -8(s0)
  lw s0, -4(s0)
  addi sp, sp, 16
  li a7, 10
  ecall
