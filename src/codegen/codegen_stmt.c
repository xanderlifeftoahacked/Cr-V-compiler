#include "codegen_internal.h"

static void emit_store_symbol_value(CodegenContext *ctx, const CodegenSymbol *symbol) {
  if (symbol->storage == CODEGEN_STORAGE_GLOBAL) {
    emit_line(ctx, "  la t0, %.*s", (int32_t) symbol->length, symbol->name);
  } else {
    emit_line(ctx, "  addi t0, s0, %d", symbol->offset);
  }

  if (symbol->type.kind == AST_TYPE_CHAR) {
    emit_line(ctx, "  sb a0, 0(t0)");
  } else {
    emit_line(ctx, "  sw a0, 0(t0)");
  }
}

static void emit_store_array_element(CodegenContext *ctx, const CodegenSymbol *symbol, AstType element_type,
                                     int32_t index) {
  int32_t offset = symbol->offset + index * type_size(element_type);

  if (symbol->storage == CODEGEN_STORAGE_GLOBAL) {
    emit_line(ctx, "  la t0, %.*s", (int32_t) symbol->length, symbol->name);
    if (index > 0) {
      emit_line(ctx, "  addi t0, t0, %d", index * type_size(element_type));
    }
  } else {
    emit_line(ctx, "  addi t0, s0, %d", offset);
  }

  if (element_type.kind == AST_TYPE_CHAR) {
    emit_line(ctx, "  sb a0, 0(t0)");
  } else {
    emit_line(ctx, "  sw a0, 0(t0)");
  }
}

static void emit_array_initializer(CodegenContext *ctx, const AstNode *node, const CodegenSymbol *symbol) {
  AstType element_type = array_element_type(node->data.var_decl.type);
  int32_t count = node->data.var_decl.type.array_size;
  size_t initializer_count = 0;

  if (node->data.var_decl.initializer) {
    if (node->data.var_decl.initializer->kind == AST_NODE_STRING_LITERAL) {
      initializer_count = node->data.var_decl.initializer->data.string_literal.length + 1;
    } else {
      initializer_count = node->data.var_decl.initializer->data.init_list.elements.count;
    }
  }

  for (int32_t i = 0; i < count; i++) {
    if ((size_t) i < initializer_count) {
      if (node->data.var_decl.initializer->kind == AST_NODE_STRING_LITERAL) {
        const AstNode *literal = node->data.var_decl.initializer;
        unsigned char value = 0;
        if ((size_t) i < literal->data.string_literal.length) {
          value = (unsigned char) literal->data.string_literal.value[i];
        }
        emit_line(ctx, "  li a0, %u", value);
      } else {
        emit_expr(ctx, node->data.var_decl.initializer->data.init_list.elements.items[i]);
        if (ctx->had_error) {
          return;
        }
      }
    } else {
      emit_line(ctx, "  li a0, 0");
    }

    emit_store_array_element(ctx, symbol, element_type, i);
  }
}

void emit_stmt(CodegenContext *ctx, const AstNode *node) {
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
      CodegenSymbol *symbol = scope_declare_current(ctx, node->data.var_decl.name, node->data.var_decl.length,
                                                    node->data.var_decl.type, CODEGEN_STORAGE_LOCAL);
      if (!symbol) {
        codegen_error(ctx, node, "duplicate declaration '%s'", node->data.var_decl.name);
        return;
      }

      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY) {
        emit_array_initializer(ctx, node, symbol);
        return;
      }

      if (node->data.var_decl.initializer) {
        emit_expr(ctx, node->data.var_decl.initializer);
        if (ctx->had_error) {
          return;
        }
      } else {
        emit_line(ctx, "  li a0, 0");
      }
      emit_store_symbol_value(ctx, symbol);
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
      continue_push(ctx, head_label);
      emit_line(ctx, "%s:", head_label);
      emit_expr(ctx, node->data.while_stmt.condition);
      if (ctx->had_error) {
        continue_pop(ctx);
        break_pop(ctx);
        return;
      }
      emit_line(ctx, "  beqz a0, %s", end_label);
      emit_stmt(ctx, node->data.while_stmt.body);
      if (!ctx->had_error) {
        emit_line(ctx, "  j %s", head_label);
      }
      emit_line(ctx, "%s:", end_label);
      continue_pop(ctx);
      break_pop(ctx);
      return;
    }
    case AST_NODE_FOR_STMT: {
      char head_label[64];
      char post_label[64];
      char end_label[64];
      make_label(ctx, "for_head", head_label, sizeof(head_label));
      make_label(ctx, "for_post", post_label, sizeof(post_label));
      make_label(ctx, "for_end", end_label, sizeof(end_label));
      scope_push(ctx);
      break_push(ctx, end_label);
      continue_push(ctx, post_label);
      emit_stmt(ctx, node->data.for_stmt.init);
      if (ctx->had_error) {
        continue_pop(ctx);
        break_pop(ctx);
        scope_pop(ctx);
        return;
      }
      emit_line(ctx, "%s:", head_label);
      if (node->data.for_stmt.condition) {
        emit_expr(ctx, node->data.for_stmt.condition);
        if (ctx->had_error) {
          continue_pop(ctx);
          break_pop(ctx);
          scope_pop(ctx);
          return;
        }
        emit_line(ctx, "  beqz a0, %s", end_label);
      }
      emit_stmt(ctx, node->data.for_stmt.body);
      if (ctx->had_error) {
        continue_pop(ctx);
        break_pop(ctx);
        scope_pop(ctx);
        return;
      }
      emit_line(ctx, "%s:", post_label);
      emit_expr(ctx, node->data.for_stmt.post);
      if (!ctx->had_error) {
        emit_line(ctx, "  j %s", head_label);
      }
      emit_line(ctx, "%s:", end_label);
      continue_pop(ctx);
      break_pop(ctx);
      scope_pop(ctx);
      return;
    }
    case AST_NODE_DO_WHILE_STMT: {
      char head_label[64];
      char continue_label[64];
      char end_label[64];
      make_label(ctx, "do_head", head_label, sizeof(head_label));
      make_label(ctx, "do_continue", continue_label, sizeof(continue_label));
      make_label(ctx, "do_end", end_label, sizeof(end_label));
      break_push(ctx, end_label);
      continue_push(ctx, continue_label);
      emit_line(ctx, "%s:", head_label);
      emit_stmt(ctx, node->data.do_while_stmt.body);
      if (ctx->had_error) {
        continue_pop(ctx);
        break_pop(ctx);
        return;
      }
      emit_line(ctx, "%s:", continue_label);
      emit_expr(ctx, node->data.do_while_stmt.condition);
      if (ctx->had_error) {
        continue_pop(ctx);
        break_pop(ctx);
        return;
      }
      emit_line(ctx, "  bnez a0, %s", head_label);
      emit_line(ctx, "%s:", end_label);
      continue_pop(ctx);
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
    case AST_NODE_CONTINUE_STMT:
      emit_continue(ctx, node);
      return;
    case AST_NODE_GOTO_STMT:
      emit_line(ctx, "  j %s", node->data.goto_stmt.label);
      return;
    case AST_NODE_LABEL_STMT:
      emit_line(ctx, "%s:", node->data.label_stmt.label);
      emit_stmt(ctx, node->data.label_stmt.statement);
      return;
    default:
      codegen_error(ctx, node, "statement is not yet supported by the backend");
      return;
  }
}
