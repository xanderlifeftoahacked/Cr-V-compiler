#include "codegen_internal.h"

#include <stdio.h>

static void emit_string_bytes(CodegenContext *ctx, const AstNode *node) {
  for (size_t i = 0; i < node->data.string_literal.length; i++) {
    emit_line(ctx, "  .byte %u", (unsigned char) node->data.string_literal.value[i]);
  }
  emit_line(ctx, "  .byte 0");
}

static void emit_string_literal_node(CodegenContext *ctx, const AstNode *node, int32_t *emitted_data_section) {
  if (!node || node->kind != AST_NODE_STRING_LITERAL) {
    return;
  }

  register_string_label(ctx, node);
  if (!*emitted_data_section) {
    emit_line(ctx, ".data");
    *emitted_data_section = 1;
  }

  emit_string_label_ref(ctx, node);
  fprintf(ctx->out, ":\n");
  emit_string_bytes(ctx, node);
}

static void collect_string_literals_from_node(CodegenContext *ctx, const AstNode *node, int32_t *emitted_data_section) {
  if (!node || ctx->had_error) {
    return;
  }

  switch (node->kind) {
    case AST_NODE_STRING_LITERAL:
      emit_string_literal_node(ctx, node, emitted_data_section);
      return;
    case AST_NODE_BLOCK:
      for (size_t i = 0; i < node->data.block.statements.count; i++) {
        collect_string_literals_from_node(ctx, node->data.block.statements.items[i], emitted_data_section);
      }
      return;
    case AST_NODE_RETURN_STMT:
      collect_string_literals_from_node(ctx, node->data.return_stmt.expr, emitted_data_section);
      return;
    case AST_NODE_EXPR_STMT:
      collect_string_literals_from_node(ctx, node->data.expr_stmt.expr, emitted_data_section);
      return;
    case AST_NODE_VAR_DECL:
      if (node->data.var_decl.type.kind == AST_TYPE_ARRAY && node->data.var_decl.initializer &&
          node->data.var_decl.initializer->kind == AST_NODE_STRING_LITERAL) {
        return;
      }
      collect_string_literals_from_node(ctx, node->data.var_decl.initializer, emitted_data_section);
      return;
    case AST_NODE_IF_STMT:
      collect_string_literals_from_node(ctx, node->data.if_stmt.condition, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.if_stmt.then_branch, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.if_stmt.else_branch, emitted_data_section);
      return;
    case AST_NODE_WHILE_STMT:
      collect_string_literals_from_node(ctx, node->data.while_stmt.condition, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.while_stmt.body, emitted_data_section);
      return;
    case AST_NODE_FOR_STMT:
      collect_string_literals_from_node(ctx, node->data.for_stmt.init, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.for_stmt.condition, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.for_stmt.post, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.for_stmt.body, emitted_data_section);
      return;
    case AST_NODE_DO_WHILE_STMT:
      collect_string_literals_from_node(ctx, node->data.do_while_stmt.body, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.do_while_stmt.condition, emitted_data_section);
      return;
    case AST_NODE_SWITCH_STMT:
      collect_string_literals_from_node(ctx, node->data.switch_stmt.expr, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.switch_stmt.body, emitted_data_section);
      return;
    case AST_NODE_CASE_STMT:
      collect_string_literals_from_node(ctx, node->data.case_stmt.value, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.case_stmt.statement, emitted_data_section);
      return;
    case AST_NODE_DEFAULT_STMT:
      collect_string_literals_from_node(ctx, node->data.default_stmt.statement, emitted_data_section);
      return;
    case AST_NODE_LABEL_STMT:
      collect_string_literals_from_node(ctx, node->data.label_stmt.statement, emitted_data_section);
      return;
    case AST_NODE_BINARY_EXPR:
      collect_string_literals_from_node(ctx, node->data.binary.left, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.binary.right, emitted_data_section);
      return;
    case AST_NODE_UNARY_EXPR:
      collect_string_literals_from_node(ctx, node->data.unary.operand, emitted_data_section);
      return;
    case AST_NODE_SUBSCRIPT_EXPR:
      collect_string_literals_from_node(ctx, node->data.subscript.base, emitted_data_section);
      collect_string_literals_from_node(ctx, node->data.subscript.index, emitted_data_section);
      return;
    case AST_NODE_CALL_EXPR:
      collect_string_literals_from_node(ctx, node->data.call.callee, emitted_data_section);
      for (size_t i = 0; i < node->data.call.args.count; i++) {
        collect_string_literals_from_node(ctx, node->data.call.args.items[i], emitted_data_section);
      }
      return;
    case AST_NODE_INIT_LIST:
      for (size_t i = 0; i < node->data.init_list.elements.count; i++) {
        collect_string_literals_from_node(ctx, node->data.init_list.elements.items[i], emitted_data_section);
      }
      return;
    default:
      return;
  }
}

