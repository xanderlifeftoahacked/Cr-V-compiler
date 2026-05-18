int mix(int a, int b, int c) {
    int x = a + b * c;
    int y = x - a / 2;
    int z = y + c;

    return (z & 255) + (x | b);
}

int main() {
    int total = 0;
    int i = 0;

    while (i < 10) {
        total = total + mix(i, i + 1, i + 2);
        i = i + 1;
    }

    return total;
}
