#include "parser_internal.h"

int32_t parser_parse_type(Parser *parser, AstType *type) {
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

int32_t parser_parse_pointer_prefix(Parser *parser, AstType *type) {
  if (!parser_match(parser, TOKEN_STAR)) {
    return 1;
  }

  if (type->kind == AST_TYPE_POINTER) {
    parser_error_at(parser, parser_previous(parser), "multi-level pointers are not supported");
    return 0;
  }

  type->element_kind = type->kind;
  type->kind = AST_TYPE_POINTER;
  type->array_size = 0;
  return 1;
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

  if (type->kind == AST_TYPE_POINTER) {
    parser_error_at(parser, size_tok, "arrays of pointers are not supported");
    return -1;
  }

  type->element_kind = type->kind;
  type->kind = AST_TYPE_ARRAY;
  type->array_size = size_tok->value.int_value;
  return 1;
}

AstFunction *parse_function_after_header(Parser *parser, AstType return_type, const Token *name_tok) {
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
      if (!parser_parse_pointer_prefix(parser, &param_type)) {
        return NULL;
      }
      const Token *param_name = parser_expect(parser, TOKEN_IDENTIFIER, "expected parameter name");
      if (!param_name) {
        return NULL;
      }
      int32_t array_status = parse_array_suffix(parser, &param_type);
      if (array_status < 0) {
        return NULL;
      }
      if (array_status > 0) {
        param_type.kind = AST_TYPE_POINTER;
        param_type.array_size = 0;
      }
      AstParam param = {
          .type = param_type, .name = parser_copy_lexeme(parser, param_name), .length = param_name->length};
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

AstFunction *parse_function(Parser *parser) {
  AstType return_type;
  if (!parser_parse_type(parser, &return_type)) {
    return NULL;
  }

  const Token *name_tok = parser_expect(parser, TOKEN_IDENTIFIER, "expected function name");
  if (!name_tok) {
    return NULL;
  }

  return parse_function_after_header(parser, return_type, name_tok);
}

AstNode *parse_variable_declaration_after_name(Parser *parser, const Token *type_token, AstType type,
                                               const Token *name_tok) {
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

AstNode *parse_variable_declaration(Parser *parser, const Token *type_token, AstType type) {
  if (!parser_parse_pointer_prefix(parser, &type)) {
    return NULL;
  }

  const Token *name_tok = parser_expect(parser, TOKEN_IDENTIFIER, "expected identifier");
  if (!name_tok) {
    return NULL;
  }

  return parse_variable_declaration_after_name(parser, type_token, type, name_tok);
}
