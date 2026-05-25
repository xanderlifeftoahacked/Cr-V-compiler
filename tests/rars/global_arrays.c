int xs[4] = {2, 4, 6, 8};
char flags[3] = {1, 2, 3};
int total = 0;

int main() {
    xs[2] = xs[0] + xs[1];
    flags[1] = flags[0] + flags[2];
    total = xs[2] + flags[1];

    rars_print_int(total);
    rars_print_char('\n');
    return 0;
}
