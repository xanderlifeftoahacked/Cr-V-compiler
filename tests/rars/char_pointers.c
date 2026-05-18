int main() {
    char text[3] = {'a', 'b', 'c'};
    char *p = text;

    *p = 'A';
    p[2] = p[0] + 2;

    print_int(text[0] + text[1] + text[2]);
    putchar('\n');
    return 0;
}
