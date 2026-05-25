#include "parser_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/diagnostic.h"

static void arena_track(Parser *parser, void *ptr) {
  if (!ptr) {
    LOG(FATAL, "out of memory");
  }

  if (parser->arena.count == parser->arena.capacity) {
    size_t new_cap = parser->arena.capacity ? parser->arena.capacity * 2 : 8;
    void **new_items = realloc(parser->arena.items, new_cap * sizeof(void *));

    if (!new_items) {
      LOG(FATAL, "out of memory");
    }

    parser->arena.items = new_items;
    parser->arena.capacity = new_cap;
  }

  parser->arena.items[parser->arena.count++] = ptr;
}

void *parser_alloc(Parser *parser, size_t size) {
  void *ptr = calloc(1, size);
  arena_track(parser, ptr);
  return ptr;
}

char *parser_copy_lexeme(Parser *parser, const Token *token) {
  char *buf = parser_alloc(parser, token->length + 1);
  memcpy(buf, token->start, token->length);
  buf[token->length] = '\0';
  return buf;
}

AstNode *parser_new_node(Parser *parser, AstNodeKind kind, const Token *token) {
  AstNode *node = parser_alloc(parser, sizeof(AstNode));
  node->kind = kind;
  node->line = token ? token->line : 0;
  node->column = token ? token->column : 0;
  return node;
}

void node_vector_push(Parser *parser, AstNodeVector *vec, AstNode *node) {
  if (vec->count == vec->capacity) {
    size_t new_cap = vec->capacity ? vec->capacity * 2 : 4;
    AstNode **new_items = parser_alloc(parser, new_cap * sizeof(AstNode *));

    if (vec->items) {
      memcpy(new_items, vec->items, vec->count * sizeof(AstNode *));
    }

    vec->items = new_items;
    vec->capacity = new_cap;
  }

  vec->items[vec->count++] = node;
}

void function_vector_push(Parser *parser, AstFunctionVector *vec, AstFunction *fn) {
  if (vec->count == vec->capacity) {
    size_t new_cap = vec->capacity ? vec->capacity * 2 : 4;
    AstFunction **new_items = parser_alloc(parser, new_cap * sizeof(AstFunction *));

    if (vec->items) {
      memcpy(new_items, vec->items, vec->count * sizeof(AstFunction *));
    }

    vec->items = new_items;
    vec->capacity = new_cap;
  }

  vec->items[vec->count++] = fn;
}

void param_vector_push(Parser *parser, AstParamVector *vec, AstParam param) {
  if (vec->count == vec->capacity) {
    size_t new_cap = vec->capacity ? vec->capacity * 2 : 4;
    AstParam *new_items = parser_alloc(parser, new_cap * sizeof(AstParam));

    if (vec->items) {
      memcpy(new_items, vec->items, vec->count * sizeof(AstParam));
    }

    vec->items = new_items;
    vec->capacity = new_cap;
  }

  vec->items[vec->count++] = param;
}

const Token *parser_peek(const Parser *parser) {
  if (parser->current >= parser->tokens->count) {
    return &parser->tokens->tokens[parser->tokens->count - 1];
  }
  return &parser->tokens->tokens[parser->current];
}

const Token *parser_peek_next(const Parser *parser) {
  size_t index = parser->current + 1;
  if (index >= parser->tokens->count) {
    return &parser->tokens->tokens[parser->tokens->count - 1];
  }
  return &parser->tokens->tokens[index];
}

const Token *parser_previous(const Parser *parser) {
  if (parser->current == 0) {
    return parser_peek(parser);
  }
  return &parser->tokens->tokens[parser->current - 1];
}

int32_t parser_is_at_end(const Parser *parser) {
  return parser_peek(parser)->kind == TOKEN_EOF;
}

const Token *parser_advance(Parser *parser) {
  if (!parser_is_at_end(parser)) {
    parser->current++;
  }
  return parser_previous(parser);
}

int32_t parser_check(const Parser *parser, TokenKind kind) {
  if (parser_is_at_end(parser)) {
    return 0;
  }
  return parser_peek(parser)->kind == kind;
}

int32_t parser_match(Parser *parser, TokenKind kind) {
  if (parser_check(parser, kind)) {
    parser_advance(parser);
    return 1;
  }
  return 0;
}

static char *parser_get_source_line(Parser *parser, const Token *token) {
  if (!parser || !parser->source_begin || !token || !token->start) {
    return NULL;
  }

  const char *line_start = token->start;
  while (line_start > parser->source_begin && *(line_start - 1) != '\n') {
    line_start--;
  }

  const char *line_end = token->start;
  while (*line_end && *line_end != '\n') {
    line_end++;
  }

  size_t len = (size_t) (line_end - line_start);
  char *buf = parser_alloc(parser, len + 1);
  memcpy(buf, line_start, len);
  buf[len] = '\0';
  return buf;
}

