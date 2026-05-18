int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int lcm(int a, int b) {
    return (a / gcd(a, b)) * b;
}

int main() {
    print_int(gcd(48, 18));
    putchar('\n');
    print_int(lcm(12, 18));
    putchar('\n');
    print_int(gcd(100, 75));
    putchar('\n');
    return 0;
}
