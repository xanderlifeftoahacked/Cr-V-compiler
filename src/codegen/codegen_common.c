#include "codegen_internal.h"

#include "utils/diagnostic.h"

#include <stdarg.h>
#include <string.h>

int32_t names_equal(const char *lhs, size_t lhs_len, const char *rhs, size_t rhs_len) {
  return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

int32_t align16(int32_t value) {
  return (value + 15) & ~15;
}

int32_t align_to(int32_t value, int32_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

AstType array_element_type(AstType type) {
  AstType element = {
      .kind = type.element_kind,
      .element_kind = type.element_kind,
      .array_size = 0,
  };
  return element;
}

AstType pointer_element_type(AstType type) {
  AstType element = {
      .kind = type.element_kind,
      .element_kind = type.element_kind,
      .array_size = 0,
  };
  return element;
}

int32_t type_size(AstType type) {
  if (type.kind == AST_TYPE_ARRAY) {
    return type.array_size * type_size(array_element_type(type));
  }

  if (type.kind == AST_TYPE_CHAR) {
    return 1;
  }

  return 4;
}

int32_t type_align(AstType type) {
  if (type.kind == AST_TYPE_ARRAY) {
    return type_align(array_element_type(type));
  }

  if (type.kind == AST_TYPE_CHAR) {
    return 1;
  }

  return 4;
}

void codegen_error(CodegenContext *ctx, const AstNode *node, const char *fmt, ...) {
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

void make_label(CodegenContext *ctx, const char *suffix, char *buffer, size_t size) {
  snprintf(buffer, size, "%s_%s_%d", ctx->function_name, suffix, ctx->label_seq++);
}

void emit_line(CodegenContext *ctx, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(ctx->out, fmt, args);
  va_end(args);
  fputc('\n', ctx->out);
}
