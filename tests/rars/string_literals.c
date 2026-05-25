char *global_text = "G";
char global_buf[4] = "hi";

int main() {
  char *local_text = "A\n";
  char local_buf[4] = "bc";

  rars_print_string("S:");
  rars_print_string(global_text);
  rars_print_char(global_buf[1]);
  rars_print_string(local_text);
  rars_print_char(local_buf[0]);
  rars_print_char(local_buf[1]);
  rars_print_char('\n');
  rars_print_int("az"[1]);
  rars_print_char('\n');
  return 0;
}
