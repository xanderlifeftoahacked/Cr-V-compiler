#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/codegen.h"
#include "compiler/stdlib.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "utils/diagnostic.h"

static char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return NULL;
  }

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

int main(int argc, char **argv) {
  int use_stdlib = 1;
  const char *path = NULL;
  if (argc == 2) {
    path = argv[1];
  } else if (argc == 3 && strcmp(argv[1], "--nostdlib") == 0) {
    use_stdlib = 0;
    path = argv[2];
  } else {
    fprintf(stderr, "usage: %s [--nostdlib] <input.c>\n", argc > 0 ? argv[0] : "crv");
    return 1;
  }

  int exit_code = 1;
  char *stdlib_source = NULL;
  Lexer stdlib_lexer;
  Parser stdlib_parser;
  int stdlib_lexer_ready = 0;
  int stdlib_parser_ready = 0;
  AstModule merged_module = {0};
  int merged_module_ready = 0;

  char *source = read_file(path);
  if (!source) {
    fprintf(stderr, "cannot read %s\n", path);
    return 1;
  }

  diagnostic_init(path);

  Lexer lexer;
  lexer_init(&lexer, source, path);
  lexer_tokenize(&lexer);
  if (lexer_had_error(&lexer)) {
    lexer_destroy(&lexer);
    free(source);
    return 1;
  }

  Parser parser;
  parser_init(&parser, lexer_get_tokens(&lexer), source, path);
  ParseResult result = parser_parse(&parser);
  if (result.had_error) {
    goto cleanup;
  }

  const AstModule *module = result.module;
  if (use_stdlib) {
    const char *stdlib_path = crv_default_stdlib_path();
    stdlib_source = read_file(stdlib_path);
    if (!stdlib_source) {
      fprintf(stderr, "cannot read stdlib %s\n", stdlib_path);
      goto cleanup;
    }

    lexer_init(&stdlib_lexer, stdlib_source, stdlib_path);
    stdlib_lexer_ready = 1;
    lexer_tokenize(&stdlib_lexer);
    if (lexer_had_error(&stdlib_lexer)) {
      goto cleanup;
    }

    parser_init(&stdlib_parser, lexer_get_tokens(&stdlib_lexer), stdlib_source, stdlib_path);
    stdlib_parser_ready = 1;
    ParseResult stdlib_result = parser_parse(&stdlib_parser);
    if (stdlib_result.had_error) {
      goto cleanup;
    }

    if (crv_merge_modules(&merged_module, stdlib_result.module, result.module) != 0) {
      fprintf(stderr, "cannot merge stdlib module\n");
      goto cleanup;
    }
    merged_module_ready = 1;
    module = &merged_module;
  }

  if (semantic_analyze(module, path) != 0) {
    goto cleanup;
  }

  if (codegen_emit_module(stdout, module, path) != 0) {
    goto cleanup;
  }

  exit_code = 0;

cleanup:
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
  lexer_destroy(&lexer);
  free(source);
  return exit_code;
}
