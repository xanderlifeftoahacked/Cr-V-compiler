int main() {
    char letters[4] = {'a', 'b', 'c', 'd'};
    int i = 0;
    int sum = 0;

    while (i < 4) {
        sum = sum + letters[i];
        i = i + 1;
    }

    letters[2] = 'A';
    rars_print_int(sum + letters[2]);
    rars_print_char('\n');
    return 0;
}
