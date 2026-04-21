#include <stdio.h>
#include <stdlib.h>

#include "codegen/codegen.h"
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
  if (argc != 2) {
    fprintf(stderr, "usage: %s <input.c>\n", argc > 0 ? argv[0] : "crv");
    return 1;
  }

  const char *path = argv[1];
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
    parser_destroy(&parser);
    lexer_destroy(&lexer);
    free(source);
    return 1;
  }

  if (semantic_analyze(result.module, path) != 0) {
    parser_destroy(&parser);
    lexer_destroy(&lexer);
    free(source);
    return 1;
  }

  if (codegen_emit_module(stdout, result.module, path) != 0) {
    parser_destroy(&parser);
    lexer_destroy(&lexer);
    free(source);
    return 1;
  }

  parser_destroy(&parser);
  lexer_destroy(&lexer);
  free(source);
  return 0;
}
