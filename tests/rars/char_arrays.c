int main() {
    char letters[4] = {'a', 'b', 'c', 'd'};
    int i = 0;
    int sum = 0;

    while (i < 4) {
        sum = sum + letters[i];
        i = i + 1;
    }

    letters[2] = 'A';
    print_int(sum + letters[2]);
    putchar('\n');
    return 0;
}
