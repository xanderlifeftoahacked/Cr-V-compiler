int first(int xs[3]) {
    return xs[0];
}

int bump(int *value) {
    *value = *value + 1;
    return *value;
}

int main() {
    int values[3] = {1, 2, 3};
    int *p = values;
    int x = 10;

    p[1] = first(values);
    return bump(&x) + p[1];
}
