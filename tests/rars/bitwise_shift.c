int main() {
  int a = 1 + 2 << 3;
  int b = 16 >> 2 + 1;
  int c = 5 ^ 3;
  int d = 1 | 2 ^ 7 & 3;
  rars_print_int(a + b + c + d);
  rars_print_char('\n');
  return 0;
}
