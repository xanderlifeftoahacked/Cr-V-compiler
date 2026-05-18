#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"
#include "codegen/codegen.h"
#include "compiler/stdlib.h"
#include "semantic/semantic.h"
#include "utils/diagnostic.h"

#ifndef TEST_ROOT
#define TEST_ROOT "tests"
#endif

typedef enum {
  TEST_LEX,
  TEST_PARSE,
  TEST_SEMANTIC,
  TEST_CODEGEN,
  TEST_CODEGEN_STDLIB
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

static char *make_expected_path(const char *rel) {
  size_t len = strlen(rel);
  char *path = malloc(len + 3);
  memcpy(path, rel, len);
  if (len >= 2 && rel[len - 2] == '.' && rel[len - 1] == 'c') {
    path[len - 1] = 's';
    path[len] = '\0';
  } else {
    path[len] = '.';
    path[len + 1] = 's';
    path[len + 2] = '\0';
  }
  return path;
}

static void trim_newlines(char *text) {
  if (!text) {
    return;
  }
  size_t len = strlen(text);
  while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
    text[--len] = '\0';
  }
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

  if (ok && (tc->stage == TEST_PARSE || tc->stage == TEST_SEMANTIC)) {
    Parser parser;
    parser_init(&parser, lexer_get_tokens(&lexer), source, path);
    ParseResult pr = parser_parse(&parser);
    if (pr.had_error) {
      ok = 0;
    }
    if (ok && tc->stage == TEST_SEMANTIC) {
      ok = semantic_analyze(pr.module, path) == 0;
    }
    if (ok && pr.module) {
      printf("{ AST for %s:\n", path);
      ast_print_module(pr.module);
      printf("end AST }");
    }
    parser_destroy(&parser);
  }

  if (ok && (tc->stage == TEST_CODEGEN || tc->stage == TEST_CODEGEN_STDLIB)) {
    Parser parser;
    parser_init(&parser, lexer_get_tokens(&lexer), source, path);
    ParseResult pr = parser_parse(&parser);
    if (pr.had_error) {
      ok = 0;
    }

    const AstModule *module = pr.module;
    char *stdlib_source = NULL;
    Lexer stdlib_lexer;
    Parser stdlib_parser;
    int stdlib_lexer_ready = 0;
    int stdlib_parser_ready = 0;
    AstModule merged_module = {0};
    int merged_module_ready = 0;

    if (ok && tc->stage == TEST_CODEGEN_STDLIB) {
      const char *stdlib_path = crv_default_stdlib_path();
      stdlib_source = read_file(stdlib_path);
      if (!stdlib_source) {
        ok = 0;
      } else {
        lexer_init(&stdlib_lexer, stdlib_source, stdlib_path);
        stdlib_lexer_ready = 1;
        lexer_tokenize(&stdlib_lexer);
        if (lexer_had_error(&stdlib_lexer)) {
          ok = 0;
        }
      }
      if (ok) {
        parser_init(&stdlib_parser, lexer_get_tokens(&stdlib_lexer), stdlib_source, stdlib_path);
        stdlib_parser_ready = 1;
        ParseResult stdlib_pr = parser_parse(&stdlib_parser);
        if (stdlib_pr.had_error || crv_merge_modules(&merged_module, stdlib_pr.module, pr.module) != 0) {
          ok = 0;
        } else {
          merged_module_ready = 1;
          module = &merged_module;
        }
      }
    }

    if (ok && semantic_analyze(module, path) != 0) {
      ok = 0;
    }
    if (ok) {
      FILE *tmp = tmpfile();
      if (!tmp) {
        ok = 0;
      } else {
        ok = codegen_emit_module(tmp, module, path) == 0;
        if (ok) {
          rewind(tmp);
          fseek(tmp, 0, SEEK_END);
          long out_size = ftell(tmp);
          rewind(tmp);
          char *out_text = malloc((size_t) out_size + 1);
          if (!out_text) {
            ok = 0;
          } else {
            fread(out_text, 1, (size_t) out_size, tmp);
            out_text[out_size] = '\0';
            char *expected_rel = make_expected_path(tc->path);
            char *expected_path = make_path(expected_rel);
            char *expected_text = read_file(expected_path);
            trim_newlines(out_text);
            trim_newlines(expected_text);
            if (!expected_text || strcmp(out_text, expected_text) != 0) {
              ok = 0;
            }
            free(expected_text);
            free(expected_path);
            free(expected_rel);
            free(out_text);
          }
          fclose(tmp);
        }
      }
    }
    if (merged_module_ready) {
      crv_free_merged_module(&merged_module);
    }
    if (stdlib_parser_ready) {
      parser_destroy(&stdlib_parser);
    }
    if (stdlib_lexer_ready) {
      lexer_destroy(&stdlib_lexer);
    }
    free(stdlib_source);
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
    {"parser/valid/nested_initializer.c", 1, TEST_PARSE},
    {"parser/valid/logical_ops.c", 1, TEST_PARSE},
    {"parser/valid/control_flow.c", 1, TEST_PARSE},
    {"parser/valid/goto_with_label.c", 1, TEST_PARSE},
    {"parser/valid/pointers.c", 1, TEST_PARSE},

    {"parser/invalid/empty_translation_unit.c", 0, TEST_PARSE},
    {"parser/invalid/trailing_comma_initializer.c", 0, TEST_PARSE},
    {"parser/invalid/struct_not_supported.c", 0, TEST_PARSE},
    {"parser/invalid/case_outside_switch.c", 0, TEST_PARSE},
    {"parser/invalid/duplicate_case.c", 0, TEST_SEMANTIC},
    {"parser/invalid/duplicate_default.c", 0, TEST_SEMANTIC},
    {"parser/invalid/goto_unknown_label.c", 0, TEST_SEMANTIC},
    {"parser/invalid/duplicate_label.c", 0, TEST_SEMANTIC},
    {"parser/invalid/for_missing_semicolon.c", 0, TEST_PARSE},

    {"semantic/valid/basic_semantic_ok.c", 1, TEST_SEMANTIC},
    {"semantic/valid/pointer_arithmetic.c", 1, TEST_SEMANTIC},

    {"semantic/invalid/undeclared_identifier.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/unknown_function_call.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/duplicate_param.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/duplicate_local_same_scope.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/break_outside_loop.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/call_arity_mismatch.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/non_lvalue_assignment.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/scalar_init_list.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/zero_length_array.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/array_scalar_initializer.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/array_init_too_many_elements.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/assignment_to_array.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/subscript_non_array.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/call_non_identifier_target.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/call_shadowed_function_name.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/call_array_arg_to_scalar_param.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/return_array_value.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/array_used_as_scalar_expr.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/subscript_array_index.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/global_function_name_conflict.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/global_initializer_expression.c", 0, TEST_CODEGEN},
    {"semantic/invalid/deref_non_pointer.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/address_of_non_lvalue.c", 0, TEST_SEMANTIC},
    {"semantic/invalid/pointer_add_two_pointers.c", 0, TEST_SEMANTIC},

    {"codegen/valid/simple_main.c", 1, TEST_CODEGEN},
    {"codegen/valid/goto_with_label.c", 1, TEST_CODEGEN},
    {"codegen/valid/control_flow.c", 1, TEST_CODEGEN},
    {"codegen/valid/arrays_globals_char.c", 1, TEST_CODEGEN},
    {"codegen/valid/local_arrays_loop.c", 1, TEST_CODEGEN},
    {"codegen/valid/global_arrays.c", 1, TEST_CODEGEN},
    {"codegen/valid/char_arrays.c", 1, TEST_CODEGEN},
    {"codegen/valid/pointers.c", 1, TEST_CODEGEN},
    {"codegen/valid/pointer_arithmetic.c", 1, TEST_CODEGEN},
    {"codegen/valid/stdlib_print_int.c", 1, TEST_CODEGEN_STDLIB},
  };

  int passed = 0;
  int total = (int) (sizeof(tests) / sizeof(tests[0]));
  for (int i = 0; i < total; i++) {
    passed += run_one(&tests[i]);
  }

  printf("\nsummary: %d/%d passed\n", passed, total);
  return (passed == total) ? 0 : 1;
}
