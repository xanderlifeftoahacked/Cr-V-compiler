#include "codegen_internal.h"

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
    for (size_t i = 0; i < initializer->data.init_list.elements.count; i++) {
      int32_t value = int_literal_value(ctx, initializer->data.init_list.elements.items[i]);
      if (ctx->had_error) {
        return;
      }

      emit_scalar_data(ctx, element_type, value);
      emitted++;
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

  emit_line(ctx, ".data");
  for (size_t i = 0; i < module->globals.count; i++) {
    const AstNode *global = module->globals.items[i];
    if (!global || global->kind != AST_NODE_VAR_DECL) {
      continue;
    }

    if (!scope_declare_current(ctx, global->data.var_decl.name, global->data.var_decl.length,
                               global->data.var_decl.type, CODEGEN_STORAGE_GLOBAL)) {
      codegen_error(ctx, global, "duplicate global declaration '%s'", global->data.var_decl.name);
      return;
    }

    emit_line(ctx, ".globl %s", global->data.var_decl.name);
    emit_line(ctx, "%s:", global->data.var_decl.name);

    if (global->data.var_decl.type.kind == AST_TYPE_ARRAY) {
      emit_array_data(ctx, global);
      continue;
    }

    int32_t value = 0;
    if (global->data.var_decl.initializer) {
      value = int_literal_value(ctx, global->data.var_decl.initializer);
      if (ctx->had_error) {
        return;
      }
    }
    emit_scalar_data(ctx, global->data.var_decl.type, value);
  }
}