void emit_string_literals(CodegenContext *ctx, const AstModule *module) {
  int32_t emitted_data_section = 0;

  for (size_t i = 0; i < module->globals.count; i++) {
    collect_string_literals_from_node(ctx, module->globals.items[i], &emitted_data_section);
  }

  for (size_t i = 0; i < module->functions.count; i++) {
    const AstFunction *function = module->functions.items[i];
    if (function) {
      collect_string_literals_from_node(ctx, function->body, &emitted_data_section);
    }
  }

  if (emitted_data_section) {
    emit_line(ctx, "  .align 2");
  }
}

static int32_t int_literal_value(CodegenContext *ctx, const AstNode *node) {
  if (!node || node->kind != AST_NODE_INT_LITERAL) {
    codegen_error(ctx, node, "global initializer must be an integer literal");
    return 0;
  }

  return node->data.int_literal.value;
}

static void emit_scalar_data(CodegenContext *ctx, AstType type, int32_t value) {
  if (type.kind == AST_TYPE_CHAR) {
    emit_line(ctx, "  .byte %d", value);
  } else {
    emit_line(ctx, "  .word %d", value);
  }
}

static void emit_array_data(CodegenContext *ctx, const AstNode *global) {
  AstType element_type = array_element_type(global->data.var_decl.type);
  int32_t emitted = 0;

  if (global->data.var_decl.initializer) {
    const AstNode *initializer = global->data.var_decl.initializer;
    if (initializer->kind == AST_NODE_STRING_LITERAL) {
      for (size_t i = 0; i < initializer->data.string_literal.length; i++) {
        emit_scalar_data(ctx, element_type, (unsigned char) initializer->data.string_literal.value[i]);
        emitted++;
      }
      if (emitted < global->data.var_decl.type.array_size) {
        emit_scalar_data(ctx, element_type, 0);
        emitted++;
      }
    } else {
      for (size_t i = 0; i < initializer->data.init_list.elements.count; i++) {
        int32_t value = int_literal_value(ctx, initializer->data.init_list.elements.items[i]);
        if (ctx->had_error) {
          return;
        }

        emit_scalar_data(ctx, element_type, value);
        emitted++;
      }
    }
  }

  while (emitted < global->data.var_decl.type.array_size) {
    emit_scalar_data(ctx, element_type, 0);
    emitted++;
  }
}

void emit_global_data(CodegenContext *ctx, const AstModule *module) {
  if (module->globals.count == 0) {
    return;
  }

  int32_t emitted_data_section = 0;
  for (size_t i = 0; i < module->globals.count; i++) {
    const AstNode *global = module->globals.items[i];
    if (!global || global->kind != AST_NODE_VAR_DECL) {
      continue;
    }

    if (!scope_declare_global(ctx, global)) {
      codegen_error(ctx, global, "duplicate global declaration '%s'", global->data.var_decl.name);
      return;
    }

    if (global->data.var_decl.storage == AST_STORAGE_EXTERN) {
      continue;
    }

    if (!emitted_data_section) {
      emit_line(ctx, ".data");
      emitted_data_section = 1;
    }

    if (global->data.var_decl.storage != AST_STORAGE_STATIC) {
      emit_line(ctx, ".globl %s", global->data.var_decl.name);
      emit_line(ctx, "%s:", global->data.var_decl.name);
    } else {
      emit_static_label_ref(ctx, global->data.var_decl.filename, global->data.var_decl.name,
                            global->data.var_decl.length);
      fprintf(ctx->out, ":\n");
    }

    if (global->data.var_decl.type.kind == AST_TYPE_ARRAY) {
      emit_array_data(ctx, global);
      continue;
    }

    int32_t value = 0;
    if (global->data.var_decl.initializer) {
      if (global->data.var_decl.initializer->kind == AST_NODE_STRING_LITERAL) {
        fprintf(ctx->out, "  .word ");
        emit_string_label_ref(ctx, global->data.var_decl.initializer);
        fprintf(ctx->out, "\n");
        continue;
      }
      value = int_literal_value(ctx, global->data.var_decl.initializer);
      if (ctx->had_error) {
        return;
      }
    }
    emit_scalar_data(ctx, global->data.var_decl.type, value);
  }
}
