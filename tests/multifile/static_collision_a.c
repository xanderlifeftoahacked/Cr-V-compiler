static int value = 1;

static int helper() {
  return value;
}

int a() {
  return helper();
}
