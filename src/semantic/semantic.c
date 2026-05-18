#include "semantic_internal.h"
#include "utils/diagnostic.h"

#include <stdarg.h>
#include <stdio.h>

int32_t sem_names_equal(const char *lhs, size_t lhs_len, const char *rhs, size_t rhs_len) {
  return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

void semantic_error(SemanticContext *ctx, const AstNode *node, const char *fmt, ...) {
  ctx->had_error = 1;

  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  SourceLocation loc = {
      .filename = ctx->filename, .line = node ? node->line : 0, .column = node ? node->column : 0, .source_line = NULL};
  diagnostic_log(DIAG_LEVEL_ERROR, loc, "%s", message);
}

static void analyze_node(SemanticContext *ctx, const AstNode *node);

static int32_t is_array_value_expression(const SemanticContext *ctx, const AstNode *node);
static AstType expression_type(const SemanticContext *ctx, const AstNode *node);

static AstType scalar_type(AstTypeKind kind) {
  AstType type = {
      .kind = kind,
      .element_kind = kind,
      .array_size = 0,
  };
  return type;
}

static AstType pointer_type_to(AstType type) {
  AstType pointer = {
      .kind = AST_TYPE_POINTER,
      .element_kind = type.kind == AST_TYPE_ARRAY ? type.element_kind : type.kind,
      .array_size = 0,
  };
  return pointer;
}

static AstType pointed_type(AstType type) {
  return scalar_type(type.element_kind);
}

static int32_t is_scalar_type(AstType type) {
  return type.kind == AST_TYPE_INT || type.kind == AST_TYPE_CHAR;
}

static void validate_builtin_call(SemanticContext *ctx, const AstNode *node, const BuiltinFunction *builtin) {
  const char *name = node->data.call.callee->data.identifier.name;

  if (node->data.call.args.count != builtin->param_count) {
    semantic_error(ctx, node, "compiler intrinsic '%s' expects %zu arguments, got %zu", name, builtin->param_count,
                   node->data.call.args.count);
    return;
  }

  for (size_t i = 0; i < node->data.call.args.count; i++) {
    const AstNode *arg = node->data.call.args.items[i];

    if (is_array_value_expression(ctx, arg)) {
      semantic_error(ctx, arg, "array value cannot be passed to compiler intrinsic '%s'", name);
    }
  }
}

static void validate_function_call(SemanticContext *ctx, const AstNode *node, const FunctionSymbol *function) {
  const char *name = node->data.call.callee->data.identifier.name;

  if (node->data.call.args.count != function->param_count) {
    semantic_error(ctx, node, "function '%s' expects %zu arguments, got %zu", name, function->param_count,
                   node->data.call.args.count);
    return;
  }

  for (size_t i = 0; i < node->data.call.args.count; i++) {
    const AstType *param_type = &function->function->params.items[i].type;
    const AstNode *arg = node->data.call.args.items[i];

    if (param_type->kind != AST_TYPE_POINTER && is_array_value_expression(ctx, arg)) {
      semantic_error(ctx, arg, "array value cannot be passed to scalar parameter %zu of '%s'", i + 1, name);
    }
  }
}

static void analyze_call(SemanticContext *ctx, const AstNode *node) {
  const AstNode *callee = node->data.call.callee;

  if (!callee || callee->kind != AST_NODE_IDENTIFIER) {
    semantic_error(ctx, node, "call target must be a function identifier");
    analyze_node(ctx, callee);
    goto analyze_args;
  }

  const char *name = callee->data.identifier.name;
  size_t length = callee->data.identifier.length;

  if (sem_scope_find(ctx, name, length)) {
    semantic_error(ctx, callee, "'%s' is a variable, not a function", name);
    goto analyze_args;
  }

  const FunctionSymbol *function = function_find(ctx, name, length);
  if (function) {
    validate_function_call(ctx, node, function);
    goto analyze_args;
  }

  const BuiltinFunction *builtin = builtin_find(name, length);
  if (builtin) {
    validate_builtin_call(ctx, node, builtin);
    goto analyze_args;
  }

  semantic_error(ctx, callee, "call to undeclared function '%s'", name);

analyze_args:
  for (size_t i = 0; i < node->data.call.args.count; i++) {
    analyze_node(ctx, node->data.call.args.items[i]);
  }
}

static void analyze_block(SemanticContext *ctx, const AstNode *node) {
  sem_scope_push(ctx);

  for (size_t i = 0; i < node->data.block.statements.count; i++) {
    analyze_node(ctx, node->data.block.statements.items[i]);
  }

  sem_scope_pop(ctx);
}

static void validate_var_decl_initializer(SemanticContext *ctx, const AstNode *node) {
  if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.type.array_size <= 0) {
    semantic_error(ctx, node, "array '%s' must have positive size", node->data.var_decl.name);
  }

  if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.initializer &&
      node->data.var_decl.initializer->kind != AST_NODE_INIT_LIST) {
    semantic_error(ctx, node, "array '%s' must be initialized with initializer list", node->data.var_decl.name);
  }

  if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.initializer &&
      node->data.var_decl.initializer->kind == AST_NODE_INIT_LIST &&
      (int32_t) node->data.var_decl.initializer->data.init_list.elements.count > node->data.var_decl.type.array_size) {
    semantic_error(ctx, node, "array '%s' initializer has too many elements", node->data.var_decl.name);
  }

  if (node->data.var_decl.type.kind != AST_TYPE_ARRAY && node->data.var_decl.initializer &&
      node->data.var_decl.initializer->kind == AST_NODE_INIT_LIST) {
    semantic_error(ctx, node, "scalar '%s' cannot be initialized with initializer list", node->data.var_decl.name);
  }
}

