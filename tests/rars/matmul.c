int mat_a[16];
int mat_b[16];
int mat_c[16];

int read_matrix(int *m, int n) {
    int total = n * n;
    int i = 0;
    while (i < total) {
        m[i] = read_int();
        i = i + 1;
    }
    return 0;
}

int matmul(int *res, int *ma, int *mb, int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j < n) {
            int s = 0;
            int k = 0;
            while (k < n) {
                s = s + ma[i * n + k] * mb[k * n + j];
                k = k + 1;
            }
            res[i * n + j] = s;
            j = j + 1;
        }
        i = i + 1;
    }
    return 0;
}

int print_matrix(int *m, int n) {
    int i = 0;
    while (i < n) {
        int j = 0;
        while (j < n) {
            print_int(m[i * n + j]);
            if (j < n - 1) {
                putchar(' ');
            }
            j = j + 1;
        }
        putchar('\n');
        i = i + 1;
    }
    return 0;
}

int main() {
    int n = read_int();
    read_matrix(mat_a, n);
    read_matrix(mat_b, n);
    matmul(mat_c, mat_a, mat_b, n);
    print_matrix(mat_c, n);
    return 0;
}
