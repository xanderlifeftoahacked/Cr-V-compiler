#include "lexer/lexer.h"
#include "utils/diagnostic.h"
#include "utils/attributes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>

#define INITIAL_TOKEN_CAPACITY 128

static void token_array_init(TokenArray *array) {
  array->tokens = malloc(INITIAL_TOKEN_CAPACITY * sizeof(Token));
  array->count = 0;
  array->capacity = INITIAL_TOKEN_CAPACITY;
}

static void token_array_push(TokenArray *array, const Token token) {
  if (array->count >= array->capacity) {
    array->capacity *= 2;
    array->tokens = realloc(array->tokens, array->capacity * sizeof(Token));
  }
  array->tokens[array->count++] = token;
}

static void token_array_destroy(TokenArray *array) {
  for (size_t i = 0; i < array->count; i++) {
    token_destroy(&array->tokens[i]);
  }
  free(array->tokens);
  array->tokens = NULL;
  array->count = 0;
  array->capacity = 0;
}

static INLINE int32_t is_eof(const Lexer *lexer) {
  return *lexer->current == '\0';
}

static INLINE char peek(const Lexer *lexer) {
  return *lexer->current;
}

static INLINE char peek_next(const Lexer *lexer) {
  if (is_eof(lexer)) return '\0';
  return lexer->current[1];
}

static INLINE char advance(Lexer *lexer) {
  return *lexer->current++;
}

static INLINE int32_t match(Lexer *lexer, const char expected) {
  if (is_eof(lexer)) return 0;
  if (*lexer->current != expected) return 0;
  lexer->current++;
  return 1;
}

static INLINE int32_t get_column(const Lexer *lexer, const char *pos) {
  return (int32_t) (pos - lexer->line_start) + 1;
}

static void lexer_error(Lexer *lexer, const char *pos, const char *fmt, ...) {
  lexer->had_error = 1;

  const char *line_end = pos;
  while (*line_end != '\n' && *line_end != '\0') {
    line_end++;
  }

  size_t line_len = line_end - lexer->line_start;
  char *line_buf = malloc(line_len + 1);
  strncpy(line_buf, lexer->line_start, line_len);
  line_buf[line_len] = '\0';

  SourceLocation loc = {
    .filename = lexer->filename,
    .line = lexer->line,
    .column = get_column(lexer, pos),
    .source_line = line_buf
  };

  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  diagnostic_log(DIAG_LEVEL_ERROR, loc, "%s", message);

  free(line_buf);
}

