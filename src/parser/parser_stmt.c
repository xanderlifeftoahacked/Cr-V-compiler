#include "parser_internal.h"

static AstNode *parse_if_statement(Parser *parser, const Token *kw);
static AstNode *parse_while_statement(Parser *parser, const Token *kw);
static AstNode *parse_for_statement(Parser *parser, const Token *kw);
static AstNode *parse_do_while_statement(Parser *parser, const Token *kw);
static AstNode *parse_switch_statement(Parser *parser, const Token *kw);

AstNode *parse_statement(Parser *parser) {
  if (parser_check(parser, TOKEN_LBRACE)) {
    return parse_block(parser);
  }

  if (parser_check(parser, TOKEN_IDENTIFIER) && parser_peek_next(parser)->kind == TOKEN_COLON) {
    const Token *label = parser_advance(parser);
    parser_advance(parser);

    AstNode *statement = parse_statement(parser);
    if (!statement) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_LABEL_STMT, label);
    node->data.label_stmt.label = parser_copy_lexeme(parser, label);
    node->data.label_stmt.length = label->length;
    node->data.label_stmt.statement = statement;
    return node;
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

  if (parser_match(parser, TOKEN_KW_for)) {
    return parse_for_statement(parser, parser_previous(parser));
  }

  if (parser_match(parser, TOKEN_KW_do)) {
    return parse_do_while_statement(parser, parser_previous(parser));
  }

  if (parser_match(parser, TOKEN_KW_switch)) {
    return parse_switch_statement(parser, parser_previous(parser));
  }

  if (parser_match(parser, TOKEN_KW_case)) {
    const Token *kw = parser_previous(parser);

    if (parser->switch_depth <= 0) {
      parser_error_at(parser, kw, "unexpected 'case'");
      return NULL;
    }

    AstNode *value = parse_expression(parser);
    if (!parser_expect(parser, TOKEN_COLON, "expected ':' after case value")) {
      return NULL;
    }

    AstNode *statement = parse_statement(parser);
    if (!statement) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_CASE_STMT, kw);
    node->data.case_stmt.value = value;
    node->data.case_stmt.statement = statement;
    return node;
  }

  if (parser_match(parser, TOKEN_KW_default)) {
    const Token *kw = parser_previous(parser);

    if (parser->switch_depth <= 0) {
      parser_error_at(parser, kw, "unexpected 'default'");
      return NULL;
    }

    if (!parser_expect(parser, TOKEN_COLON, "expected ':' after default")) {
      return NULL;
    }

    AstNode *statement = parse_statement(parser);
    if (!statement) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_DEFAULT_STMT, kw);
    node->data.default_stmt.statement = statement;
    return node;
  }

  if (parser_match(parser, TOKEN_KW_break)) {
    const Token *kw = parser_previous(parser);
    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after break")) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_BREAK_STMT, kw);
    return node;
  }

  if (parser_match(parser, TOKEN_KW_continue)) {
    const Token *kw = parser_previous(parser);
    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after continue")) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_CONTINUE_STMT, kw);
    return node;
  }

  if (parser_match(parser, TOKEN_KW_return)) {
    const Token *kw = parser_previous(parser);
    AstNode *expr = parse_expression(parser);

    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after return value")) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_RETURN_STMT, kw);
    node->data.return_stmt.expr = expr;
    return node;
  }

  if (parser_match(parser, TOKEN_KW_goto)) {
    const Token *kw = parser_previous(parser);
    const Token *label = parser_expect(parser, TOKEN_IDENTIFIER, "expected label after goto");
    if (!label) {
      return NULL;
    }

    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after goto")) {
      return NULL;
    }

    AstNode *node = parser_new_node(parser, AST_NODE_GOTO_STMT, kw);
    node->data.goto_stmt.label = parser_copy_lexeme(parser, label);
    node->data.goto_stmt.length = label->length;
    return node;
  }

  if (parser_check(parser, TOKEN_KW_int) || parser_check(parser, TOKEN_KW_char)) {
    const Token *type_token = parser_peek(parser);
    AstType type;
    parser_parse_type(parser, &type);
    return parse_variable_declaration(parser, type_token, type);
  }

  AstNode *expr = parse_expression(parser);
  if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after expression")) {
    return NULL;
  }

  AstNode *node = parser_new_node(parser, AST_NODE_EXPR_STMT, parser_previous(parser));
  node->data.expr_stmt.expr = expr;
  return node;
}

AstNode *parse_block(Parser *parser) {
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

static AstNode *parse_for_statement(Parser *parser, const Token *kw) {
  if (!parser_expect(parser, TOKEN_LPAREN, "expected '('")) {
    return NULL;
  }

  AstNode *init = NULL;
  if (parser_check(parser, TOKEN_KW_int) || parser_check(parser, TOKEN_KW_char)) {
    const Token *type_token = parser_peek(parser);
    AstType type;
    parser_parse_type(parser, &type);
    init = parse_variable_declaration(parser, type_token, type);
    if (!init) {
      return NULL;
    }
  } else if (!parser_match(parser, TOKEN_SEMICOLON)) {
    AstNode *init_expr = parse_expression(parser);
    if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after for init")) {
      return NULL;
    }
    AstNode *init_stmt = parser_new_node(parser, AST_NODE_EXPR_STMT, kw);
    init_stmt->data.expr_stmt.expr = init_expr;
    init = init_stmt;
  }

  AstNode *condition = NULL;
  if (!parser_check(parser, TOKEN_SEMICOLON)) {
    condition = parse_expression(parser);
  }
  if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after for condition")) {
    return NULL;
  }

  AstNode *post = NULL;
  if (!parser_check(parser, TOKEN_RPAREN)) {
    post = parse_expression(parser);
  }
  if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
    return NULL;
  }

  AstNode *body = parse_statement(parser);
  if (!body) {
    return NULL;
  }

  AstNode *node = parser_new_node(parser, AST_NODE_FOR_STMT, kw);
  node->data.for_stmt.init = init;
  node->data.for_stmt.condition = condition;
  node->data.for_stmt.post = post;
  node->data.for_stmt.body = body;
  return node;
}

static AstNode *parse_do_while_statement(Parser *parser, const Token *kw) {
  AstNode *body = parse_statement(parser);
  if (!body) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_KW_while, "expected 'while' after do body")) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_LPAREN, "expected '('")) {
    return NULL;
  }

  AstNode *condition = parse_expression(parser);
  if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_SEMICOLON, "expected ';' after do-while")) {
    return NULL;
  }

  AstNode *node = parser_new_node(parser, AST_NODE_DO_WHILE_STMT, kw);
  node->data.do_while_stmt.body = body;
  node->data.do_while_stmt.condition = condition;
  return node;
}

static AstNode *parse_switch_statement(Parser *parser, const Token *kw) {
  if (!parser_expect(parser, TOKEN_LPAREN, "expected '('")) {
    return NULL;
  }
  AstNode *expr = parse_expression(parser);
  if (!parser_expect(parser, TOKEN_RPAREN, "expected ')'")) {
    return NULL;
  }

  parser->switch_depth++;
  AstNode *body = parse_statement(parser);
  parser->switch_depth--;

  if (!body) {
    return NULL;
  }

  AstNode *node = parser_new_node(parser, AST_NODE_SWITCH_STMT, kw);
  node->data.switch_stmt.expr = expr;
  node->data.switch_stmt.body = body;
  return node;
}
