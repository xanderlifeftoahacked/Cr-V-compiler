int print_int(int value) {
    return __rars_syscall1(1, value);
}

int print_int_hex(int value) {
    return __rars_syscall1(34, value);
}

int print_int_binary(int value) {
    return __rars_syscall1(35, value);
}

int print_uint(int value) {
    return __rars_syscall1(36, value);
}

int read_int() {
    return __rars_syscall0(5);
}

int putchar(int value) {
    return __rars_syscall1(11, value);
}

int getchar() {
    return __rars_syscall0(12);
}

int sbrk(int byte_count) {
    return __rars_syscall1(9, byte_count);
}

int time_ms_low() {
    return __rars_syscall0(30);
}

int sleep_ms(int milliseconds) {
    return __rars_syscall1(32, milliseconds);
}

int rand_seed(int generator, int seed) {
    return __rars_syscall2(40, generator, seed);
}

int rand_int(int generator) {
    return __rars_syscall1(41, generator);
}

int rand_range(int generator, int upper_bound) {
    return __rars_syscall2(42, generator, upper_bound);
}

int exit(int code) {
    return __rars_syscall1(93, code);
}
