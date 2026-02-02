#pragma once

#include "lexer/token.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  Token *tokens;
  size_t count;
  size_t capacity;
} TokenArray;

typedef struct {
  const char *source;
  const char *current;
  const char *line_start;
  int32_t line;
  TokenArray tokens;
  const char *filename;
  int32_t had_error;
} Lexer;

void lexer_init(Lexer *lexer, const char *source, const char *filename);

int32_t lexer_tokenize(Lexer *lexer);

const TokenArray *lexer_get_tokens(const Lexer *lexer);

void lexer_destroy(Lexer *lexer);

void lexer_print_tokens(const Lexer *lexer);

int32_t lexer_had_error(const Lexer *lexer);
