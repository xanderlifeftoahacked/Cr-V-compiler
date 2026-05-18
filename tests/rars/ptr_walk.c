int sum_slice(int *p, int len) {
    int s = 0;
    int i = 0;
    while (i < len) {
        s = s + *(p + i);
        i = i + 1;
    }
    return s;
}

int dot_product(int *a, int *b, int n) {
    int result = 0;
    int i = 0;
    while (i < n) {
        result = result + (*(a + i)) * (*(b + i));
        i = i + 1;
    }
    return result;
}

int ptr_diff(int *a, int *b) {
    return a - b;
}

int main() {
    int xs[5] = {1, 2, 3, 4, 5};
    int ys[5] = {5, 4, 3, 2, 1};
    int *p = xs;
    int *q = p + 3;

    int d = ptr_diff(q, p);

    int s1 = sum_slice(p, 3);
    int s2 = sum_slice(q, 2);

    *(p + 1) = *(p + 1) * 3;
    *q = *q + 10;

    int s3 = sum_slice(xs, 5);
    int dp = dot_product(xs, ys, 5);

    print_int(d + s1 + s2 + s3 + dp);
    putchar('\n');
    return 0;
}
