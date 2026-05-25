extern int shared;
int get_shared();

int main() {
  rars_print_int(shared + get_shared());
  rars_print_char('\n');
  return 0;
}
