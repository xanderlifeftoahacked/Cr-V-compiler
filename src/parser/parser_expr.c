#include "parser_internal.h"

#include <string.h>

static AstNode *parse_assignment(Parser *parser);
static AstNode *parse_left_associative(Parser *parser, AstNode *(*next)(Parser *), const TokenKind *ops,
                                       size_t op_count);
static AstNode *parse_logical_or(Parser *parser);
static AstNode *parse_logical_and(Parser *parser);
static AstNode *parse_bitwise_or(Parser *parser);
static AstNode *parse_bitwise_xor(Parser *parser);
static AstNode *parse_bitwise_and(Parser *parser);
static AstNode *parse_equality(Parser *parser);
static AstNode *parse_relational(Parser *parser);
static AstNode *parse_shift(Parser *parser);
static AstNode *parse_additive(Parser *parser);
static AstNode *parse_multiplicative(Parser *parser);
static AstNode *parse_unary(Parser *parser);
static AstNode *parse_postfix(Parser *parser);
static AstNode *parse_primary(Parser *parser);

static AstNode *make_binary(Parser *parser, TokenKind op, const Token *token, AstNode *left, AstNode *right) {
  AstNode *node = parser_new_node(parser, AST_NODE_BINARY_EXPR, token);
  node->data.binary.left = left;
  node->data.binary.right = right;
  node->data.binary.op = op;
  return node;
}

static int32_t is_assignment_operator(TokenKind kind) {
  switch (kind) {
    case TOKEN_ASSIGN:
    case TOKEN_PLUS_ASSIGN:
    case TOKEN_MINUS_ASSIGN:
    case TOKEN_STAR_ASSIGN:
    case TOKEN_DIV_ASSIGN:
    case TOKEN_MOD_ASSIGN:
    case TOKEN_AMPERSAND_ASSIGN:
    case TOKEN_PIPE_ASSIGN:
    case TOKEN_CARET_ASSIGN:
    case TOKEN_LSHIFT_ASSIGN:
    case TOKEN_RSHIFT_ASSIGN:
      return 1;
    default:
      return 0;
  }
}

static AstNode *make_unary(Parser *parser, TokenKind op, const Token *token, AstNode *operand, int32_t is_postfix) {
  AstNode *node = parser_new_node(parser, AST_NODE_UNARY_EXPR, token);
  node->data.unary.op = op;
  node->data.unary.operand = operand;
  node->data.unary.is_postfix = is_postfix;
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

static AstNode *make_string_literal(Parser *parser, const Token *token) {
  AstNode *node = parser_new_node(parser, AST_NODE_STRING_LITERAL, token);
  node->data.string_literal.value = parser_alloc(parser, token->string_length + 1);
  if (token->string_length > 0) {
    memcpy(node->data.string_literal.value, token->value.string_value, token->string_length);
  }
  node->data.string_literal.value[token->string_length] = '\0';
  node->data.string_literal.length = token->string_length;
  return node;
}

AstNode *parse_expression(Parser *parser) {
  return parse_assignment(parser);
}

static AstNode *parse_assignment(Parser *parser) {
  AstNode *left = parse_logical_or(parser);

  if (is_assignment_operator(parser_peek(parser)->kind)) {
    parser_advance(parser);
    const Token *op = parser_previous(parser);
    AstNode *right = parse_assignment(parser);
    return make_binary(parser, op->kind, op, left, right);
  }

  return left;
}

static AstNode *parse_logical_or(Parser *parser) {
  const TokenKind ops[] = {TOKEN_LOGICAL_OR};
  return parse_left_associative(parser, parse_logical_and, ops, 1);
}

static AstNode *parse_logical_and(Parser *parser) {
  const TokenKind ops[] = {TOKEN_LOGICAL_AND};
  return parse_left_associative(parser, parse_bitwise_or, ops, 1);
}

static AstNode *parse_left_associative(Parser *parser, AstNode *(*next)(Parser *), const TokenKind *ops,
                                       size_t op_count) {
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
  return parse_left_associative(parser, parse_bitwise_xor, ops, 1);
}

static AstNode *parse_bitwise_xor(Parser *parser) {
  const TokenKind ops[] = {TOKEN_CARET};
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
  return parse_left_associative(parser, parse_shift, ops, 4);
}

static AstNode *parse_shift(Parser *parser) {
  const TokenKind ops[] = {TOKEN_LSHIFT, TOKEN_RSHIFT};
  return parse_left_associative(parser, parse_additive, ops, 2);
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
  if (parser_match(parser, TOKEN_PLUS_PLUS) || parser_match(parser, TOKEN_MINUS_MINUS)) {
    const Token *op = parser_previous(parser);
    AstNode *operand = parse_unary(parser);
    return make_unary(parser, op->kind, op, operand, 0);
  }

  if (parser_match(parser, TOKEN_MINUS) || parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_EXCLAIM) ||
      parser_match(parser, TOKEN_TILDE) || parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_AMPERSAND)) {
    const Token *op = parser_previous(parser);
    AstNode *operand = parse_unary(parser);

    return make_unary(parser, op->kind, op, operand, 0);
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

    if (parser_match(parser, TOKEN_PLUS_PLUS) || parser_match(parser, TOKEN_MINUS_MINUS)) {
      const Token *op = parser_previous(parser);
      expr = make_unary(parser, op->kind, op, expr, 1);
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
    return make_string_literal(parser, parser_previous(parser));
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

AstNode *parse_initializer(Parser *parser) {
  if (parser_match(parser, TOKEN_LBRACE)) {
    const Token *lbrace = parser_previous(parser);
    AstNode *node = parser_new_node(parser, AST_NODE_INIT_LIST, lbrace);

    node->data.init_list.elements.items = NULL;
    node->data.init_list.elements.count = 0;
    node->data.init_list.elements.capacity = 0;

    if (!parser_check(parser, TOKEN_RBRACE)) {
      while (1) {
        AstNode *elem = parse_initializer(parser);

        if (!elem) {
          return NULL;
        }

        node_vector_push(parser, &node->data.init_list.elements, elem);

        if (parser_match(parser, TOKEN_COMMA)) {
          if (parser_check(parser, TOKEN_RBRACE)) {
            parser_error_at(parser, parser_peek(parser), "expected initializer after ','");
            return NULL;
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
