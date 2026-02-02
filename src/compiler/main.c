#include <stdio.h>
#include <stdlib.h>
#include "lexer/lexer.h"
#include "utils/diagnostic.h"
#include "parser/parser.h"

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;

#if defined(DEBUG)
  LOG(INFO, "DEBUG MODE");
#endif

  return 0;
}
