int main() {
    int i = 0;
    int sum = 0;

    for (i = 0; i < 4; i = i + 1) {
        sum = sum + i;
    }

    rars_print_int(sum);
    rars_print_char('\n');

    return 0;
}
