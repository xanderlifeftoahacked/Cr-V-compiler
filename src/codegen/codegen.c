// labudi labuday I want to die

#include "codegen/codegen.h"
#include "utils/diagnostic.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


typedef struct CodegenSymbol {
  const char *name;
  size_t length;
  AstType type;
  int32_t slot;
  struct CodegenSymbol *next;
} CodegenSymbol;

typedef struct CodegenScope {
  CodegenSymbol *symbols;
  struct CodegenScope *parent;
} CodegenScope;

typedef struct BreakTarget {
  char label[64];
  struct BreakTarget *next;
} BreakTarget;

typedef struct SwitchCaseLabel {
  int32_t value;
  char label[64];
  struct SwitchCaseLabel *next;
} SwitchCaseLabel;

typedef struct SwitchContext {
  char end_label[64];
  char default_label[64];
  int32_t has_default;
  SwitchCaseLabel *cases;
  struct SwitchContext *next;
} SwitchContext;

typedef struct {
  FILE *out;
  const char *filename;
  const char *function_name;
  int32_t had_error;
  CodegenScope *scope;
  BreakTarget *break_stack;
  SwitchContext *switch_stack;
  int32_t next_slot;
  int32_t frame_size;
  int32_t label_seq;
} CodegenContext;

static int32_t names_equal(const char *lhs, size_t lhs_len, const char *rhs, size_t rhs_len) {
  return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

static int32_t align16(int32_t value) {
  return (value + 15) & ~15;
}

static int32_t slot_offset(int32_t slot) {
  return -12 - slot * 4;
}

static void codegen_error(CodegenContext *ctx, const AstNode *node, const char *fmt, ...) {
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
  diagnostic_log(DIAG_LEVEL_ERROR, loc, "[GEN] ""%s", message);
}

static void make_label(CodegenContext *ctx, const char *suffix, char *buffer, size_t size) {
  // TODO optimize .s size
  snprintf(buffer, size, "%s_%s_%d", ctx->function_name, suffix, ctx->label_seq++);
}

static void emit_line(CodegenContext *ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(ctx->out, fmt, args);
  va_end(args);
  fputc('\n', ctx->out);
}

static CodegenScope *scope_push(CodegenContext *ctx) {
  CodegenScope *scope = malloc(sizeof(CodegenScope));
  if (!scope) {
    LOG(FATAL, "really?");
  }
  scope->symbols = NULL;
  scope->parent = ctx->scope;
  ctx->scope = scope;
  return scope;
}

static void scope_pop(CodegenContext *ctx) {
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

static CodegenSymbol *scope_find(const CodegenContext *ctx, const char *name, size_t length) {
  for (const CodegenScope *scope = ctx->scope; scope; scope = scope->parent) {
    for (CodegenSymbol *it = scope->symbols; it; it = it->next) {
      if (names_equal(it->name, it->length, name, length)) {
        return it;
      }
    }
  }
  return NULL;
}

static int32_t scope_declare_current(CodegenContext *ctx, const char *name, size_t length, AstType type) {
  if (!ctx->scope) {
    scope_push(ctx);
  }

  for (CodegenSymbol *it = ctx->scope->symbols; it; it = it->next) {
    if (names_equal(it->name, it->length, name, length)) {
      return 0;
    }
  }

  CodegenSymbol *symbol = malloc(sizeof(CodegenSymbol));
  if (!symbol) {
    LOG(FATAL, "really?");
  }

  symbol->name = name;
  symbol->length = length;
  symbol->type = type;
  symbol->slot = ctx->next_slot++;
  symbol->next = ctx->scope->symbols;
  ctx->scope->symbols = symbol;

  return 1;
}

static BreakTarget *break_push(CodegenContext *ctx, const char *label) {
  BreakTarget *target = malloc(sizeof(BreakTarget));
  if (!target) {
    LOG(FATAL, "really?");
  }

  snprintf(target->label, sizeof(target->label), "%s", label);
  target->next = ctx->break_stack;
  ctx->break_stack = target;

  return target;
}

static void break_pop(CodegenContext *ctx) {
  BreakTarget *target = ctx->break_stack;
  if (!target) {
    return;
  }

  ctx->break_stack = target->next;
  free(target);
}

static const char *break_top(const CodegenContext *ctx) {
  return ctx->break_stack ? ctx->break_stack->label : NULL;
}

static SwitchContext *switch_push(CodegenContext *ctx) {
  SwitchContext *sw = malloc(sizeof(SwitchContext));
  if (!sw) {
    LOG(FATAL, "really?");
  }

  sw->end_label[0] = '\0';
  sw->default_label[0] = '\0';
  sw->has_default = 0;
  sw->cases = NULL;
  sw->next = ctx->switch_stack;
  ctx->switch_stack = sw;

  return sw;
}

static void switch_pop(CodegenContext *ctx) {
  SwitchContext *sw = ctx->switch_stack;
  if (!sw) {
    return;
  }

  SwitchCaseLabel *it = sw->cases;
  while (it) {
    SwitchCaseLabel *next = it->next;
    free(it);
    it = next;
  }

  ctx->switch_stack = sw->next;
  free(sw);
}

static SwitchContext *switch_top(CodegenContext *ctx) {
  return ctx->switch_stack;
}

static SwitchCaseLabel *switch_case_find(SwitchContext *sw, int32_t value) {
  for (SwitchCaseLabel *it = sw->cases; it; it = it->next) {
    if (it->value == value) {
      return it;
    }
  }
  return NULL;
}

static SwitchCaseLabel *switch_case_add(CodegenContext *ctx, SwitchContext *sw, int32_t value, const char *suffix) {
  SwitchCaseLabel *label = malloc(sizeof(SwitchCaseLabel));
  if (!label) {
    LOG(FATAL, "really");
  }

  label->value = value;
  make_label(ctx, suffix, label->label, sizeof(label->label));
  label->next = sw->cases;
  sw->cases = label;

  return label;
}

static int32_t count_decl_node(CodegenContext *ctx, const AstNode *node) {
  if (!node || ctx->had_error) {
    return 0;
  }

  switch (node->kind) {
    case AST_NODE_BLOCK: {
      int32_t count = 0;
      for (size_t i = 0; i < node->data.block.statements.count; i++) {
        count += count_decl_node(ctx, node->data.block.statements.items[i]);
      }
      return count;
    }

    case AST_NODE_VAR_DECL:
      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY) {
        // TODO
        codegen_error(ctx, node, "arrays are not yet supported");
        return 0;
      }
      return 1;

    case AST_NODE_IF_STMT:
      return count_decl_node(ctx, node->data.if_stmt.then_branch) +
             count_decl_node(ctx, node->data.if_stmt.else_branch);

    case AST_NODE_WHILE_STMT:
      return count_decl_node(ctx, node->data.while_stmt.body);

    case AST_NODE_FOR_STMT:
      return count_decl_node(ctx, node->data.for_stmt.init) + count_decl_node(ctx, node->data.for_stmt.body);

    case AST_NODE_DO_WHILE_STMT:
      return count_decl_node(ctx, node->data.do_while_stmt.body);

    case AST_NODE_SWITCH_STMT:
      return count_decl_node(ctx, node->data.switch_stmt.body);

    case AST_NODE_CASE_STMT:
      return count_decl_node(ctx, node->data.case_stmt.statement);

    case AST_NODE_DEFAULT_STMT:
      return count_decl_node(ctx, node->data.default_stmt.statement);

    case AST_NODE_LABEL_STMT:
      return count_decl_node(ctx, node->data.label_stmt.statement);

    default:
      return 0;
  }
}

static void emit_expr(CodegenContext *ctx, const AstNode *node);

static void emit_stmt(CodegenContext *ctx, const AstNode *node);

static void emit_push_a0(CodegenContext *ctx) {
  emit_line(ctx, "  addi sp, sp, -4");
  emit_line(ctx, "  sw a0, 0(sp)");
}

static void emit_pop_t0(CodegenContext *ctx) {
  emit_line(ctx, "  lw t0, 0(sp)");
  emit_line(ctx, "  addi sp, sp, 4");
}

static void emit_load_symbol(CodegenContext *ctx, const CodegenSymbol *symbol) {
  emit_line(ctx, "  lw a0, %d(s0)", slot_offset(symbol->slot));
}

static void emit_store_symbol(CodegenContext *ctx, const CodegenSymbol *symbol) {
  emit_line(ctx, "  sw a0, %d(s0)", slot_offset(symbol->slot));
}

static void emit_binary_stack(CodegenContext *ctx, TokenKind op) {
  emit_pop_t0(ctx);
  switch (op) {
    case TOKEN_PLUS:
      emit_line(ctx, "  add a0, t0, a0");
      return;

    case TOKEN_MINUS:
      emit_line(ctx, "  sub a0, t0, a0");
      return;

    case TOKEN_STAR:
      emit_line(ctx, "  mul a0, t0, a0");
      return;

    case TOKEN_DIV:
      emit_line(ctx, "  div a0, t0, a0");
      return;

    case TOKEN_MOD:
      emit_line(ctx, "  rem a0, t0, a0");
      return;

    case TOKEN_AMPERSAND:
      emit_line(ctx, "  and a0, t0, a0");
      return;

    case TOKEN_PIPE:
      emit_line(ctx, "  or a0, t0, a0");
      return;

    case TOKEN_CARET:
      emit_line(ctx, "  xor a0, t0, a0");
      return;

    case TOKEN_LSHIFT:
      emit_line(ctx, "  sll a0, t0, a0");
      return;

    case TOKEN_RSHIFT:
      emit_line(ctx, "  sra a0, t0, a0");
      return;

    case TOKEN_LESS:
      emit_line(ctx, "  slt a0, t0, a0");
      return;

    case TOKEN_GREATER:
      emit_line(ctx, "  slt a0, a0, t0");
      return;

    case TOKEN_EQUAL:
      emit_line(ctx, "  sub a0, t0, a0");
      emit_line(ctx, "  seqz a0, a0");
      return;

    case TOKEN_NOT_EQUAL:
      emit_line(ctx, "  sub a0, t0, a0");
      emit_line(ctx, "  snez a0, a0");
      return;

    case TOKEN_LESS_EQUAL:
      emit_line(ctx, "  slt a0, a0, t0");
      emit_line(ctx, "  seqz a0, a0");
      return;

    case TOKEN_GREATER_EQUAL:
      emit_line(ctx, "  slt a0, t0, a0");
      emit_line(ctx, "  seqz a0, a0");
      return;

    case TOKEN_LOGICAL_AND:
      emit_line(ctx, "  snez t0, t0");
      emit_line(ctx, "  snez a0, a0");
      emit_line(ctx, "  and a0, t0, a0");
      return;

    case TOKEN_LOGICAL_OR:
      emit_line(ctx, "  snez t0, t0");
      emit_line(ctx, "  snez a0, a0");
      emit_line(ctx, "  or a0, t0, a0");
      return;

    default:
      codegen_error(ctx, NULL, "unsupported bin op");
      return;
  }
}

static int32_t is_array_value_expression(const CodegenContext *ctx, const AstNode *node) {
  if (!node || node->kind != AST_NODE_IDENTIFIER) {
    return 0;
  }

  CodegenSymbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
  return symbol && symbol->type.kind == AST_TYPE_ARRAY;
}

static void collect_switch_labels(CodegenContext *ctx, AstNode *node, SwitchContext *sw) {
  if (!node || ctx->had_error) {
    return;
  }

  switch (node->kind) {
    case AST_NODE_BLOCK:
      for (size_t i = 0; i < node->data.block.statements.count; i++) {
        collect_switch_labels(ctx, node->data.block.statements.items[i], sw);
      }
      return;

    case AST_NODE_IF_STMT:
      collect_switch_labels(ctx, node->data.if_stmt.then_branch, sw);
      collect_switch_labels(ctx, node->data.if_stmt.else_branch, sw);
      return;

    case AST_NODE_WHILE_STMT:
      collect_switch_labels(ctx, node->data.while_stmt.body, sw);
      return;

    case AST_NODE_FOR_STMT:
      collect_switch_labels(ctx, node->data.for_stmt.init, sw);
      collect_switch_labels(ctx, node->data.for_stmt.body, sw);
      return;

    case AST_NODE_DO_WHILE_STMT:
      collect_switch_labels(ctx, node->data.do_while_stmt.body, sw);
      return;

    case AST_NODE_SWITCH_STMT:
      return;

    case AST_NODE_CASE_STMT:
      if (node->data.case_stmt.value && node->data.case_stmt.value->kind == AST_NODE_INT_LITERAL) {
        int32_t value = node->data.case_stmt.value->data.int_literal.value;
        if (switch_case_find(sw, value)) {
          codegen_error(ctx, node, "duplicate case value '%d'", value);
          return;
        }
        switch_case_add(ctx, sw, value, "case");
      }
      collect_switch_labels(ctx, node->data.case_stmt.statement, sw);
      return;

    case AST_NODE_DEFAULT_STMT:
      if (!sw->has_default) {
        sw->has_default = 1;
        make_label(ctx, "default", sw->default_label, sizeof(sw->default_label));
      }
      collect_switch_labels(ctx, node->data.default_stmt.statement, sw);
      return;

    case AST_NODE_LABEL_STMT:
      collect_switch_labels(ctx, node->data.label_stmt.statement, sw);
      return;

    default:
      return;
  }
}

static void emit_switch_dispatch(CodegenContext *ctx, const AstNode *expr, SwitchContext *sw) {
  emit_expr(ctx, expr);
  if (ctx->had_error) {
    return;
  }

  emit_line(ctx, "  mv t0, a0");
  for (SwitchCaseLabel *it = sw->cases; it; it = it->next) {
    emit_line(ctx, "  li t1, %d", it->value);
    emit_line(ctx, "  beq t0, t1, %s", it->label);
  }

  if (sw->has_default) {
    emit_line(ctx, "  j %s", sw->default_label);
  } else {
    emit_line(ctx, "  j %s", sw->end_label);
  }
}

static void emit_break(CodegenContext *ctx, const AstNode *node) {
  const char *label = break_top(ctx);
  if (!label) {
    codegen_error(ctx, node, "'break' not in loop or switch");
    return;
  }
  emit_line(ctx, "  j %s", label);
}

static void emit_expr(CodegenContext *ctx, const AstNode *node) {
  if (!node || ctx->had_error) {
    return;
  }
  switch (node->kind) {
    case AST_NODE_INT_LITERAL:
      emit_line(ctx, "  li a0, %d", node->data.int_literal.value);
      return;

    case AST_NODE_IDENTIFIER: {
      CodegenSymbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
      if (!symbol) {
        codegen_error(ctx, node, "undeclared identifier '%s'", node->data.identifier.name);
        return;
      }
      emit_load_symbol(ctx, symbol);
      return;
    }

    case AST_NODE_UNARY_EXPR:
      emit_expr(ctx, node->data.unary.operand);
      if (ctx->had_error) {
        return;
      }
      if (is_array_value_expression(ctx, node->data.unary.operand)) {
        codegen_error(ctx, node, "array val cannot be used in unary expression");
        return;
      }
      switch (node->data.unary.op) {
        case TOKEN_PLUS:
          return;
        case TOKEN_MINUS:
          emit_line(ctx, "  sub a0, x0, a0");
          return;
        case TOKEN_EXCLAIM:
          emit_line(ctx, "  seqz a0, a0");
          return;
        case TOKEN_TILDE:
          emit_line(ctx, "  xori a0, a0, -1");
          return;
        default:
          codegen_error(ctx, node, "unsupported unary operator");
          return;
      }

    case AST_NODE_BINARY_EXPR:
      if (node->data.binary.op == TOKEN_ASSIGN) {
        if (node->data.binary.left->kind != AST_NODE_IDENTIFIER) {
          codegen_error(ctx, node, "assignement target must be a scalar identifier");
          return;
        }
        CodegenSymbol *symbol = scope_find(ctx, node->data.binary.left->data.identifier.name,
                                           node->data.binary.left->data.identifier.length);
        if (!symbol) {
          codegen_error(ctx, node->data.binary.left, "undeclared identifier '%s'",
                        node->data.binary.left->data.identifier.name);
          return;
        }
        emit_expr(ctx, node->data.binary.right);
        if (ctx->had_error) {
          return;
        }
        if (is_array_value_expression(ctx, node->data.binary.right)) {
          codegen_error(ctx, node, "array value cannot be assigned to a scalar expression");
          return;
        }
        emit_store_symbol(ctx, symbol);
        return;
      }
      emit_expr(ctx, node->data.binary.left);
      if (ctx->had_error) {
        return;
      }
      emit_push_a0(ctx);
      emit_expr(ctx, node->data.binary.right);
      if (ctx->had_error) {
        return;
      }
      if (is_array_value_expression(ctx, node->data.binary.left) ||
          is_array_value_expression(ctx, node->data.binary.right)) {
        codegen_error(ctx, node, "array value cannot be used in scalar expression");
        return;
      }
      emit_binary_stack(ctx, node->data.binary.op);
      return;

    case AST_NODE_SUBSCRIPT_EXPR:
      // TODO
      codegen_error(ctx, node, "ARRAYS ARE NOT SUPPORTED!!!!!!!");
      return;

    case AST_NODE_CALL_EXPR: {
      if (node->data.call.callee->kind != AST_NODE_IDENTIFIER) {
        codegen_error(ctx, node, "call target must be a function identifier");
        return;
      }
      if (node->data.call.args.count > 8) {
        codegen_error(ctx, node, "functions with >8 args are not supported");
        return;
      }
      const char *name = node->data.call.callee->data.identifier.name;
      size_t length = node->data.call.callee->data.identifier.length;
      for (size_t i = 0; i < node->data.call.args.count; i++) {
        emit_expr(ctx, node->data.call.args.items[i]);
        if (ctx->had_error) {
          return;
        }
        emit_push_a0(ctx);
      }
      for (size_t i = node->data.call.args.count; i > 0; i--) {
        emit_pop_t0(ctx);
        emit_line(ctx, "  mv a%zu, t0", i - 1);
      }
      emit_line(ctx, "  call %.*s", (int32_t) length, name);
      return;
    }

    case AST_NODE_INIT_LIST:
      codegen_error(ctx, node, "initilizer lists are not yet supported");
      return;

    default:
      codegen_error(ctx, node, "unsupported expr");
      return;
  }
}

static void emit_stmt(CodegenContext *ctx, const AstNode *node) {
  if (!node || ctx->had_error) {
    return;
  }

  switch (node->kind) {
    case AST_NODE_BLOCK:
      scope_push(ctx);
      for (size_t i = 0; i < node->data.block.statements.count; i++) {
        emit_stmt(ctx, node->data.block.statements.items[i]);
      }
      scope_pop(ctx);
      return;

    case AST_NODE_VAR_DECL: {
      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY) {
        // TODO
        codegen_error(ctx, node, "wait please waittt");
        return;
      }
      if (!scope_declare_current(ctx, node->data.var_decl.name, node->data.var_decl.length, node->data.var_decl.type)) {
        codegen_error(ctx, node, "duplicate declaration '%s'", node->data.var_decl.name);
        return;
      }
      CodegenSymbol *symbol = scope_find(ctx, node->data.var_decl.name, node->data.var_decl.length);
      if (node->data.var_decl.initializer) {
        emit_expr(ctx, node->data.var_decl.initializer);
        if (ctx->had_error) {
          return;
        }
      } else {
        emit_line(ctx, "  li a0, 0");
      }
      emit_store_symbol(ctx, symbol);
      return;
    }

    case AST_NODE_EXPR_STMT:
      emit_expr(ctx, node->data.expr_stmt.expr);
      return;

    case AST_NODE_RETURN_STMT:
      emit_expr(ctx, node->data.return_stmt.expr);
      if (!ctx->had_error) {
        emit_line(ctx, "  j %s_epilogue", ctx->function_name);
      }
      return;

    case AST_NODE_IF_STMT: {
      char else_label[64];
      char end_label[64];
      make_label(ctx, "else", else_label, sizeof(else_label));
      make_label(ctx, "endif", end_label, sizeof(end_label));
      emit_expr(ctx, node->data.if_stmt.condition);
      if (ctx->had_error) {
        return;
      }
      emit_line(ctx, "  beqz a0, %s", node->data.if_stmt.else_branch ? else_label : end_label);
      emit_stmt(ctx, node->data.if_stmt.then_branch);
      if (ctx->had_error) {
        return;
      }
      if (node->data.if_stmt.else_branch) {
        emit_line(ctx, "  j %s", end_label);
        emit_line(ctx, "%s:", else_label);
        emit_stmt(ctx, node->data.if_stmt.else_branch);
        if (ctx->had_error) {
          return;
        }
      }
      emit_line(ctx, "%s:", end_label);
      return;
    }

    case AST_NODE_WHILE_STMT: {
      char head_label[64];
      char end_label[64];
      make_label(ctx, "while_head", head_label, sizeof(head_label));
      make_label(ctx, "while_end", end_label, sizeof(end_label));
      break_push(ctx, end_label);
      emit_line(ctx, "%s:", head_label);
      emit_expr(ctx, node->data.while_stmt.condition);
      if (ctx->had_error) {
        break_pop(ctx);
        return;
      }
      emit_line(ctx, "  beqz a0, %s", end_label);
      emit_stmt(ctx, node->data.while_stmt.body);
      if (!ctx->had_error) {
        emit_line(ctx, "  j %s", head_label);
      }
      emit_line(ctx, "%s:", end_label);
      break_pop(ctx);
      return;
    }

    case AST_NODE_FOR_STMT: {
      char head_label[64];
      char end_label[64];
      make_label(ctx, "for_head", head_label, sizeof(head_label));
      make_label(ctx, "for_end", end_label, sizeof(end_label));
      scope_push(ctx);
      break_push(ctx, end_label);
      emit_stmt(ctx, node->data.for_stmt.init);
      if (ctx->had_error) {
        break_pop(ctx);
        scope_pop(ctx);
        return;
      }
      emit_line(ctx, "%s:", head_label);
      if (node->data.for_stmt.condition) {
        emit_expr(ctx, node->data.for_stmt.condition);
        if (ctx->had_error) {
          break_pop(ctx);
          scope_pop(ctx);
          return;
        }
        emit_line(ctx, "  beqz a0, %s", end_label);
      }
      emit_stmt(ctx, node->data.for_stmt.body);
      if (ctx->had_error) {
        break_pop(ctx);
        scope_pop(ctx);
        return;
      }
      emit_expr(ctx, node->data.for_stmt.post);
      if (!ctx->had_error) {
        emit_line(ctx, "  j %s", head_label);
      }
      emit_line(ctx, "%s:", end_label);
      break_pop(ctx);
      scope_pop(ctx);
      return;
    }

    case AST_NODE_DO_WHILE_STMT: {
      char head_label[64];
      char end_label[64];
      make_label(ctx, "do_head", head_label, sizeof(head_label));
      make_label(ctx, "do_end", end_label, sizeof(end_label));
      break_push(ctx, end_label);
      emit_line(ctx, "%s:", head_label);
      emit_stmt(ctx, node->data.do_while_stmt.body);
      if (ctx->had_error) {
        break_pop(ctx);
        return;
      }
      emit_expr(ctx, node->data.do_while_stmt.condition);
      if (ctx->had_error) {
        break_pop(ctx);
        return;
      }
      emit_line(ctx, "  bnez a0, %s", head_label);
      emit_line(ctx, "%s:", end_label);
      break_pop(ctx);
      return;
    }

    case AST_NODE_SWITCH_STMT: {
      char end_label[64];
      make_label(ctx, "switch_end", end_label, sizeof(end_label));
      SwitchContext *sw = switch_push(ctx);
      snprintf(sw->end_label, sizeof(sw->end_label), "%s", end_label);
      break_push(ctx, sw->end_label);
      collect_switch_labels(ctx, node->data.switch_stmt.body, sw);
      if (ctx->had_error) {
        break_pop(ctx);
        switch_pop(ctx);
        return;
      }
      emit_switch_dispatch(ctx, node->data.switch_stmt.expr, sw);
      if (ctx->had_error) {
        break_pop(ctx);
        switch_pop(ctx);
        return;
      }
      emit_stmt(ctx, node->data.switch_stmt.body);
      if (!ctx->had_error) {
        emit_line(ctx, "%s:", sw->end_label);
      }
      break_pop(ctx);
      switch_pop(ctx);
      return;
    }

    case AST_NODE_CASE_STMT: {
      SwitchContext *sw = switch_top(ctx);
      if (!sw) {
        codegen_error(ctx, node, "unexpected 'case'");
        return;
      }
      if (!node->data.case_stmt.value || node->data.case_stmt.value->kind != AST_NODE_INT_LITERAL) {
        codegen_error(ctx, node, "case value must be integer literal");
        return;
      }
      SwitchCaseLabel *label = switch_case_find(sw, node->data.case_stmt.value->data.int_literal.value);
      if (!label) {
        codegen_error(ctx, node, "unknown case label");
        return;
      }
      emit_line(ctx, "%s:", label->label);
      emit_stmt(ctx, node->data.case_stmt.statement);
      return;
    }

    case AST_NODE_DEFAULT_STMT: {
      SwitchContext *sw = switch_top(ctx);
      if (!sw) {
        codegen_error(ctx, node, "unexpected 'default'");
        return;
      }
      if (!sw->has_default) {
        codegen_error(ctx, node, "default label is missing");
        return;
      }
      emit_line(ctx, "%s:", sw->default_label);
      emit_stmt(ctx, node->data.default_stmt.statement);
      return;
    }

    case AST_NODE_BREAK_STMT:
      emit_break(ctx, node);
      return;

    case AST_NODE_GOTO_STMT:
      emit_line(ctx, "  j %s", node->data.goto_stmt.label);
      return;

    case AST_NODE_LABEL_STMT:
      emit_line(ctx, "%s:", node->data.label_stmt.label);
      emit_stmt(ctx, node->data.label_stmt.statement);
      return;

    default:
      codegen_error(ctx, node, "statement is not yet supported");
      return;
  }
}

