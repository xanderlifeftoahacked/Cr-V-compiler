#include "codegen_internal.h"

#include <string.h>

void emit_function(CodegenContext *ctx, const AstFunction *function) {
  ctx->function_name = function->name;
  ctx->label_seq = 0;
  ctx->next_local_offset = 8;
  ctx->break_stack = NULL;
  ctx->switch_stack = NULL;
  ctx->had_error = 0;

  int32_t measured_bytes = 8;
  for (size_t i = 0; i < function->params.count; i++) {
    measured_bytes = align_to(measured_bytes, type_align(function->params.items[i].type));
    measured_bytes += type_size(function->params.items[i].type);
  }
  measured_bytes = measure_decl_node(ctx, function->body, measured_bytes);

  scope_push(ctx);
  for (size_t i = 0; i < function->params.count; i++) {
    const AstParam *param = &function->params.items[i];
    if (param->type.kind == AST_TYPE_ARRAY) {
      codegen_error(ctx, function->body, "array parameter was not lowered to a pointer parameter");
      scope_pop(ctx);
      return;
    }
    if (function->params.count > 8) {
      codegen_error(ctx, function->body, "functions with more than 8 parameters are not supported");
      scope_pop(ctx);
      return;
    }
    CodegenSymbol *symbol = scope_declare_current(ctx, param->name, param->length, param->type, CODEGEN_STORAGE_LOCAL);
    if (!symbol) {
      codegen_error(ctx, function->body, "duplicate parameter '%s'", param->name);
      scope_pop(ctx);
      return;
    }
  }

  if (ctx->had_error) {
    scope_pop(ctx);
    return;
  }

  ctx->frame_size = align16(measured_bytes);
  emit_line(ctx, ".globl %s", function->name);
  emit_line(ctx, "%s:", function->name);
  emit_line(ctx, "  addi sp, sp, -%d", ctx->frame_size);
  emit_line(ctx, "  sw s0, %d(sp)", ctx->frame_size - 4);
  emit_line(ctx, "  sw ra, %d(sp)", ctx->frame_size - 8);
  emit_line(ctx, "  addi s0, sp, %d", ctx->frame_size);

  for (size_t i = 0; i < function->params.count; i++) {
    CodegenSymbol *symbol = scope_find(ctx, function->params.items[i].name, function->params.items[i].length);
    if (function->params.items[i].type.kind == AST_TYPE_CHAR) {
      emit_line(ctx, "  sb a%zu, %d(s0)", i, symbol->offset);
    } else {
      emit_line(ctx, "  sw a%zu, %d(s0)", i, symbol->offset);
    }
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
