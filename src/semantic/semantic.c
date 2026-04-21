#include "semantic/semantic.h"

#include "utils/diagnostic.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct Symbol {
  const char *name;
  size_t length;
  AstType type;
  struct Symbol *next;
} Symbol;

typedef struct Scope {
  Symbol *symbols;
  struct Scope *parent;
} Scope;

typedef struct FunctionSymbol {
  const char *name;
  size_t length;
  size_t param_count;
  const AstFunction *function;
  struct FunctionSymbol *next;
} FunctionSymbol;

typedef struct LabelSymbol {
  const char *name;
  size_t length;
  const AstNode *node;
  struct LabelSymbol *next;
} LabelSymbol;

typedef struct GotoUse {
  const char *name;
  size_t length;
  const AstNode *node;
  struct GotoUse *next;
} GotoUse;

typedef struct IntValueNode {
  int32_t value;
  struct IntValueNode *next;
} IntValueNode;

typedef struct SwitchContext {
  IntValueNode *cases;
  int32_t has_default;
  struct SwitchContext *next;
} SwitchContext;

typedef struct {
  const char *filename;
  int32_t had_error;
  Scope *scope;
  FunctionSymbol *functions;
  LabelSymbol *labels;
  GotoUse *gotos;
  SwitchContext *switch_stack;
  int32_t loop_depth;
  int32_t switch_depth;
  AstType current_return_type;
} SemanticContext;

static int32_t names_equal(const char *lhs, size_t lhs_len, const char *rhs, size_t rhs_len) {
  return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

static void semantic_error(SemanticContext *ctx, const AstNode *node, const char *fmt, ...) {
  ctx->had_error = 1;

  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  SourceLocation loc = {
    .filename = ctx->filename,
    .line = node ? node->line : 0,
    .column = node ? node->column : 0,
    .source_line = NULL
  };
  diagnostic_log(DIAG_LEVEL_ERROR, loc, "%s", message);
}

static Scope *scope_push(SemanticContext *ctx) {
  Scope *scope = malloc(sizeof(Scope));
  if (!scope) {
    LOG(FATAL, "out of memory");
  }
  scope->symbols = NULL;
  scope->parent = ctx->scope;
  ctx->scope = scope;
  return scope;
}

static void scope_pop(SemanticContext *ctx) {
  Scope *scope = ctx->scope;
  if (!scope) {
    return;
  }
  Symbol *symbol = scope->symbols;
  while (symbol) {
    Symbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }
  ctx->scope = scope->parent;
  free(scope);
}

static int32_t scope_declare_current(SemanticContext *ctx, const char *name, size_t length, AstType type) {
  if (!ctx->scope) {
    scope_push(ctx);
  }
  for (Symbol *it = ctx->scope->symbols; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      return 0;
    }
  }
  Symbol *symbol = malloc(sizeof(Symbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }
  symbol->name = name;
  symbol->length = length;
  symbol->type = type;
  symbol->next = ctx->scope->symbols;
  ctx->scope->symbols = symbol;
  return 1;
}

static const Symbol *scope_find(const SemanticContext *ctx, const char *name, size_t length) {
  for (const Scope *scope = ctx->scope; scope; scope = scope->parent) {
    for (const Symbol *it = scope->symbols; it; it = it->next) {
      if (names_equal(it->name, it->length, name, length)) {
        return it;
      }
    }
  }
  return NULL;
}

static int32_t scope_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  return scope_find(ctx, name, length) != NULL;
}

static int32_t function_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  for (const FunctionSymbol *it = ctx->functions; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      return 1;
    }
  }
  return 0;
}

