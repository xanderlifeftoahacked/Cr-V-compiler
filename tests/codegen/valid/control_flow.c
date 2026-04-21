int main() {
    int i = 0;
    int sum = 0;
    for (i = 0; i < 4; i = i + 1) {
        sum = sum + i;
    }
    do {
        sum = sum - 1;
    } while (sum > 3);
    switch (sum) {
        case 3: break;
        default: break;
    }
    goto done;
done:
    return sum;
}

