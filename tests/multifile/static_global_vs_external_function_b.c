int local_value();

int helper() {
  return 2;
}

int main() {
  rars_print_int(helper() + local_value());
  rars_print_char('\n');
  return 0;
}
