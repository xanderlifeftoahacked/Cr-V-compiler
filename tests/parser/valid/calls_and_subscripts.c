int foo() {
    return 5;
}

int main() {
    int arr[2] = {0, 1};
    int x = foo();
    arr[0] = x + arr[1];
    return arr[0];
}