static void function_add(SemanticContext *ctx, const AstFunction *function) {
  if (function_lookup(ctx, function->name, function->length)) {
    semantic_error(ctx, function->body, "duplicate function '%s'", function->name);
    return;
  }
  FunctionSymbol *symbol = malloc(sizeof(FunctionSymbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }
  symbol->name = function->name;
  symbol->length = function->length;
  symbol->param_count = function->params.count;
  symbol->function = function;
  symbol->next = ctx->functions;
  ctx->functions = symbol;
}

static const FunctionSymbol *function_find(const SemanticContext *ctx, const char *name, size_t length) {
  for (const FunctionSymbol *it = ctx->functions; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      return it;
    }
  }
  return NULL;
}

static void functions_destroy(SemanticContext *ctx) {
  FunctionSymbol *symbol = ctx->functions;
  while (symbol) {
    FunctionSymbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }
  ctx->functions = NULL;
}

static void labels_destroy(SemanticContext *ctx) {
  LabelSymbol *symbol = ctx->labels;
  while (symbol) {
    LabelSymbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }
  ctx->labels = NULL;
}

static void gotos_destroy(SemanticContext *ctx) {
  GotoUse *use = ctx->gotos;
  while (use) {
    GotoUse *next = use->next;
    free(use);
    use = next;
  }
  ctx->gotos = NULL;
}

static void switch_context_pop(SemanticContext *ctx) {
  if (!ctx->switch_stack) {
    return;
  }
  SwitchContext *current = ctx->switch_stack;
  IntValueNode *it = current->cases;
  while (it) {
    IntValueNode *next = it->next;
    free(it);
    it = next;
  }
  ctx->switch_stack = current->next;
  free(current);
}

static void switch_context_push(SemanticContext *ctx) {
  SwitchContext *current = malloc(sizeof(SwitchContext));
  if (!current) {
    LOG(FATAL, "out of memory");
  }
  current->cases = NULL;
  current->has_default = 0;
  current->next = ctx->switch_stack;
  ctx->switch_stack = current;
}

static int32_t switch_context_has_case(const SwitchContext *ctx, int32_t value) {
  for (const IntValueNode *it = ctx->cases; it; it = it->next) {
    if (it->value == value) {
      return 1;
    }
  }
  return 0;
}

static void switch_context_add_case(SwitchContext *ctx, int32_t value) {
  IntValueNode *node = malloc(sizeof(IntValueNode));
  if (!node) {
    LOG(FATAL, "out of memory");
  }
  node->value = value;
  node->next = ctx->cases;
  ctx->cases = node;
}

static int32_t label_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  for (const LabelSymbol *it = ctx->labels; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      return 1;
    }
  }
  return 0;
}

static void label_add(SemanticContext *ctx, const AstNode *node, const char *name, size_t length) {
  for (const LabelSymbol *it = ctx->labels; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      semantic_error(ctx, node, "duplicate label '%s'", name);
      return;
    }
  }
  LabelSymbol *label = malloc(sizeof(LabelSymbol));
  if (!label) {
    LOG(FATAL, "out of memory");
  }
  label->name = name;
  label->length = length;
  label->node = node;
  label->next = ctx->labels;
  ctx->labels = label;
}

static void goto_add(SemanticContext *ctx, const AstNode *node, const char *name, size_t length) {
  GotoUse *use = malloc(sizeof(GotoUse));
  if (!use) {
    LOG(FATAL, "out of memory");
  }
  use->name = name;
  use->length = length;
  use->node = node;
  use->next = ctx->gotos;
  ctx->gotos = use;
}

static void validate_gotos(SemanticContext *ctx) {
  for (const GotoUse *use = ctx->gotos; use; use = use->next) {
    if (!label_lookup(ctx, use->name, use->length)) {
      semantic_error(ctx, use->node, "unknown label '%s'", use->name);
    }
  }
}

static void analyze_node(SemanticContext *ctx, const AstNode *node);

static int32_t is_array_value_expression(const SemanticContext *ctx, const AstNode *node);

