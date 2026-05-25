int a();

static int value = 2;

static int helper() {
  return value;
}

int main() {
  rars_print_int(a() + helper());
  rars_print_char('\n');
  return 0;
}