static void emit_function(CodegenContext *ctx, const AstFunction *function) {
  ctx->function_name = function->name;
  ctx->label_seq = 0;
  ctx->next_slot = 0;
  ctx->scope = NULL;
  ctx->break_stack = NULL;
  ctx->switch_stack = NULL;
  ctx->had_error = 0;
  scope_push(ctx);

  for (size_t i = 0; i < function->params.count; i++) {
    const AstParam *param = &function->params.items[i];
    if (param->type.kind == AST_TYPE_ARRAY) {
      // TODO
      codegen_error(ctx, function->body, "...");
      scope_pop(ctx);
      return;
    }
    if (function->params.count > 8) {
      codegen_error(ctx, function->body, "funcs with >8 parameters are not supported");
      scope_pop(ctx);
      return;
    }
    if (!scope_declare_current(ctx, param->name, param->length, param->type)) {
      codegen_error(ctx, function->body, "duplicate parameter '%s'", param->name);
      scope_pop(ctx);
      return;
    }
  }

  int32_t local_count = count_decl_node(ctx, function->body);
  if (ctx->had_error) {
    scope_pop(ctx);
    return;
  }
  ctx->frame_size = align16(8 + (int32_t) function->params.count * 4 + local_count * 4);

  emit_line(ctx, ".globl %s", function->name);
  emit_line(ctx, "%s:", function->name);
  emit_line(ctx, "  addi sp, sp, -%d", ctx->frame_size);
  emit_line(ctx, "  sw s0, %d(sp)", ctx->frame_size - 4);
  emit_line(ctx, "  sw ra, %d(sp)", ctx->frame_size - 8);
  emit_line(ctx, "  addi s0, sp, %d", ctx->frame_size);

  for (size_t i = 0; i < function->params.count; i++) {
    CodegenSymbol *symbol = scope_find(ctx, function->params.items[i].name, function->params.items[i].length);
    emit_line(ctx, "  sw a%zu, %d(s0)", i, slot_offset(symbol->slot));
  }

  emit_stmt(ctx, function->body);
  if (!ctx->had_error) {
    emit_line(ctx, "  li a0, 0");
    emit_line(ctx, "%s_epilogue:", function->name);
    emit_line(ctx, "  lw ra, -8(s0)");
    emit_line(ctx, "  lw s0, -4(s0)");
    emit_line(ctx, "  addi sp, sp, %d", ctx->frame_size);
    if (strcmp(function->name, "main") == 0) {
      emit_line(ctx, "  li a7, 10");
      emit_line(ctx, "  ecall");
    } else {
      emit_line(ctx, "  ret");
    }
  }

  scope_pop(ctx);
}

int32_t codegen_emit_module(FILE *out, const AstModule *module, const char *filename) {
  if (!out || !module) {
    return 1;
  }
  CodegenContext ctx = {
    .out = out,
    .filename = filename,
    .function_name = NULL,
    .had_error = 0,
    .scope = NULL,
    .break_stack = NULL,
    .switch_stack = NULL,
    .next_slot = 0,
    .frame_size = 0,
    .label_seq = 0
  };

  emit_line(&ctx, ".text");
  for (size_t i = 0; i < module->functions.count; i++) {
    emit_function(&ctx, module->functions.items[i]);
    if (ctx.had_error) {
      break;
    }
  }
  while (ctx.scope) {
    scope_pop(&ctx);
  }

  return ctx.had_error;
}
