char *global_text = "G";
char global_buf[4] = "hi";

int main() {
  char *local_text = "A";
  char local_buf[4] = "bc";
  return global_text[0] + global_buf[1] + local_text[0] + local_buf[1] + "az"[1];
}
