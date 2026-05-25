#include "codegen_internal.h"

#include "utils/diagnostic.h"

#include <stdlib.h>

BreakTarget *break_push(CodegenContext *ctx, const char *label) {
  BreakTarget *target = malloc(sizeof(BreakTarget));
  if (!target) {
    LOG(FATAL, "out of memory");
  }
  snprintf(target->label, sizeof(target->label), "%s", label);
  target->next = ctx->break_stack;
  ctx->break_stack = target;
  return target;
}

void break_pop(CodegenContext *ctx) {
  BreakTarget *target = ctx->break_stack;
  if (!target) {
    return;
  }
  ctx->break_stack = target->next;
  free(target);
}

const char *break_top(const CodegenContext *ctx) {
  return ctx->break_stack ? ctx->break_stack->label : NULL;
}

BreakTarget *continue_push(CodegenContext *ctx, const char *label) {
  BreakTarget *target = malloc(sizeof(BreakTarget));
  if (!target) {
    LOG(FATAL, "out of memory");
  }
  snprintf(target->label, sizeof(target->label), "%s", label);
  target->next = ctx->continue_stack;
  ctx->continue_stack = target;
  return target;
}

void continue_pop(CodegenContext *ctx) {
  BreakTarget *target = ctx->continue_stack;
  if (!target) {
    return;
  }
  ctx->continue_stack = target->next;
  free(target);
}

const char *continue_top(const CodegenContext *ctx) {
  return ctx->continue_stack ? ctx->continue_stack->label : NULL;
}

SwitchContext *switch_push(CodegenContext *ctx) {
  SwitchContext *sw = malloc(sizeof(SwitchContext));
  if (!sw) {
    LOG(FATAL, "out of memory");
  }
  sw->end_label[0] = '\0';
  sw->default_label[0] = '\0';
  sw->has_default = 0;
  sw->cases = NULL;
  sw->next = ctx->switch_stack;
  ctx->switch_stack = sw;
  return sw;
}

void switch_pop(CodegenContext *ctx) {
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

SwitchContext *switch_top(CodegenContext *ctx) {
  return ctx->switch_stack;
}

SwitchCaseLabel *switch_case_find(SwitchContext *sw, int32_t value) {
  for (SwitchCaseLabel *it = sw->cases; it; it = it->next) {
    if (it->value == value) {
      return it;
    }
  }
  return NULL;
}

SwitchCaseLabel *switch_case_add(CodegenContext *ctx, SwitchContext *sw, int32_t value, const char *suffix) {
  SwitchCaseLabel *label = malloc(sizeof(SwitchCaseLabel));
  if (!label) {
    LOG(FATAL, "out of memory");
  }
  label->value = value;
  make_label(ctx, suffix, label->label, sizeof(label->label));
  label->next = sw->cases;
  sw->cases = label;
  return label;
}

void collect_switch_labels(CodegenContext *ctx, AstNode *node, SwitchContext *sw) {
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

void emit_switch_dispatch(CodegenContext *ctx, const AstNode *expr, SwitchContext *sw) {
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

void emit_break(CodegenContext *ctx, const AstNode *node) {
  const char *label = break_top(ctx);
  if (!label) {
    codegen_error(ctx, node, "'break' not within loop or switch");
    return;
  }
  emit_line(ctx, "  j %s", label);
}

void emit_continue(CodegenContext *ctx, const AstNode *node) {
  const char *label = continue_top(ctx);
  if (!label) {
    codegen_error(ctx, node, "'continue' not within loop");
    return;
  }
  emit_line(ctx, "  j %s", label);
}
