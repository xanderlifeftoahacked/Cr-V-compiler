.text

.globl rars_print_int
rars_print_int:
  li a7, 1
  ecall
  ret

.globl rars_print_string
rars_print_string:
  li a7, 4
  ecall
  ret

.globl rars_read_int
rars_read_int:
  li a7, 5
  ecall
  ret

.globl rars_read_string
rars_read_string:
  li a7, 8
  ecall
  ret

.globl rars_sbrk
rars_sbrk:
  li a7, 9
  ecall
  ret

.globl rars_exit
rars_exit:
  li a7, 10
  ecall
  ret

.globl rars_print_char
rars_print_char:
  li a7, 11
  ecall
  ret

.globl rars_read_char
rars_read_char:
  li a7, 12
  ecall
  ret

.globl rars_get_cwd
rars_get_cwd:
  li a7, 17
  ecall
  ret

.globl rars_time
rars_time:
  li a7, 30
  ecall
  ret

.globl rars_time_low
rars_time_low:
  li a7, 30
  ecall
  ret

.globl rars_time_high
rars_time_high:
  li a7, 30
  ecall
  mv a0, a1
  ret

.globl rars_midi_out
rars_midi_out:
  li a7, 31
  ecall
  ret

.globl rars_sleep_ms
rars_sleep_ms:
  li a7, 32
  ecall
  ret

.globl rars_midi_out_sync
rars_midi_out_sync:
  li a7, 33
  ecall
  ret

.globl rars_print_int_hex
rars_print_int_hex:
  li a7, 34
  ecall
  ret

.globl rars_print_int_binary
rars_print_int_binary:
  li a7, 35
  ecall
  ret

.globl rars_print_uint
rars_print_uint:
  li a7, 36
  ecall
  ret

.globl rars_rand_seed
rars_rand_seed:
  li a7, 40
  ecall
  ret

.globl rars_rand_int
rars_rand_int:
  li a7, 41
  ecall
  ret

.globl rars_rand_range
rars_rand_range:
  li a7, 42
  ecall
  ret

.globl rars_confirm_dialog
rars_confirm_dialog:
  li a7, 50
  ecall
  ret

.globl rars_input_dialog_int
rars_input_dialog_int:
  li a7, 51
  ecall
  ret

.globl rars_input_dialog_string
rars_input_dialog_string:
  li a7, 54
  ecall
  ret

.globl rars_message_dialog
rars_message_dialog:
  li a7, 55
  ecall
  ret

.globl rars_message_dialog_int
rars_message_dialog_int:
  li a7, 56
  ecall
  ret

.globl rars_close
rars_close:
  li a7, 57
  ecall
  ret

.globl rars_message_dialog_string
rars_message_dialog_string:
  li a7, 59
  ecall
  ret

.globl rars_lseek
rars_lseek:
  li a7, 62
  ecall
  ret

.globl rars_read
rars_read:
  li a7, 63
  ecall
  ret

.globl rars_write
rars_write:
  li a7, 64
  ecall
  ret

.globl rars_exit2
rars_exit2:
  li a7, 93
  ecall
  li a7, 10
  ecall

.globl rars_open
rars_open:
  li a7, 1024
  ecall
  ret