static int32_t is_assignable_node(const AstNode *node) {
  if (!node) {
    return 0;
  }
  return node->kind == AST_NODE_IDENTIFIER || node->kind == AST_NODE_SUBSCRIPT_EXPR ||
         (node->kind == AST_NODE_UNARY_EXPR && node->data.unary.op == TOKEN_STAR);
}

static int32_t is_array_identifier(const SemanticContext *ctx, const AstNode *node) {
  if (!node || node->kind != AST_NODE_IDENTIFIER) {
    return 0;
  }

  const Symbol *symbol = sem_scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
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

static AstType expression_type(const SemanticContext *ctx, const AstNode *node) {
  AstType fallback = scalar_type(AST_TYPE_INT);
  if (!node) {
    return fallback;
  }

  switch (node->kind) {
    case AST_NODE_IDENTIFIER: {
      const Symbol *symbol = sem_scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
      return symbol ? symbol->type : fallback;
    }
    case AST_NODE_SUBSCRIPT_EXPR: {
      AstType base_type = expression_type(ctx, node->data.subscript.base);
      if (base_type.kind == AST_TYPE_ARRAY) {
        return scalar_type(base_type.element_kind);
      }
      if (base_type.kind == AST_TYPE_POINTER) {
        return pointed_type(base_type);
      }
      return fallback;
    }
    case AST_NODE_UNARY_EXPR:
      if (node->data.unary.op == TOKEN_STAR) {
        AstType operand_type = expression_type(ctx, node->data.unary.operand);
        return operand_type.kind == AST_TYPE_POINTER ? pointed_type(operand_type) : fallback;
      }
      if (node->data.unary.op == TOKEN_AMPERSAND) {
        return pointer_type_to(expression_type(ctx, node->data.unary.operand));
      }
      return fallback;
    case AST_NODE_CALL_EXPR:
      if (node->data.call.callee && node->data.call.callee->kind == AST_NODE_IDENTIFIER) {
        const char *name = node->data.call.callee->data.identifier.name;
        size_t length = node->data.call.callee->data.identifier.length;
        const FunctionSymbol *function = function_find(ctx, name, length);
        return function ? function->function->return_type : fallback;
      }
      return fallback;
    case AST_NODE_BINARY_EXPR: {
      AstType left_type = expression_type(ctx, node->data.binary.left);
      AstType right_type = expression_type(ctx, node->data.binary.right);
      int32_t left_ptr = left_type.kind == AST_TYPE_POINTER;
      int32_t right_ptr = right_type.kind == AST_TYPE_POINTER;
      int32_t left_scalar = is_scalar_type(left_type);
      int32_t right_scalar = is_scalar_type(right_type);

      if (node->data.binary.op == TOKEN_PLUS) {
        if (left_ptr && right_scalar) {
          return left_type;
        }
        if (right_ptr && left_scalar) {
          return right_type;
        }
      } else if (node->data.binary.op == TOKEN_MINUS) {
        if (left_ptr && right_scalar) {
          return left_type;
        }
        if (left_ptr && right_ptr) {
          return scalar_type(AST_TYPE_INT);
        }
      }
      return fallback;
    }
    case AST_NODE_INT_LITERAL:
      return scalar_type(AST_TYPE_INT);
    default:
      return fallback;
  }
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

      if (ctx->current_return_type.kind != AST_TYPE_ARRAY && ctx->current_return_type.kind != AST_TYPE_POINTER &&
          is_array_value_expression(ctx, node->data.return_stmt.expr)) {
        semantic_error(ctx, node, "cannot return array value from scalar function");
      }

      return;

    case AST_NODE_EXPR_STMT:
      analyze_node(ctx, node->data.expr_stmt.expr);
      return;

    case AST_NODE_VAR_DECL:
      if (!sem_scope_declare_current(ctx, node->data.var_decl.name, node->data.var_decl.length,
                                     node->data.var_decl.type)) {
        semantic_error(ctx, node, "duplicate declaration '%s'", node->data.var_decl.name);
      }

      validate_var_decl_initializer(ctx, node);
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
      sem_scope_push(ctx);

      analyze_node(ctx, node->data.for_stmt.init);
      analyze_node(ctx, node->data.for_stmt.condition);
      analyze_node(ctx, node->data.for_stmt.post);

      ctx->loop_depth++;
      analyze_node(ctx, node->data.for_stmt.body);
      ctx->loop_depth--;

      sem_scope_pop(ctx);
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
        AstType left_type = expression_type(ctx, node->data.binary.left);
        if (left_type.kind != AST_TYPE_POINTER && is_array_value_expression(ctx, node->data.binary.right)) {
          semantic_error(ctx, node, "array value cannot be assigned to a scalar expression");
        }
      } else {
        if (node->data.binary.op == TOKEN_PLUS || node->data.binary.op == TOKEN_MINUS) {
          AstType left_type = expression_type(ctx, node->data.binary.left);
          AstType right_type = expression_type(ctx, node->data.binary.right);
          int32_t left_ptr = left_type.kind == AST_TYPE_POINTER;
          int32_t right_ptr = right_type.kind == AST_TYPE_POINTER;
          int32_t left_scalar = is_scalar_type(left_type);
          int32_t right_scalar = is_scalar_type(right_type);

          if (left_ptr && right_ptr) {
            if (node->data.binary.op == TOKEN_PLUS) {
              semantic_error(ctx, node, "cannot add two pointers");
            } else if (left_type.element_kind != right_type.element_kind) {
              semantic_error(ctx, node, "pointer subtraction requires matching element types");
            }
          } else if (left_ptr && right_scalar) {
          } else if (right_ptr && left_scalar && node->data.binary.op == TOKEN_PLUS) {
          } else if (left_ptr || right_ptr) {
            semantic_error(ctx, node, "invalid operands to pointer arithmetic");
          }
        }

        if (is_array_value_expression(ctx, node->data.binary.left) ||
            is_array_value_expression(ctx, node->data.binary.right)) {
          semantic_error(ctx, node, "array value cannot be used in scalar expression");
        }
      }
      return;

    case AST_NODE_UNARY_EXPR:
      if (node->data.unary.op == TOKEN_AMPERSAND && !is_assignable_node(node->data.unary.operand)) {
        semantic_error(ctx, node, "address-of operand must be assignable");
      }

      analyze_node(ctx, node->data.unary.operand);

      if (node->data.unary.op == TOKEN_STAR) {
        AstType operand_type = expression_type(ctx, node->data.unary.operand);
        if (operand_type.kind != AST_TYPE_POINTER) {
          semantic_error(ctx, node, "cannot dereference a non-pointer expression");
        }
      } else if (node->data.unary.op != TOKEN_AMPERSAND && is_array_value_expression(ctx, node->data.unary.operand)) {
        semantic_error(ctx, node, "array value cannot be used in unary expression");
      }

      return;

    case AST_NODE_INT_LITERAL:
      return;

    case AST_NODE_IDENTIFIER:
      if (!sem_scope_lookup(ctx, node->data.identifier.name, node->data.identifier.length)) {
        semantic_error(ctx, node, "undeclared identifier '%s'", node->data.identifier.name);
      }

      return;

    case AST_NODE_SUBSCRIPT_EXPR:
      analyze_node(ctx, node->data.subscript.base);
      analyze_node(ctx, node->data.subscript.index);

      AstType base_type = expression_type(ctx, node->data.subscript.base);
      if (base_type.kind != AST_TYPE_ARRAY && base_type.kind != AST_TYPE_POINTER) {
        semantic_error(ctx, node, "subscript base must be an array or pointer");
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

  sem_scope_push(ctx);

  for (size_t i = 0; i < function->params.count; i++) {
    const AstParam *param = &function->params.items[i];

    if (!sem_scope_declare_current(ctx, param->name, param->length, param->type)) {
      semantic_error(ctx, function->body, "duplicate parameter '%s'", param->name);
    }
  }

  analyze_node(ctx, function->body);
  sem_scope_pop(ctx);

  validate_gotos(ctx);
}

int32_t semantic_analyze(const AstModule *module, const char *filename) {
  if (!module) {
    return 1;
  }

  SemanticContext ctx = {.filename = filename,
                         .had_error = 0,
                         .scope = NULL,
                         .functions = NULL,
                         .labels = NULL,
                         .gotos = NULL,
                         .switch_stack = NULL,
                         .loop_depth = 0,
                         .switch_depth = 0,
                         .current_return_type = {.kind = AST_TYPE_INT, .element_kind = AST_TYPE_INT, .array_size = 0}};

  sem_scope_push(&ctx);

  for (size_t i = 0; i < module->globals.count; i++) {
    const AstNode *global = module->globals.items[i];
    if (!global || global->kind != AST_NODE_VAR_DECL) {
      continue;
    }

    if (!sem_scope_declare_current(&ctx, global->data.var_decl.name, global->data.var_decl.length,
                                   global->data.var_decl.type)) {
      semantic_error(&ctx, global, "duplicate global declaration '%s'", global->data.var_decl.name);
    }

    validate_var_decl_initializer(&ctx, global);
    analyze_node(&ctx, global->data.var_decl.initializer);
  }

  for (size_t i = 0; i < module->functions.count; i++) {
    function_add(&ctx, module->functions.items[i]);
  }

  if (ctx.had_error) {
    sem_scope_pop(&ctx);
    functions_destroy(&ctx);
    return ctx.had_error;
  }

  for (size_t i = 0; i < module->functions.count; i++) {
    analyze_function(&ctx, module->functions.items[i]);
  }

  sem_scope_pop(&ctx);
  functions_destroy(&ctx);
  labels_destroy(&ctx);
  gotos_destroy(&ctx);
  while (ctx.switch_stack) {
    switch_context_pop(&ctx);
  }

  return ctx.had_error;
}
