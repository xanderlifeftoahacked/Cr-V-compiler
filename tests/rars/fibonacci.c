int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int sum = 0;
    int i = 0;
    while (i < 10) {
        sum = sum + fib(i);
        i = i + 1;
    }
    print_int(sum);
    putchar('\n');
    return 0;
}