void parser_error_at(Parser *parser, const Token *token, const char *fmt, ...) {
  parser->had_error = 1;

  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  SourceLocation loc = {.filename = parser->filename,
                        .line = token ? token->line : 0,
                        .column = token ? token->column : 0,
                        .source_line = parser_get_source_line(parser, token)};
  diagnostic_log(DIAG_LEVEL_ERROR, loc, "%s", message);
}

const Token *parser_expect(Parser *parser, TokenKind kind, const char *message) {
  if (parser_check(parser, kind)) {
    return parser_advance(parser);
  }
  parser_error_at(parser, parser_peek(parser), "%s", message);
  return NULL;
}

void parser_sync(Parser *parser) {
  while (!parser_is_at_end(parser)) {
    if (parser_previous(parser)->kind == TOKEN_SEMICOLON) {
      return;
    }

    switch (parser_peek(parser)->kind) {
      case TOKEN_KW_int:
      case TOKEN_KW_char:
      case TOKEN_KW_return:
      case TOKEN_KW_if:
      case TOKEN_KW_while:
      case TOKEN_KW_for:
      case TOKEN_KW_do:
      case TOKEN_KW_switch:
      case TOKEN_KW_case:
      case TOKEN_KW_default:
      case TOKEN_KW_break:
      case TOKEN_KW_goto:
        return;
      default:
        parser_advance(parser);
    }
  }
}

void parser_init(Parser *parser, const TokenArray *tokens, const char *source, const char *filename) {
  parser->tokens = tokens;
  parser->current = 0;
  parser->filename = filename;
  parser->source_begin = source;
  parser->had_error = 0;
  parser->switch_depth = 0;
  parser->arena.items = NULL;
  parser->arena.count = 0;
  parser->arena.capacity = 0;
  parser->module = NULL;
}

void parser_destroy(Parser *parser) {
  for (size_t i = 0; i < parser->arena.count; i++) {
    free(parser->arena.items[i]);
  }
  free(parser->arena.items);
  parser->arena.items = NULL;
  parser->arena.count = 0;
  parser->arena.capacity = 0;
  parser->module = NULL;
}

static AstModule *parser_get_module(Parser *parser) {
  if (!parser->module) {
    parser->module = parser_alloc(parser, sizeof(AstModule));
  }
  return parser->module;
}

ParseResult parser_parse(Parser *parser) {
  AstModule *module = parser_get_module(parser);
  module->globals.items = NULL;
  module->globals.count = 0;
  module->globals.capacity = 0;
  module->functions.items = NULL;
  module->functions.count = 0;
  module->functions.capacity = 0;

  while (!parser_is_at_end(parser)) {
    size_t start = parser->current;

    AstStorageClass storage = AST_STORAGE_NONE;
    if (parser_match(parser, TOKEN_KW_static)) {
      storage = AST_STORAGE_STATIC;
    } else if (parser_match(parser, TOKEN_KW_extern)) {
      storage = AST_STORAGE_EXTERN;
    }

    if ((storage == AST_STORAGE_STATIC && parser_check(parser, TOKEN_KW_extern)) ||
        (storage == AST_STORAGE_EXTERN && parser_check(parser, TOKEN_KW_static))) {
      parser_error_at(parser, parser_peek(parser), "cannot combine 'static' and 'extern'");
      parser_sync(parser);

      if (parser->current == start && !parser_is_at_end(parser)) {
        parser_advance(parser);
      }

      continue;
    }

    const Token *type_token = parser_peek(parser);
    AstType type;
    if (!parser_parse_type(parser, &type)) {
      parser_sync(parser);

      if (parser->current == start && !parser_is_at_end(parser)) {
        parser_advance(parser);
      }

      continue;
    }
    if (!parser_parse_pointer_prefix(parser, &type)) {
      parser_sync(parser);

      if (parser->current == start && !parser_is_at_end(parser)) {
        parser_advance(parser);
      }

      continue;
    }

    const Token *name_tok = parser_expect(parser, TOKEN_IDENTIFIER, "expected identifier");
    if (!name_tok) {
      parser_sync(parser);

      if (parser->current == start && !parser_is_at_end(parser)) {
        parser_advance(parser);
      }

      continue;
    }

    if (parser_check(parser, TOKEN_LPAREN)) {
      AstFunction *fn = parse_function_after_header(parser, type, name_tok, storage);

      if (!fn) {
        parser_sync(parser);

        if (parser->current == start && !parser_is_at_end(parser)) {
          parser_advance(parser);
        }

        if (parser_is_at_end(parser)) {
          break;
        }

        continue;
      }

      function_vector_push(parser, &module->functions, fn);
      continue;
    }

    AstNode *global = parse_variable_declaration_after_name(parser, type_token, type, name_tok, storage);
    if (!global) {
      parser_sync(parser);

      if (parser->current == start && !parser_is_at_end(parser)) {
        parser_advance(parser);
      }

      continue;
    }

    node_vector_push(parser, &module->globals, global);
  }

  if (module->globals.count == 0 && module->functions.count == 0) {
    parser_error_at(parser, parser_peek(parser), "expected declaration or function definition");
  }

  ParseResult result = {.module = module, .had_error = parser->had_error};
  return result;
}