static void analyze_call(SemanticContext *ctx, const AstNode *node) {
  const AstNode *callee = node->data.call.callee;
  if (callee && callee->kind == AST_NODE_IDENTIFIER) {
    const char *name = callee->data.identifier.name;
    size_t length = callee->data.identifier.length;
    const Symbol *shadow = scope_find(ctx, name, length);
    if (shadow) {
      semantic_error(ctx, callee, "'%s' is not a function", name);
    } else {
      const FunctionSymbol *function = function_find(ctx, name, length);
      if (!function) {
        semantic_error(ctx, callee, "unknown function '%s'", name);
      } else if (node->data.call.args.count != function->param_count) {
        semantic_error(ctx, node, "function '%s' expects %zu arguments, got %zu", name, function->param_count,
                       node->data.call.args.count);
      } else {
        for (size_t i = 0; i < node->data.call.args.count; i++) {
          const AstType *param_type = &function->function->params.items[i].type;
          const AstNode *arg = node->data.call.args.items[i];
          if (param_type->kind != AST_TYPE_ARRAY && is_array_value_expression(ctx, arg)) {
            semantic_error(ctx, arg, "array value cannot be passed to scalar parameter %zu of '%s'", i + 1, name);
          }
        }
      }
    }
  } else {
    semantic_error(ctx, node, "call target must be a function identifier");
    analyze_node(ctx, callee);
  }
  for (size_t i = 0; i < node->data.call.args.count; i++) {
    analyze_node(ctx, node->data.call.args.items[i]);
  }
}

static void analyze_block(SemanticContext *ctx, const AstNode *node) {
  scope_push(ctx);
  for (size_t i = 0; i < node->data.block.statements.count; i++) {
    analyze_node(ctx, node->data.block.statements.items[i]);
  }
  scope_pop(ctx);
}

static int32_t is_assignable_node(const AstNode *node) {
  if (!node) {
    return 0;
  }
  return node->kind == AST_NODE_IDENTIFIER || node->kind == AST_NODE_SUBSCRIPT_EXPR;
}

static int32_t is_array_identifier(const SemanticContext *ctx, const AstNode *node) {
  if (!node || node->kind != AST_NODE_IDENTIFIER) {
    return 0;
  }
  const Symbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
  if (!symbol) {
    return 0;
  }
  return symbol->type.kind == AST_TYPE_ARRAY;
}

static int32_t is_array_value_expression(const SemanticContext *ctx, const AstNode *node) {
  if (!node) {
    return 0;
  }
  return is_array_identifier(ctx, node);
}

