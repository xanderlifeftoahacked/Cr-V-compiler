int square(int value) {
    return value * value;
}

int add3(int a, int b, int c) {
    return a + b + c;
}

int accumulate(int limit) {
    int i = 0;
    int result = 0;

    do {
        result = result + add3(square(i), i, 1);
        i = i + 1;
    } while (i < limit);

    return result;
}

int main() {
    return accumulate(8);
}
