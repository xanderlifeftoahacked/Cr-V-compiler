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
    rars_print_int(gcd(48, 18));
    rars_print_char('\n');
    rars_print_int(lcm(12, 18));
    rars_print_char('\n');
    rars_print_int(gcd(100, 75));
    rars_print_char('\n');
    return 0;
}
