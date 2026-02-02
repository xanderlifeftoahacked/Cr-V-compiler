#include "parser/parser.h"
#include "utils/diagnostic.h"
#include "utils/attributes.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void *parser_alloc(Parser *parser, const size_t size) {
  void *ptr = calloc(1, size);
  arena_track(parser, ptr);
  return ptr;
}

static char *parser_copy_lexeme(Parser *parser, const Token *token) {
  char *buf = parser_alloc(parser, token->length + 1);
  memcpy(buf, token->start, token->length);
  buf[token->length] = '\0';
  return buf;
}

static AstNode *parser_new_node(Parser *parser, const AstNodeKind kind, const Token *token) {
  AstNode *node = parser_alloc(parser, sizeof(AstNode));
  node->kind = kind;
  node->line = token ? token->line : 0;
  node->column = token ? token->column : 0;
  return node;
}

static void node_vector_push(Parser *parser, AstNodeVector *vec, AstNode *node) {
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

static void function_vector_push(Parser *parser, AstFunctionVector *vec, AstFunction *fn) {
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

static void param_vector_push(Parser *parser, AstParamVector *vec, const AstParam param) {
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

static INLINE const Token *parser_peek(const Parser *parser) {
  if (parser->current >= parser->tokens->count) {
    return &parser->tokens->tokens[parser->tokens->count - 1];
  }
  return &parser->tokens->tokens[parser->current];
}

static INLINE const Token *parser_previous(const Parser *parser) {
  if (parser->current == 0) {
    return parser_peek(parser);
  }
  return &parser->tokens->tokens[parser->current - 1];
}

static INLINE int32_t parser_is_at_end(const Parser *parser) {
  return parser_peek(parser)->kind == TOKEN_EOF;
}

static INLINE const Token *parser_advance(Parser *parser) {
  if (!parser_is_at_end(parser)) {
    parser->current++;
  }
  return parser_previous(parser);
}

static INLINE int32_t parser_check(const Parser *parser, const TokenKind kind) {
  if (parser_is_at_end(parser)) {
    return 0;
  }
  return parser_peek(parser)->kind == kind;
}

static INLINE int32_t parser_match(Parser *parser, const TokenKind kind) {
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

static void parser_error_at(Parser *parser, const Token *token, const char *fmt, ...) {
  parser->had_error = 1;
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  SourceLocation loc = {
    .filename = parser->filename,
    .line = token ? token->line : 0,
    .column = token ? token->column : 0,
    .source_line = parser_get_source_line(parser, token)
  };
  diagnostic_log(DIAG_LEVEL_ERROR, loc, "%s", message);
}

static const Token *parser_expect(Parser *parser, const TokenKind kind, const char *message) {
  if (parser_check(parser, kind)) {
    return parser_advance(parser);
  }
  parser_error_at(parser, parser_peek(parser), "%s", message);
  return NULL;
}

static void parser_sync(Parser *parser) {
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
      case TOKEN_KW_break:
        return;
      default:
        parser_advance(parser);
    }
  }
}

static int32_t parser_parse_type(Parser *parser, AstType *type) {
  type->kind = AST_TYPE_INT;
  type->element_kind = AST_TYPE_INT;
  type->array_size = 0;
  if (parser_match(parser, TOKEN_KW_int)) {
    type->kind = AST_TYPE_INT;
    type->element_kind = AST_TYPE_INT;
    return 1;
  }
  if (parser_match(parser, TOKEN_KW_char)) {
    type->kind = AST_TYPE_CHAR;
    type->element_kind = AST_TYPE_CHAR;
    return 1;
  }
  parser_error_at(parser, parser_peek(parser), "expected type specifier");
  return 0;
}

static AstNode *parse_block(Parser *parser);

static AstNode *parse_statement(Parser *parser);

static AstNode *parse_expression(Parser *parser);

static AstNode *parse_assignment(Parser *parser);

static AstNode *parse_bitwise_or(Parser *parser);

static AstNode *parse_bitwise_and(Parser *parser);

static AstNode *parse_equality(Parser *parser);

static AstNode *parse_relational(Parser *parser);

static AstNode *parse_additive(Parser *parser);

static AstNode *parse_multiplicative(Parser *parser);

static AstNode *parse_unary(Parser *parser);

static AstNode *parse_postfix(Parser *parser);

static AstNode *parse_primary(Parser *parser);

static AstNode *parse_initializer(Parser *parser);

static AstNode *parse_if_statement(Parser *parser, const Token *kw);

static AstNode *parse_while_statement(Parser *parser, const Token *kw);

void parser_init(Parser *parser, const TokenArray *tokens, const char *source, const char *filename) {
  parser->tokens = tokens;
  parser->current = 0;
  parser->filename = filename;
  parser->source_begin = source;
  parser->had_error = 0;
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

static AstFunction *parse_function(Parser *parser) {
  AstType return_type;
  if (!parser_parse_type(parser, &return_type)) {
    return NULL;
  }
  const Token *name_tok = parser_expect(parser, TOKEN_IDENTIFIER, "expected function name");
  if (!name_tok) {
    return NULL;
  }
  if (!parser_expect(parser, TOKEN_LPAREN, "expected '('")) {
    return NULL;
  }
  AstParamVector params = {0};
  if (!parser_check(parser, TOKEN_RPAREN)) {
    while (1) {
      AstType param_type;
      if (!parser_parse_type(parser, &param_type)) {
        return NULL;
      }
      const Token *param_name = parser_expect(parser, TOKEN_IDENTIFIER, "expected parameter name");
      if (!param_name) {
        return NULL;
      }
      AstParam param = {
        .type = param_type,
        .name = parser_copy_lexeme(parser, param_name),
        .length = param_name->length
      };
      param_vector_push(parser, &params, param);
      if (parser_match(parser, TOKEN_COMMA)) {
        continue;
      }
      break;
    }
  }
  if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
    return NULL;
  }
  AstNode *body = parse_block(parser);
  if (!body) {
    return NULL;
  }
  AstFunction *fn = parser_alloc(parser, sizeof(AstFunction));
  fn->name = parser_copy_lexeme(parser, name_tok);
  fn->length = name_tok->length;
  fn->return_type = return_type;
  fn->body = body;
  fn->params = params;
  return fn;
}

static int32_t parse_array_suffix(Parser *parser, AstType *type) {
  if (!parser_match(parser, TOKEN_LBRACKET)) {
    return 0;
  }
  const Token *size_tok = parser_expect(parser, TOKEN_NUMBER, "expected array size");
  if (!size_tok) {
    parser_expect(parser, TOKEN_RBRACKET, "expected ']' after array size");
    return -1;
  }
  parser_expect(parser, TOKEN_RBRACKET, "expected ']' after array size");
  type->element_kind = type->kind;
  type->kind = AST_TYPE_ARRAY;
  type->array_size = size_tok->value.int_value;
  return 1;
}

static AstNode *parse_variable_declaration(Parser *parser, const Token *type_token, AstType type) {
  const Token *name_tok = parser_expect(parser, TOKEN_IDENTIFIER, "expected identifier");
  if (!name_tok) {
    return NULL;
  }
  int32_t array_status = parse_array_suffix(parser, &type);
  if (array_status < 0) {
    return NULL;
  }
  AstNode *initializer = NULL;
  if (parser_match(parser, TOKEN_ASSIGN)) {
    initializer = parse_initializer(parser);
    if (!initializer) {
      return NULL;
    }
  }
  if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';'")) {
    return NULL;
  }
  AstNode *node = parser_new_node(parser, AST_NODE_VAR_DECL, type_token);
  node->data.var_decl.type = type;
  node->data.var_decl.name = parser_copy_lexeme(parser, name_tok);
  node->data.var_decl.length = name_tok->length;
  node->data.var_decl.initializer = initializer;
  return node;
}

static AstNode *parse_statement(Parser *parser) {
  if (parser_check(parser, TOKEN_LBRACE)) {
    return parse_block(parser);
  }
  if (parser_match(parser, TOKEN_KW_if)) {
    return parse_if_statement(parser, parser_previous(parser));
  }
  if (parser_match(parser, TOKEN_KW_else)) {
    parser_error_at(parser, parser_previous(parser), "unexpected 'else'");
    return NULL;
  }
  if (parser_match(parser, TOKEN_KW_while)) {
    return parse_while_statement(parser, parser_previous(parser));
  }
  if (parser_match(parser, TOKEN_KW_break)) {
    const Token *kw = parser_previous(parser);
    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';'")) {
      return NULL;
    }
    AstNode *node = parser_new_node(parser, AST_NODE_BREAK_STMT, kw);
    node->data.break_stmt.unused = 0;
    return node;
  }
  if (parser_match(parser, TOKEN_KW_return)) {
    const Token *kw = parser_previous(parser);
    AstNode *expr = parse_expression(parser);
    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';'")) {
      return NULL;
    }
    AstNode *node = parser_new_node(parser, AST_NODE_RETURN_STMT, kw);
    node->data.return_stmt.expr = expr;
    return node;
  }
  if (parser_check(parser, TOKEN_KW_int) || parser_check(parser, TOKEN_KW_char)) {
    const Token *type_token = parser_peek(parser);
    AstType type;
    parser_parse_type(parser, &type);
    return parse_variable_declaration(parser, type_token, type);
  }
  AstNode *expr = parse_expression(parser);
  if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';'")) {
    return NULL;
  }
  AstNode *node = parser_new_node(parser, AST_NODE_EXPR_STMT, parser_previous(parser));
  node->data.expr_stmt.expr = expr;
  return node;
}

