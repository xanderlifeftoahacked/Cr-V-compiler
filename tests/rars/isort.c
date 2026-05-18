int array_sum(int *arr, int n) {
    int s = 0;
    int i = 0;
    while (i < n) {
        s = s + arr[i];
        i = i + 1;
    }
    return s;
}

int isort(int *arr, int n) {
    int i = 1;
    while (i < n) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0) {
            if (arr[j] > key) {
                arr[j + 1] = arr[j];
                j = j - 1;
            } else {
                break;
            }
        }
        arr[j + 1] = key;
        i = i + 1;
    }
    return n;
}

int main() {
    int xs[7] = {5, 3, 8, 1, 9, 2, 7};
    isort(xs, 7);
    print_int(xs[0]);
    putchar('\n');
    print_int(xs[6]);
    putchar('\n');
    print_int(array_sum(xs, 7));
    putchar('\n');
    return 0;
}
