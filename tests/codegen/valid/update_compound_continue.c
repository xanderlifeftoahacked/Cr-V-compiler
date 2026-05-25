int main() {
    int i = 0;
    int sum = 0;

    while (i < 8) {
        i++;
        if (i == 3) {
            continue;
        }
        sum += i;
    }

    int x = 5;
    int a = x++;
    int b = ++x;
    x += 10;
    x -= 3;
    x *= 2;
    x /= 4;
    x %= 5;
    x |= 8;
    x &= 14;
    x ^= 3;
    x <<= 2;
    x >>= 3;

    int values[3] = {1, 2, 3};
    int *p = values;
    p++;
    int c = *p;
    p += 1;
    int d = *p;
    p--;
    p -= 1;
    int e = *p;

    return sum + a + b + x + c + d + e;
}