static AstNode *parse_block(Parser *parser) {
  const Token *lbrace = parser_expect(parser, TOKEN_LBRACE, "expected '{'");
  if (!lbrace) {
    return NULL;
  }
  AstNode *node = parser_new_node(parser, AST_NODE_BLOCK, lbrace);
  node->data.block.statements.items = NULL;
  node->data.block.statements.count = 0;
  node->data.block.statements.capacity = 0;
  while (!parser_check(parser, TOKEN_RBRACE) && !parser_is_at_end(parser)) {
    AstNode *stmt = parse_statement(parser);
    if (!stmt) {
      parser_sync(parser);
      continue;
    }
    node_vector_push(parser, &node->data.block.statements, stmt);
  }
  parser_expect(parser, TOKEN_RBRACE, "expected '}'");
  return node;
}

static AstNode *parse_if_statement(Parser *parser, const Token *kw) {
  if (!parser_expect(parser, TOKEN_LPAREN, "expected '('")) {
    return NULL;
  }
  AstNode *condition = parse_expression(parser);
  if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
    return NULL;
  }
  AstNode *then_branch = parse_statement(parser);
  if (!then_branch) {
    return NULL;
  }
  AstNode *else_branch = NULL;
  if (parser_match(parser, TOKEN_KW_else)) {
    else_branch = parse_statement(parser);
    if (!else_branch) {
      return NULL;
    }
  }
  AstNode *node = parser_new_node(parser, AST_NODE_IF_STMT, kw);
  node->data.if_stmt.condition = condition;
  node->data.if_stmt.then_branch = then_branch;
  node->data.if_stmt.else_branch = else_branch;
  return node;
}