static void analyze_node(SemanticContext *ctx, const AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->kind) {
    case AST_NODE_BLOCK:
      analyze_block(ctx, node);
      return;
    case AST_NODE_RETURN_STMT:
      analyze_node(ctx, node->data.return_stmt.expr);
      if (ctx->current_return_type.kind != AST_TYPE_ARRAY &&
          is_array_value_expression(ctx, node->data.return_stmt.expr)) {
        semantic_error(ctx, node, "cannot return array value from scalar function");
      }
      return;
    case AST_NODE_EXPR_STMT:
      analyze_node(ctx, node->data.expr_stmt.expr);
      return;
    case AST_NODE_VAR_DECL:
      if (!scope_declare_current(ctx, node->data.var_decl.name, node->data.var_decl.length, node->data.var_decl.type)) {
        semantic_error(ctx, node, "duplicate declaration '%s'", node->data.var_decl.name);
      }
      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.type.array_size <= 0) {
        semantic_error(ctx, node, "array '%s' must have positive size", node->data.var_decl.name);
      }
      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.initializer &&
          node->data.var_decl.initializer->kind != AST_NODE_INIT_LIST) {
        semantic_error(ctx, node, "array '%s' must be initialized with initializer list", node->data.var_decl.name);
      }
      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.initializer &&
          node->data.var_decl.initializer->kind == AST_NODE_INIT_LIST &&
          (int32_t) node->data.var_decl.initializer->data.init_list.elements.count > node->data.var_decl.type.
          array_size) {
        semantic_error(ctx, node, "array '%s' initializer has too many elements", node->data.var_decl.name);
      }
      if (node->data.var_decl.type.kind != AST_TYPE_ARRAY && node->data.var_decl.initializer &&
          node->data.var_decl.initializer->kind == AST_NODE_INIT_LIST) {
        semantic_error(ctx, node, "scalar '%s' cannot be initialized with initializer list", node->data.var_decl.name);
      }
      analyze_node(ctx, node->data.var_decl.initializer);
      return;
    case AST_NODE_IF_STMT:
      analyze_node(ctx, node->data.if_stmt.condition);
      analyze_node(ctx, node->data.if_stmt.then_branch);
      analyze_node(ctx, node->data.if_stmt.else_branch);
      return;
    case AST_NODE_WHILE_STMT:
      analyze_node(ctx, node->data.while_stmt.condition);
      ctx->loop_depth++;
      analyze_node(ctx, node->data.while_stmt.body);
      ctx->loop_depth--;
      return;
    case AST_NODE_FOR_STMT:
      scope_push(ctx);
      analyze_node(ctx, node->data.for_stmt.init);
      analyze_node(ctx, node->data.for_stmt.condition);
      analyze_node(ctx, node->data.for_stmt.post);
      ctx->loop_depth++;
      analyze_node(ctx, node->data.for_stmt.body);
      ctx->loop_depth--;
      scope_pop(ctx);
      return;
    case AST_NODE_DO_WHILE_STMT:
      ctx->loop_depth++;
      analyze_node(ctx, node->data.do_while_stmt.body);
      ctx->loop_depth--;
      analyze_node(ctx, node->data.do_while_stmt.condition);
      return;
    case AST_NODE_SWITCH_STMT:
      analyze_node(ctx, node->data.switch_stmt.expr);
      switch_context_push(ctx);
      ctx->switch_depth++;
      analyze_node(ctx, node->data.switch_stmt.body);
      ctx->switch_depth--;
      switch_context_pop(ctx);
      return;
    case AST_NODE_CASE_STMT: {
      if (ctx->switch_depth <= 0 || !ctx->switch_stack) {
        semantic_error(ctx, node, "unexpected 'case'");
      }
      const AstNode *value = node->data.case_stmt.value;
      if (!value || value->kind != AST_NODE_INT_LITERAL) {
        semantic_error(ctx, node, "case value must be integer literal");
      } else if (ctx->switch_stack) {
        int32_t case_value = value->data.int_literal.value;
        if (switch_context_has_case(ctx->switch_stack, case_value)) {
          semantic_error(ctx, node, "duplicate case value '%d'", case_value);
        } else {
          switch_context_add_case(ctx->switch_stack, case_value);
        }
      }
      analyze_node(ctx, node->data.case_stmt.statement);
      return;
    }
    case AST_NODE_DEFAULT_STMT:
      if (ctx->switch_depth <= 0 || !ctx->switch_stack) {
        semantic_error(ctx, node, "unexpected 'default'");
      } else if (ctx->switch_stack->has_default) {
        semantic_error(ctx, node, "duplicate default label");
      } else {
        ctx->switch_stack->has_default = 1;
      }
      analyze_node(ctx, node->data.default_stmt.statement);
      return;
    case AST_NODE_BREAK_STMT:
      if (ctx->loop_depth <= 0 && ctx->switch_depth <= 0) {
        semantic_error(ctx, node, "'break' not within loop or switch");
      }
      return;
    case AST_NODE_GOTO_STMT:
      goto_add(ctx, node, node->data.goto_stmt.label, node->data.goto_stmt.length);
      return;
    case AST_NODE_LABEL_STMT:
      label_add(ctx, node, node->data.label_stmt.label, node->data.label_stmt.length);
      analyze_node(ctx, node->data.label_stmt.statement);
      return;
    case AST_NODE_BINARY_EXPR:
      if (node->data.binary.op == TOKEN_ASSIGN && !is_assignable_node(node->data.binary.left)) {
        semantic_error(ctx, node, "left side of assignment must be assignable");
      }
      if (node->data.binary.op == TOKEN_ASSIGN && is_array_identifier(ctx, node->data.binary.left)) {
        semantic_error(ctx, node, "array '%s' is not assignable", node->data.binary.left->data.identifier.name);
      }
      analyze_node(ctx, node->data.binary.left);
      analyze_node(ctx, node->data.binary.right);
      if (node->data.binary.op == TOKEN_ASSIGN) {
        if (is_array_value_expression(ctx, node->data.binary.right)) {
          semantic_error(ctx, node, "array value cannot be assigned to a scalar expression");
        }
      } else if (is_array_value_expression(ctx, node->data.binary.left) ||
                 is_array_value_expression(ctx, node->data.binary.right)) {
        semantic_error(ctx, node, "array value cannot be used in scalar expression");
      }
      return;
    case AST_NODE_UNARY_EXPR:
      analyze_node(ctx, node->data.unary.operand);
      if (is_array_value_expression(ctx, node->data.unary.operand)) {
        semantic_error(ctx, node, "array value cannot be used in unary expression");
      }
      return;
    case AST_NODE_INT_LITERAL:
      return;
    case AST_NODE_IDENTIFIER:
      if (!scope_lookup(ctx, node->data.identifier.name, node->data.identifier.length)) {
        semantic_error(ctx, node, "undeclared identifier '%s'", node->data.identifier.name);
      }
      return;
    case AST_NODE_SUBSCRIPT_EXPR:
      analyze_node(ctx, node->data.subscript.base);
      analyze_node(ctx, node->data.subscript.index);
      if (!is_array_identifier(ctx, node->data.subscript.base)) {
        semantic_error(ctx, node, "subscript base must be an array identifier");
      }
      if (is_array_value_expression(ctx, node->data.subscript.index)) {
        semantic_error(ctx, node, "subscript index must be a scalar expression");
      }
      return;
    case AST_NODE_CALL_EXPR:
      analyze_call(ctx, node);
      return;
    case AST_NODE_INIT_LIST:
      for (size_t i = 0; i < node->data.init_list.elements.count; i++) {
        analyze_node(ctx, node->data.init_list.elements.items[i]);
      }
      return;
    default:
      return;
  }
}

