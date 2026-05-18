#include "codegen_internal.h"

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
      .next_local_offset = 8,
      .frame_size = 0,
      .label_seq = 0,
  };

  scope_push(&ctx);
  emit_global_data(&ctx, module);
  if (ctx.had_error) {
    goto cleanup;
  }

  emit_line(&ctx, ".text");
  for (size_t i = 0; i < module->functions.count; i++) {
    emit_function(&ctx, module->functions.items[i]);
    if (ctx.had_error) {
      break;
    }
  }

cleanup:
  while (ctx.break_stack) {
    break_pop(&ctx);
  }
  while (ctx.switch_stack) {
    switch_pop(&ctx);
  }
  while (ctx.scope) {
    scope_pop(&ctx);
  }

  return ctx.had_error;
}
