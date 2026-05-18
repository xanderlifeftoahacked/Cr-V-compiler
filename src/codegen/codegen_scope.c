#include "codegen_internal.h"

#include "utils/diagnostic.h"

#include <stdlib.h>

CodegenScope *scope_push(CodegenContext *ctx) {
  CodegenScope *scope = malloc(sizeof(CodegenScope));
  if (!scope) {
    LOG(FATAL, "out of memory");
  }
  scope->symbols = NULL;
  scope->parent = ctx->scope;
  ctx->scope = scope;
  return scope;
}

void scope_pop(CodegenContext *ctx) {
  CodegenScope *scope = ctx->scope;
  if (!scope) {
    return;
  }
  CodegenSymbol *symbol = scope->symbols;
  while (symbol) {
    CodegenSymbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }
  ctx->scope = scope->parent;
  free(scope);
}

CodegenSymbol *scope_find(const CodegenContext *ctx, const char *name, size_t length) {
  for (const CodegenScope *scope = ctx->scope; scope; scope = scope->parent) {
    for (CodegenSymbol *it = scope->symbols; it; it = it->next) {
      if (names_equal(it->name, it->length, name, length)) {
        return it;
      }
    }
  }
  return NULL;
}

static int32_t allocate_local_offset(CodegenContext *ctx, AstType type) {
  ctx->next_local_offset = align_to(ctx->next_local_offset, type_align(type));
  ctx->next_local_offset += type_size(type);
  return -ctx->next_local_offset;
}

CodegenSymbol *scope_declare_current(CodegenContext *ctx, const char *name, size_t length, AstType type,
                                      CodegenStorage storage) {
  if (!ctx->scope) {
    scope_push(ctx);
  }
  for (CodegenSymbol *it = ctx->scope->symbols; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      return NULL;
    }
  }
  CodegenSymbol *symbol = malloc(sizeof(CodegenSymbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }
  symbol->name = name;
  symbol->length = length;
  symbol->type = type;
  symbol->storage = storage;
  symbol->offset = storage == CODEGEN_STORAGE_LOCAL ? allocate_local_offset(ctx, type) : 0;
  symbol->next = ctx->scope->symbols;
  ctx->scope->symbols = symbol;
  return symbol;
}

static int32_t measure_type(AstType type, int32_t used_bytes) {
  used_bytes = align_to(used_bytes, type_align(type));
  return used_bytes + type_size(type);
}

int32_t measure_decl_node(CodegenContext *ctx, const AstNode *node, int32_t used_bytes) {
  if (!node || ctx->had_error) {
    return used_bytes;
  }

  switch (node->kind) {
    case AST_NODE_BLOCK: {
      for (size_t i = 0; i < node->data.block.statements.count; i++) {
        used_bytes = measure_decl_node(ctx, node->data.block.statements.items[i], used_bytes);
      }
      return used_bytes;
    }
    case AST_NODE_VAR_DECL:
      return measure_type(node->data.var_decl.type, used_bytes);
    case AST_NODE_IF_STMT:
      used_bytes = measure_decl_node(ctx, node->data.if_stmt.then_branch, used_bytes);
      return measure_decl_node(ctx, node->data.if_stmt.else_branch, used_bytes);
    case AST_NODE_WHILE_STMT:
      return measure_decl_node(ctx, node->data.while_stmt.body, used_bytes);
    case AST_NODE_FOR_STMT:
      used_bytes = measure_decl_node(ctx, node->data.for_stmt.init, used_bytes);
      return measure_decl_node(ctx, node->data.for_stmt.body, used_bytes);
    case AST_NODE_DO_WHILE_STMT:
      return measure_decl_node(ctx, node->data.do_while_stmt.body, used_bytes);
    case AST_NODE_SWITCH_STMT:
      return measure_decl_node(ctx, node->data.switch_stmt.body, used_bytes);
    case AST_NODE_CASE_STMT:
      return measure_decl_node(ctx, node->data.case_stmt.statement, used_bytes);
    case AST_NODE_DEFAULT_STMT:
      return measure_decl_node(ctx, node->data.default_stmt.statement, used_bytes);
    case AST_NODE_LABEL_STMT:
      return measure_decl_node(ctx, node->data.label_stmt.statement, used_bytes);
    default:
      return used_bytes;
  }
}

int32_t is_array_value_expression(const CodegenContext *ctx, const AstNode *node) {
  if (!node || node->kind != AST_NODE_IDENTIFIER) {
    return 0;
  }
  CodegenSymbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
  return symbol && symbol->type.kind == AST_TYPE_ARRAY;
}
