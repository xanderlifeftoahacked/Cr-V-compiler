#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lexer/token.h"

typedef enum {
  AST_TYPE_INT,
  AST_TYPE_CHAR,
  AST_TYPE_ARRAY
} AstTypeKind;

typedef struct {
  AstTypeKind kind;
  AstTypeKind element_kind;
  int32_t array_size;
} AstType;

typedef enum {
  AST_NODE_FUNCTION,
  AST_NODE_BLOCK,
  AST_NODE_RETURN_STMT,
  AST_NODE_EXPR_STMT,
  AST_NODE_VAR_DECL,
  AST_NODE_IF_STMT,
  AST_NODE_WHILE_STMT,
  AST_NODE_BREAK_STMT,
  AST_NODE_BINARY_EXPR,
  AST_NODE_UNARY_EXPR,
  AST_NODE_INT_LITERAL,
  AST_NODE_IDENTIFIER,
  AST_NODE_SUBSCRIPT_EXPR,
  AST_NODE_CALL_EXPR,
  AST_NODE_INIT_LIST
} AstNodeKind;

typedef struct AstNode AstNode;

typedef struct {
  AstNode **items;
  size_t count;
  size_t capacity;
} AstNodeVector;

typedef struct {
  AstType type;
  char *name;
  size_t length;
} AstParam;

typedef struct {
  AstParam *items;
  size_t count;
  size_t capacity;
} AstParamVector;

typedef struct {
  const char *name;
  size_t length;
  AstType return_type;
  AstNode *body;
  AstParamVector params;
} AstFunction;

typedef struct {
  AstFunction **items;
  size_t count;
  size_t capacity;
} AstFunctionVector;

typedef struct {
  AstFunctionVector functions;
} AstModule;

struct AstNode {
  AstNodeKind kind;
  int32_t line;
  int32_t column;

  union {
    struct {
      AstNodeVector statements;
    } block;

    struct {
      AstNode *expr;
    } return_stmt;

    struct {
      AstNode *expr;
    } expr_stmt;

    struct {
      AstType type;
      char *name;
      size_t length;
      AstNode *initializer;
    } var_decl;

    struct {
      AstNode *condition;
      AstNode *then_branch;
      AstNode *else_branch;
    } if_stmt;

    struct {
      AstNode *condition;
      AstNode *body;
    } while_stmt;

    struct {
      int32_t unused;
    } break_stmt;

    struct {
      AstNode *left;
      AstNode *right;
      TokenKind op;
    } binary;

    struct {
      AstNode *operand;
      TokenKind op;
    } unary;

    struct {
      int32_t value;
    } int_literal;

    struct {
      char *name;
      size_t length;
    } identifier;

    struct {
      AstNode *base;
      AstNode *index;
    } subscript;

    struct {
      AstNode *callee;
      AstNodeVector args;
    } call;

    struct {
      AstNodeVector elements;
    } init_list;
  } data;
};
