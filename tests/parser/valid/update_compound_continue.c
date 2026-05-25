int main() {
    int i = 0;
    int total = 0;

    while (i < 10) {
        i++;
        if (i == 5) {
            continue;
        }
        total += i;
    }

    total -= 3;
    total *= 2;
    total /= 4;
    total %= 7;
    total &= 3;
    total |= 8;
    total ^= 1;
    total <<= 2;
    total >>= 1;

    return ++total + total--;
}