static AstNode *parse_while_statement(Parser *parser, const Token *kw) {
  if (!parser_expect(parser, TOKEN_LPAREN, "expected '('")) {
    return NULL;
  }
  AstNode *condition = parse_expression(parser);
  if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
    return NULL;
  }
  AstNode *body = parse_statement(parser);
  if (!body) {
    return NULL;
  }
  AstNode *node = parser_new_node(parser, AST_NODE_WHILE_STMT, kw);
  node->data.while_stmt.condition = condition;
  node->data.while_stmt.body = body;
  return node;
}

static AstNode *make_binary(Parser *parser, const TokenKind op, const Token *token, AstNode *left, AstNode *right) {
  AstNode *node = parser_new_node(parser, AST_NODE_BINARY_EXPR, token);
  node->data.binary.left = left;
  node->data.binary.right = right;
  node->data.binary.op = op;
  return node;
}

static AstNode *make_unary(Parser *parser, const TokenKind op, const Token *token, AstNode *operand) {
  AstNode *node = parser_new_node(parser, AST_NODE_UNARY_EXPR, token);
  node->data.unary.op = op;
  node->data.unary.operand = operand;
  return node;
}

static AstNode *make_int_literal(Parser *parser, const Token *token) {
  AstNode *node = parser_new_node(parser, AST_NODE_INT_LITERAL, token);
  node->data.int_literal.value = token->value.int_value;
  return node;
}

