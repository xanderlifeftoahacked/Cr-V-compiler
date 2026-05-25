int main() {
    int nums[4] = {10, 20, 30, 40};
    int *p = nums;
    int *q = p + 2;
    int *r = p + 1;
    int diff = q - p;
    int value = *(p + 1);
    int total = diff + value + (r - p);

    rars_print_int(total);
    rars_print_char('\n');
    return 0;
}

