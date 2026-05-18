int main() {
    int i = 0;
    int sum = 0;

    for (i = 0; i < 4; i = i + 1) {
        sum = sum + i;
    }

    print_int(sum);
    putchar('\n');

    return 0;
}
