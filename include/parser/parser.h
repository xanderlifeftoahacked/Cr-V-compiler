#pragma once

#include <stdint.h>
#include <stddef.h>

#include "lexer/lexer.h"
#include "parser/ast.h"

typedef struct ParserArena {
  void **items;
  size_t count;
  size_t capacity;
} ParserArena;

typedef struct Parser {
  const TokenArray *tokens;
  size_t current;
  const char *filename;
  const char *source_begin;
  int32_t had_error;
  ParserArena arena;
  AstModule *module;
} Parser;

typedef struct {
  AstModule *module;
  int32_t had_error;
} ParseResult;

void parser_init(Parser *parser, const TokenArray *tokens, const char *source, const char *filename);

ParseResult parser_parse(Parser *parser);

void parser_destroy(Parser *parser);
