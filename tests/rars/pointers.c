int first(int xs[3]) {
    return xs[0];
}

int sum_ptr(int *values, int count) {
    int i = 0;
    int sum = 0;

    while (i < count) {
        sum = sum + values[i];
        i = i + 1;
    }

    return sum;
}

int main() {
    int values[4] = {1, 2, 3, 4};
    int *p = values;
    int x = 10;

    *p = 5;
    p[2] = first(values) + x;

    print_int(sum_ptr(values, 4));
    putchar('\n');
    return 0;
}
