#include "lexer/token.h"
#include "utils/attributes.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *token_names[] = {
#define TOK(t) #t,
#define PUNCT(t, s) #t,
#define KEYWORD(t) "KW_" #t,
#include "lexer/tokens.def"
};

typedef struct {
  TokenKind kind;
  const char *str;
  size_t length;
} PunctuatorEntry;

static const PunctuatorEntry punctuator_table[] = {
#define PUNCT(t, s) { TOKEN_##t, s, sizeof(s) - 1 },
#include "lexer/tokens.def"
  {TOKEN_UNKNOWN, NULL, 0}
};

typedef struct {
  const char *word;
  TokenKind kind;
} KeywordEntry;

static const KeywordEntry keyword_table[] = {
#define KEYWORD(t) { #t, TOKEN_KW_##t },
#include "lexer/tokens.def"
  {NULL, TOKEN_UNKNOWN}
};

INLINE Token token_create(TokenKind kind, const char *start, size_t length,
                          int32_t line, int32_t column) {
  Token token = {
    .kind = kind,
    .start = start,
    .length = length,
    .line = line,
    .column = column,
    .value = {.int_value = 0}
  };
  return token;
}

INLINE const char *token_kind_name(TokenKind kind) {
  if (kind >= 0 && kind < TOKEN_COUNT) {
    return token_names[kind];
  }
  return "INVALID";
}

void token_print(const Token *token) {
  printf("Token{%s", token_kind_name(token->kind));

  if (token->kind == TOKEN_IDENTIFIER ||
      token->kind == TOKEN_NUMBER ||
      token->kind == TOKEN_STRING_LITERAL ||
      token->kind == TOKEN_CHAR_LITERAL) {
    printf(", \"%.*s\"", (int32_t) token->length, token->start);
  }

  printf(", line=%d, col=%d}", token->line, token->column);
}

INLINE TokenKind token_check_keyword(const char *str, size_t length) {
  for (size_t i = 0; keyword_table[i].word != NULL; i++) {
    size_t kw_len = strlen(keyword_table[i].word);
    if (length == kw_len &&
        strncmp(str, keyword_table[i].word, length) == 0) {
      return keyword_table[i].kind;
    }
  }
  return TOKEN_IDENTIFIER;
}

INLINE const char *token_punctuator_string(TokenKind kind) {
  for (size_t i = 0; punctuator_table[i].str != NULL; i++) {
    if (punctuator_table[i].kind == kind) {
      return punctuator_table[i].str;
    }
  }
  return NULL;
}

INLINE void token_destroy(Token *token) {
  if (token->kind == TOKEN_STRING_LITERAL && token->value.string_value) {
    free(token->value.string_value);
    token->value.string_value = NULL;
  }
}
