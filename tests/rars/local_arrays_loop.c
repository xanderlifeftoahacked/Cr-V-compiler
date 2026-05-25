int main() {
    int xs[4] = {1, 2, 3, 4};
    int i = 0;
    int sum = 0;

    while (i < 4) {
        xs[i] = xs[i] + i;
        sum = sum + xs[i];
        i = i + 1;
    }

    rars_print_int(sum);
    rars_print_char('\n');
    return 0;
}
