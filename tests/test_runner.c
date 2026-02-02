#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"
#include "utils/diagnostic.h"

#ifndef TEST_ROOT
#define TEST_ROOT "tests"
#endif

typedef enum {
  TEST_LEX,
  TEST_PARSE
} TestStage;

typedef struct {
  const char *path;
  int expect_success;
  TestStage stage;
} TestCase;

static char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return NULL;
  }
  rewind(f);
  char *buffer = malloc((size_t) size + 1);
  if (!buffer) {
    fclose(f);
    return NULL;
  }
  size_t read = fread(buffer, 1, (size_t) size, f);
  buffer[read] = '\0';
  fclose(f);
  return buffer;
}

static char *make_path(const char *rel) {
  size_t base_len = strlen(TEST_ROOT);
  size_t rel_len = strlen(rel);
  char *full = malloc(base_len + 1 + rel_len + 1);
  memcpy(full, TEST_ROOT, base_len);
  full[base_len] = '/';
  memcpy(full + base_len + 1, rel, rel_len);
  full[base_len + 1 + rel_len] = '\0';
  return full;
}

static int run_one(const TestCase *tc) {
  char *path = make_path(tc->path);
  char *source = read_file(path);
  if (!source) {
    printf("[ERROR] cannot read %s\n", path);
    free(path);
    return 0;
  }

  diagnostic_reset();

  Lexer lexer;
  lexer_init(&lexer, source, path);
  lexer_tokenize(&lexer);

  int ok = 1;
  if (lexer_had_error(&lexer)) {
    ok = 0;
  }

  if (ok && tc->stage == TEST_PARSE) {
    Parser parser;
    parser_init(&parser, lexer_get_tokens(&lexer), source, path);
    ParseResult pr = parser_parse(&parser);
    if (pr.had_error) {
      ok = 0;
    }
    if (ok && pr.module) {
      printf("{ AST for %s:\n", path);
      ast_print_module(pr.module);
      printf("end AST }");
    }
    parser_destroy(&parser);
  }

  lexer_destroy(&lexer);
  free(source);

  int pass = (ok == tc->expect_success);
  printf("[%s] %s (expected %s)\n", pass ? "PASS" : "FAIL", path,
         tc->expect_success ? "success" : "failure");
  free(path);
  return pass;
}

int main(void) {
  const TestCase tests[] = {
    {"lexer/invalid/lexer_error.c", 0, TEST_LEX},

    {"parser/valid/simple_main.c", 1, TEST_PARSE},
    {"parser/valid/arrays_and_while.c", 1, TEST_PARSE},
    {"parser/valid/calls_and_subscripts.c", 1, TEST_PARSE},
    {"parser/valid/func_params.c", 1, TEST_PARSE},
  };

  int passed = 0;
  int total = (int) (sizeof(tests) / sizeof(tests[0]));
  for (int i = 0; i < total; i++) {
    passed += run_one(&tests[i]);
  }

  printf("\nsummary: %d/%d passed\n", passed, total);
  return (passed == total) ? 0 : 1;
}
