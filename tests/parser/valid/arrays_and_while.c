int main() {
    int arr[3] = {1, 2, 3};
    int i = 0;
    int sum = 0;
    while (i < 3) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}