static AstNode *make_identifier(Parser *parser, const Token *token) {
  AstNode *node = parser_new_node(parser, AST_NODE_IDENTIFIER, token);
  node->data.identifier.name = parser_copy_lexeme(parser, token);
  node->data.identifier.length = token->length;
  return node;
}

static AstNode *parse_expression(Parser *parser) {
  return parse_assignment(parser);
}

static AstNode *parse_assignment(Parser *parser) {
  AstNode *left = parse_bitwise_or(parser);
  if (parser_match(parser, TOKEN_ASSIGN)) {
    const Token *op = parser_previous(parser);
    AstNode *right = parse_assignment(parser);
    return make_binary(parser, TOKEN_ASSIGN, op, left, right);
  }
  return left;
}

static AstNode *parse_left_associative(Parser *parser, AstNode *(*next)(Parser *), const TokenKind *ops,
                                       const size_t op_count) {
  AstNode *expr = next(parser);
  while (1) {
    int matched = 0;
    for (size_t i = 0; i < op_count; i++) {
      if (parser_match(parser, ops[i])) {
        const Token *op = parser_previous(parser);
        AstNode *right = next(parser);
        expr = make_binary(parser, ops[i], op, expr, right);
        matched = 1;
        break;
      }
    }
    if (!matched) {
      break;
    }
  }
  return expr;
}

static AstNode *parse_bitwise_or(Parser *parser) {
  const TokenKind ops[] = {TOKEN_PIPE};
  return parse_left_associative(parser, parse_bitwise_and, ops, 1);
}

static AstNode *parse_bitwise_and(Parser *parser) {
  const TokenKind ops[] = {TOKEN_AMPERSAND};
  return parse_left_associative(parser, parse_equality, ops, 1);
}

static AstNode *parse_equality(Parser *parser) {
  const TokenKind ops[] = {TOKEN_EQUAL, TOKEN_NOT_EQUAL};
  return parse_left_associative(parser, parse_relational, ops, 2);
}

static AstNode *parse_relational(Parser *parser) {
  const TokenKind ops[] = {TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL};
  return parse_left_associative(parser, parse_additive, ops, 4);
}

static AstNode *parse_additive(Parser *parser) {
  const TokenKind ops[] = {TOKEN_PLUS, TOKEN_MINUS};
  return parse_left_associative(parser, parse_multiplicative, ops, 2);
}

static AstNode *parse_multiplicative(Parser *parser) {
  const TokenKind ops[] = {TOKEN_STAR, TOKEN_DIV, TOKEN_MOD};
  return parse_left_associative(parser, parse_unary, ops, 3);
}

static AstNode *parse_unary(Parser *parser) {
  if (parser_match(parser, TOKEN_MINUS) || parser_match(parser, TOKEN_PLUS) ||
      parser_match(parser, TOKEN_EXCLAIM) || parser_match(parser, TOKEN_TILDE)) {
    const Token *op = parser_previous(parser);
    AstNode *operand = parse_unary(parser);
    return make_unary(parser, op->kind, op, operand);
  }
  return parse_postfix(parser);
}

