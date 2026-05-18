int classify(int value) {
    int result = 0;

    if (value < 0) {
        result = -1;
    } else if (value == 0) {
        result = 10;
    } else {
        result = 1;
    }

    switch (value) {
    case 1:
        result = result + 100;
        break;
    case 2:
        result = result + 200;
        break;
    default:
        result = result + 300;
        break;
    }

    return result;
}

int main() {
    int i = -1;
    int total = 0;

    for (i = -1; i < 4; i = i + 1) {
        total = total + classify(i);
    }

    return total;
}
