#pragma once

int __rars_syscall0(int number);
int __rars_syscall1(int number, int a0);
int __rars_syscall2(int number, int a0, int a1);

int print_int(int value);
int print_int_hex(int value);
int print_int_binary(int value);
int print_uint(int value);
int read_int(void);
int putchar(int value);
int getchar(void);
int sbrk(int byte_count);
int time_ms_low(void);
int sleep_ms(int milliseconds);
int rand_seed(int generator, int seed);
int rand_int(int generator);
int rand_range(int generator, int upper_bound);
int exit(int code);