static void skip_whitespace(Lexer *lexer) {
  while (1) {
    char c = peek(lexer);
    switch (c) {
      case ' ':
      case '\t':
      case '\r':
        advance(lexer);
        break;
      case '\n':
        lexer->line++;
        advance(lexer);
        lexer->line_start = lexer->current;
        break;
      case '/':
        if (peek_next(lexer) == '*') {
          advance(lexer);
          advance(lexer);
          while (!is_eof(lexer)) {
            if (peek(lexer) == '*' && peek_next(lexer) == '/') {
              advance(lexer);
              advance(lexer);
              break;
            }
            if (peek(lexer) == '\n') {
              lexer->line++;
              lexer->line_start = lexer->current + 1;
            }
            advance(lexer);
          }
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static Token lex_number(Lexer *lexer) {
  const char *start = lexer->current;
  int32_t line = lexer->line;
  int32_t column = get_column(lexer, start);

  while (isdigit(peek(lexer))) {
    advance(lexer);
  }

  if (isalpha(peek(lexer)) || peek(lexer) == '.') {
    lexer_error(lexer, lexer->current,
                "invalid suffix on integer constant");
    while (isalnum(peek(lexer)) || peek(lexer) == '.') {
      advance(lexer);
    }
  }

  Token token = token_create(TOKEN_NUMBER, start,
                             lexer->current - start, line, column);

  char *endptr;
  errno = 0;
  long val = strtol(start, &endptr, 10);

  if (errno == ERANGE || val > INT32_MAX || val < INT32_MIN) {
    lexer_error(lexer, start, "integer constant is too large");
    token.value.int_value = 0;
  } else {
    token.value.int_value = (int32_t) val;
  }

  return token;
}

static Token lex_identifier(Lexer *lexer) {
  const char *start = lexer->current;
  int32_t line = lexer->line;
  int32_t column = get_column(lexer, start);

  while (isalnum(peek(lexer)) || peek(lexer) == '_') {
    advance(lexer);
  }

  size_t length = lexer->current - start;
  TokenKind kind = token_check_keyword(start, length);

  return token_create(kind, start, length, line, column);
}

static Token lex_char(Lexer *lexer) {
  const char *start = lexer->current;
  int32_t line = lexer->line;
  int32_t column = get_column(lexer, start);

  advance(lexer);

  char value = 0;
  if (peek(lexer) == '\\') {
    advance(lexer);
    switch (peek(lexer)) {
      case 'n': value = '\n';
        break;
      case 't': value = '\t';
        break;
      case 'r': value = '\r';
        break;
      case '0': value = '\0';
        break;
      case '\\': value = '\\';
        break;
      case '\'': value = '\'';
        break;
      default:
        lexer_error(lexer, lexer->current - 1,
                    "unknown escape sequence '\\%c'", peek(lexer));
        value = peek(lexer);
    }
    advance(lexer);
  } else {
    value = advance(lexer);
  }

  if (!match(lexer, '\'')) {
    lexer_error(lexer, start, "unterminated character literal");
  }

  Token token = token_create(TOKEN_CHAR_LITERAL, start,
                             lexer->current - start, line, column);
  token.value.char_value = value;
  return token;
}

static Token lex_string(Lexer *lexer) {
  const char *start = lexer->current;
  int32_t line = lexer->line;
  int32_t column = get_column(lexer, start);

  advance(lexer);

  char *buffer = malloc(256);
  size_t capacity = 256;
  size_t length = 0;

  while (peek(lexer) != '"' && !is_eof(lexer)) {
    if (length >= capacity - 1) {
      capacity *= 2;
      buffer = realloc(buffer, capacity);
    }

    if (peek(lexer) == '\\') {
      advance(lexer);
      char escaped = peek(lexer);
      switch (escaped) {
        case 'n': buffer[length++] = '\n';
          break;
        case 't': buffer[length++] = '\t';
          break;
        case 'r': buffer[length++] = '\r';
          break;
        case '0': buffer[length++] = '\0';
          break;
        case '\\': buffer[length++] = '\\';
          break;
        case '"': buffer[length++] = '"';
          break;
        default: buffer[length++] = escaped;
          break;
      }
      advance(lexer);
    } else {
      if (peek(lexer) == '\n') {
        lexer_error(lexer, start, "unterminated string literal");
        break;
      }
      buffer[length++] = advance(lexer);
    }
  }

  if (!match(lexer, '"')) {
    lexer_error(lexer, start, "unterminated string literal");
  }

  buffer[length] = '\0';

  Token token = token_create(TOKEN_STRING_LITERAL, start,
                             lexer->current - start, line, column);
  token.value.string_value = buffer;
  return token;
}

static Token lex_punctuator(Lexer *lexer) {
  const char *start = lexer->current;
  int32_t line = lexer->line;
  int32_t column = get_column(lexer, start);

  struct {
    const char *str;
    size_t len;
    TokenKind kind;
  } punct_table[] = {
#define PUNCT(t, s) { s, sizeof(s) - 1, TOKEN_##t },
#include "lexer/tokens.def"
    {NULL, 0, TOKEN_UNKNOWN}
  };

  for (size_t i = 0; punct_table[i].str != NULL; i++) {
    if (strncmp(lexer->current, punct_table[i].str, punct_table[i].len) == 0) {
      lexer->current += punct_table[i].len;
      return token_create(punct_table[i].kind, start,
                          punct_table[i].len, line, column);
    }
  }

  char c = peek(lexer);
  if (c == '@' || c == '$' || c == '`') {
    lexer_error(lexer, lexer->current,
                "invalid character '%c'", c);
  } else if (isprint(c)) {
    lexer_error(lexer, lexer->current,
                "unexpected character '%c'", c);
  } else {
    lexer_error(lexer, lexer->current,
                "unexpected character with code 0x%02X", (unsigned char) c);
  }

  advance(lexer);
  return token_create(TOKEN_UNKNOWN, start, 1, line, column);
}

void lexer_init(Lexer *lexer, const char *source, const char *filename) {
  lexer->source = source;
  lexer->current = source;
  lexer->line_start = source;
  lexer->line = 1;
  lexer->filename = filename;
  lexer->had_error = 0;
  token_array_init(&lexer->tokens);
}

int32_t lexer_tokenize(Lexer *lexer) {
  while (!is_eof(lexer)) {
    skip_whitespace(lexer);

    if (is_eof(lexer)) break;

    char c = peek(lexer);
    Token token;

    if (isdigit(c)) {
      token = lex_number(lexer);
    } else if (isalpha(c) || c == '_') {
      token = lex_identifier(lexer);
    } else if (c == '\'') {
      token = lex_char(lexer);
    } else if (c == '"') {
      token = lex_string(lexer);
    } else {
      token = lex_punctuator(lexer);
    }

    token_array_push(&lexer->tokens, token);
  }

  Token eof = token_create(TOKEN_EOF, lexer->current, 0,
                           lexer->line, get_column(lexer, lexer->current));
  token_array_push(&lexer->tokens, eof);
#if defined(DEBUG)
  lexer_print_tokens(lexer);
#endif
  return 0;
}

const TokenArray *lexer_get_tokens(const Lexer *lexer) {
  return &lexer->tokens;
}

void lexer_destroy(Lexer *lexer) {
  token_array_destroy(&lexer->tokens);
}

void lexer_print_tokens(const Lexer *lexer) {
  printf("tokens (%zu):\n", lexer->tokens.count);
  for (size_t i = 0; i < lexer->tokens.count; i++) {
    printf("[%3zu] ", i);
    token_print(&lexer->tokens.tokens[i]);
    printf("\n");
  }
}

int32_t lexer_had_error(const Lexer *lexer) {
  return lexer->had_error;
}