static AstNode *parse_postfix(Parser *parser) {
  AstNode *expr = parse_primary(parser);
  while (1) {
    if (parser_match(parser, TOKEN_LBRACKET)) {
      const Token *lbracket = parser_previous(parser);
      AstNode *index = parse_expression(parser);
      if (!parser_expect(parser, TOKEN_RBRACKET, "expected ']'")) {
        return NULL;
      }
      AstNode *node = parser_new_node(parser, AST_NODE_SUBSCRIPT_EXPR, lbracket);
      node->data.subscript.base = expr;
      node->data.subscript.index = index;
      expr = node;
      continue;
    }
    if (parser_match(parser, TOKEN_LPAREN)) {
      const Token *lparen = parser_previous(parser);
      AstNodeVector args = {0};
      if (!parser_check(parser, TOKEN_RPAREN)) {
        while (1) {
          AstNode *arg = parse_expression(parser);
          if (!arg) {
            return NULL;
          }
          node_vector_push(parser, &args, arg);
          if (parser_match(parser, TOKEN_COMMA)) {
            continue;
          }
          break;
        }
      }
      if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
        return NULL;
      }
      AstNode *node = parser_new_node(parser, AST_NODE_CALL_EXPR, lparen);
      node->data.call.callee = expr;
      node->data.call.args = args;
      expr = node;
      continue;
    }
    break;
  }
  return expr;
}

static AstNode *parse_primary(Parser *parser) {
  if (parser_match(parser, TOKEN_NUMBER)) {
    return make_int_literal(parser, parser_previous(parser));
  }
  if (parser_match(parser, TOKEN_CHAR_LITERAL)) {
    const Token *tok = parser_previous(parser);
    AstNode *node = parser_new_node(parser, AST_NODE_INT_LITERAL, tok);
    node->data.int_literal.value = tok->value.char_value;
    return node;
  }
  if (parser_match(parser, TOKEN_IDENTIFIER)) {
    return make_identifier(parser, parser_previous(parser));
  }
  if (parser_match(parser, TOKEN_LPAREN)) {
    AstNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN, "expected ')'");
    return expr;
  }
  if (parser_match(parser, TOKEN_STRING_LITERAL)) {
    parser_error_at(parser, parser_previous(parser), "string literals are currently not supported");
  } else {
    parser_error_at(parser, parser_peek(parser), "expected expression");
    if (!parser_is_at_end(parser)) {
      parser_advance(parser);
    }
  }
  AstNode *node = parser_new_node(parser, AST_NODE_INT_LITERAL, parser_previous(parser));
  node->data.int_literal.value = 0;
  return node;
}

static AstNode *parse_initializer(Parser *parser) {
  if (parser_match(parser, TOKEN_LBRACE)) {
    const Token *lbrace = parser_previous(parser);
    AstNode *node = parser_new_node(parser, AST_NODE_INIT_LIST, lbrace);
    node->data.init_list.elements.items = NULL;
    node->data.init_list.elements.count = 0;
    node->data.init_list.elements.capacity = 0;
    if (!parser_check(parser, TOKEN_RBRACE)) {
      while (1) {
        AstNode *elem = parse_expression(parser);
        if (!elem) {
          return NULL;
        }
        node_vector_push(parser, &node->data.init_list.elements, elem);
        if (parser_match(parser, TOKEN_COMMA)) {
          if (parser_check(parser, TOKEN_RBRACE)) {
            break;
          }
          continue;
        }
        break;
      }
    }
    if (!parser_expect(parser, TOKEN_RBRACE, "expected '}' in initializer list")) {
      return NULL;
    }
    return node;
  }
  return parse_expression(parser);
}

ParseResult parser_parse(Parser *parser) {
  AstModule *module = parser_get_module(parser);
  module->functions.items = NULL;
  module->functions.count = 0;
  module->functions.capacity = 0;
  while (!parser_is_at_end(parser)) {
    AstFunction *fn = parse_function(parser);
    if (!fn) {
      parser_sync(parser);
      if (parser_is_at_end(parser)) {
        break;
      }
      continue;
    }
    function_vector_push(parser, &module->functions, fn);
  }
  ParseResult result = {
    .module = module,
    .had_error = parser->had_error
  };
  return result;
}
