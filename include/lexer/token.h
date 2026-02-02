#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
#define TOK(t) TOKEN_##t,
#define PUNCT(t, s) TOKEN_##t,
#define KEYWORD(t) TOKEN_KW_##t,
#include "tokens.def"
  TOKEN_COUNT
} TokenKind;

typedef struct {
  TokenKind kind;
  const char *start;
  size_t length;
  int32_t line;
  int32_t column;

  union {
    int32_t int_value;
    char char_value;
    char *string_value;
  } value;
} Token;

Token token_create(TokenKind kind, const char *start, size_t length, int32_t line, int32_t column);

const char *token_kind_name(TokenKind kind);

void token_print(const Token *token);

TokenKind token_check_keyword(const char *str, size_t length);

const char *token_punctuator_string(TokenKind kind);

void token_destroy(Token *token);