static void analyze_function(SemanticContext *ctx, const AstFunction *function) {
  labels_destroy(ctx);
  gotos_destroy(ctx);

  ctx->loop_depth = 0;
  ctx->switch_depth = 0;
  ctx->current_return_type = function->return_type;

  scope_push(ctx);
  for (size_t i = 0; i < function->params.count; i++) {
    const AstParam *param = &function->params.items[i];
    if (!scope_declare_current(ctx, param->name, param->length, param->type)) {
      semantic_error(ctx, function->body, "duplicate parameter '%s'", param->name);
    }
  }
  analyze_node(ctx, function->body);
  scope_pop(ctx);

  validate_gotos(ctx);
}

int32_t semantic_analyze(const AstModule *module, const char *filename) {
  if (!module) {
    return 1;
  }

  SemanticContext ctx = {
    .filename = filename,
    .had_error = 0,
    .scope = NULL,
    .functions = NULL,
    .labels = NULL,
    .gotos = NULL,
    .switch_stack = NULL,
    .loop_depth = 0,
    .switch_depth = 0,
    .current_return_type = {.kind = AST_TYPE_INT, .element_kind = AST_TYPE_INT, .array_size = 0}
  };

  for (size_t i = 0; i < module->functions.count; i++) {
    function_add(&ctx, module->functions.items[i]);
  }

  for (size_t i = 0; i < module->functions.count; i++) {
    analyze_function(&ctx, module->functions.items[i]);
  }

  functions_destroy(&ctx);
  labels_destroy(&ctx);
  gotos_destroy(&ctx);
  while (ctx.switch_stack) {
    switch_context_pop(&ctx);
  }
  return ctx.had_error;
}
