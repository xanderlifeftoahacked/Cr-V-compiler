int g = 5;
char marker = 'A';
int numbers[3] = {1, 2, 3};
char letters[3] = {'x', 'y', 'z'};

int main() {
    int local[3] = {4, 5, 6};
    char bytes[2] = {'a', 'b'};

    local[1] = local[0] + numbers[2];
    bytes[0] = marker;
    g = local[1] + bytes[0] + letters[1];

    return g;
}
