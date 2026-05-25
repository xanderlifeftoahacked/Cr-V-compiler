#include <stdio.h>
#include <stdlib.h>

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

typedef struct {
  const char *path;
  char *source;
  Lexer lexer;
  Parser parser;
  ParseResult result;
  int lexer_ready;
  int parser_ready;
} CompilationUnit;

static int parse_unit(CompilationUnit *unit) {
  unit->source = read_file(unit->path);
  if (!unit->source) {
    fprintf(stderr, "cannot read %s\n", unit->path);
    return 1;
  }

  lexer_init(&unit->lexer, unit->source, unit->path);
  unit->lexer_ready = 1;
  lexer_tokenize(&unit->lexer);
  if (lexer_had_error(&unit->lexer)) {
    return 1;
  }

  parser_init(&unit->parser, lexer_get_tokens(&unit->lexer), unit->source, unit->path);
  unit->parser_ready = 1;
  unit->result = parser_parse(&unit->parser);
  return unit->result.had_error ? 1 : 0;
}

static void cleanup_unit(CompilationUnit *unit) {
  if (unit->parser_ready) {
    parser_destroy(&unit->parser);
  }
  if (unit->lexer_ready) {
    lexer_destroy(&unit->lexer);
  }
  free(unit->source);
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    fprintf(stderr, "usage: %s <input.c>...\n", argc > 0 ? argv[0] : "crv");
    return 1;
  }

  int exit_code = 1;
  size_t input_count = (size_t) (argc - 1);
  size_t unit_count = input_count;
  CompilationUnit *units = calloc(unit_count, sizeof(CompilationUnit));
  const AstModule **modules = calloc(unit_count, sizeof(AstModule *));
  AstModule merged_module = {0};
  int merged_module_ready = 0;

  if (!units || !modules) {
    fprintf(stderr, "out of memory\n");
    free(units);
    free(modules);
    return 1;
  }

  size_t unit_index = 0;
  for (int i = 1; i < argc; i++) {
    units[unit_index++].path = argv[i];
  }

  const char *diagnostic_name = input_count == 1 ? argv[1] : "<multiple inputs>";
  diagnostic_init(diagnostic_name);

  for (size_t i = 0; i < unit_count; i++) {
    if (parse_unit(&units[i]) != 0) {
      goto cleanup;
    }
    modules[i] = units[i].result.module;
  }

  if (crv_merge_module_list(&merged_module, modules, unit_count) != 0) {
    fprintf(stderr, "cannot merge modules\n");
    goto cleanup;
  }
  merged_module_ready = 1;

  if (semantic_analyze(&merged_module, diagnostic_name) != 0) {
    goto cleanup;
  }

  if (codegen_emit_module(stdout, &merged_module, diagnostic_name) != 0) {
    goto cleanup;
  }

  exit_code = 0;

cleanup:
  if (merged_module_ready) {
    crv_free_merged_module(&merged_module);
  }
  for (size_t i = 0; i < unit_count; i++) {
    cleanup_unit(&units[i]);
  }
  free(modules);
  free(units);
  return exit_code;
}
