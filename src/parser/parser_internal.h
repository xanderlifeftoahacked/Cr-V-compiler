#pragma once

#include <stddef.h>
#include <stdint.h>

#include "parser/parser.h"

void *parser_alloc(Parser *parser, size_t size);
char *parser_copy_lexeme(Parser *parser, const Token *token);
AstNode *parser_new_node(Parser *parser, AstNodeKind kind, const Token *token);
void node_vector_push(Parser *parser, AstNodeVector *vec, AstNode *node);
void function_vector_push(Parser *parser, AstFunctionVector *vec, AstFunction *fn);
void param_vector_push(Parser *parser, AstParamVector *vec, AstParam param);

const Token *parser_peek(const Parser *parser);
const Token *parser_peek_next(const Parser *parser);
const Token *parser_previous(const Parser *parser);
int32_t parser_is_at_end(const Parser *parser);
const Token *parser_advance(Parser *parser);
int32_t parser_check(const Parser *parser, TokenKind kind);
int32_t parser_match(Parser *parser, TokenKind kind);
void parser_error_at(Parser *parser, const Token *token, const char *fmt, ...);
const Token *parser_expect(Parser *parser, TokenKind kind, const char *message);
void parser_sync(Parser *parser);

int32_t parser_parse_type(Parser *parser, AstType *type);
int32_t parser_parse_pointer_prefix(Parser *parser, AstType *type);
AstFunction *parse_function(Parser *parser);
AstFunction *parse_function_after_header(Parser *parser, AstType return_type, const Token *name_tok,
                                         AstStorageClass storage);
AstNode *parse_variable_declaration(Parser *parser, const Token *type_token, AstType type);
AstNode *parse_variable_declaration_after_name(Parser *parser, const Token *type_token, AstType type,
                                               const Token *name_tok, AstStorageClass storage);
AstNode *parse_block(Parser *parser);
AstNode *parse_statement(Parser *parser);
AstNode *parse_expression(Parser *parser);
AstNode *parse_initializer(Parser *parser);
