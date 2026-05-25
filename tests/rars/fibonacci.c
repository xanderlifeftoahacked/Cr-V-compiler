int fib(int n) {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

int main() {
  int sum = 0;
  int i = 0;
  i = i + 1;
  while (i < 10) {
    sum = sum + fib(i);
    i = i + 1;
  }
  rars_print_int(sum);
  rars_print_char('\n');
  return 0;
}
