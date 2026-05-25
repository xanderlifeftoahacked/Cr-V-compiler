static int value = 3;

static int helper(int x) {
  return value + x;
}

int main() {
  return helper(4);
}
