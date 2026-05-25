#include "codegen_internal.h"

#include "utils/diagnostic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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

static int32_t find_string_label(const CodegenContext *ctx, const AstNode *node) {
  for (const CodegenStringLabel *it = ctx->string_labels; it; it = it->next) {
    if (it->node == node) {
      return it->id;
    }
  }
  return -1;
}

int32_t register_string_label(CodegenContext *ctx, const AstNode *node) {
  int32_t existing = find_string_label(ctx, node);
  if (existing >= 0) {
    return existing;
  }

  CodegenStringLabel *label = malloc(sizeof(CodegenStringLabel));
  if (!label) {
    LOG(FATAL, "out of memory");
  }
  label->node = node;
  label->id = ctx->string_seq++;
  label->next = ctx->string_labels;
  ctx->string_labels = label;
  return label->id;
}

void emit_string_label_ref(CodegenContext *ctx, const AstNode *node) {
  int32_t label = find_string_label(ctx, node);
  if (label < 0) {
    codegen_error(ctx, node, "internal error: unregistered string literal");
    label = 0;
  }
  fprintf(ctx->out, "__crv_str_%d", label);
}

void string_labels_destroy(CodegenContext *ctx) {
  CodegenStringLabel *label = ctx->string_labels;
  while (label) {
    CodegenStringLabel *next = label->next;
    free(label);
    label = next;
  }
  ctx->string_labels = NULL;
}

static int32_t filenames_equal(const char *lhs, const char *rhs) {
  if (!lhs || !rhs) {
    return lhs == rhs;
  }
  return strcmp(lhs, rhs) == 0;
}

static int32_t filename_seen_before(const CodegenContext *ctx, const char *filename, size_t global_limit,
                                    size_t function_limit) {
  if (!ctx || !ctx->module || !filename) {
    return 0;
  }

  for (size_t i = 0; i < global_limit; i++) {
    const AstNode *global = ctx->module->globals.items[i];
    if (global && global->kind == AST_NODE_VAR_DECL && filenames_equal(global->data.var_decl.filename, filename)) {
      return 1;
    }
  }

  for (size_t i = 0; i < function_limit; i++) {
    const AstFunction *fn = ctx->module->functions.items[i];
    if (fn && filenames_equal(fn->filename, filename)) {
      return 1;
    }
  }

  return 0;
}

static size_t static_file_index(const CodegenContext *ctx, const char *filename) {
  if (!ctx || !ctx->module || !filename) {
    return 0;
  }

  size_t index = 0;
  for (size_t i = 0; i < ctx->module->globals.count; i++) {
    const AstNode *global = ctx->module->globals.items[i];
    if (!global || global->kind != AST_NODE_VAR_DECL || !global->data.var_decl.filename ||
        filename_seen_before(ctx, global->data.var_decl.filename, i, 0)) {
      continue;
    }
    if (filenames_equal(global->data.var_decl.filename, filename)) {
      return index;
    }
    index++;
  }

  for (size_t i = 0; i < ctx->module->functions.count; i++) {
    const AstFunction *fn = ctx->module->functions.items[i];
    if (!fn || !fn->filename || filename_seen_before(ctx, fn->filename, ctx->module->globals.count, i)) {
      continue;
    }
    if (filenames_equal(fn->filename, filename)) {
      return index;
    }
    index++;
  }

  return index;
}

void format_static_label(const CodegenContext *ctx, char *buffer, size_t size, const char *filename, const char *name,
                         size_t length) {
  snprintf(buffer, size, "__crv_static_%zu_%.*s", static_file_index(ctx, filename), (int32_t) length, name);
}

void emit_static_label_ref(CodegenContext *ctx, const char *filename, const char *name, size_t length) {
  char label[128];
  format_static_label(ctx, label, sizeof(label), filename, name, length);
  fprintf(ctx->out, "%s", label);
}

const AstFunction *codegen_find_function(const CodegenContext *ctx, const char *name, size_t length) {
  const AstFunction *fallback = NULL;
  if (!ctx || !ctx->module) {
    return NULL;
  }
  for (size_t i = 0; i < ctx->module->functions.count; i++) {
    const AstFunction *fn = ctx->module->functions.items[i];
    if (!fn || !names_equal(fn->name, fn->length, name, length)) {
      continue;
    }
    if (fn->storage == AST_STORAGE_STATIC) {
      if (fn->filename && ctx->current_file && strcmp(fn->filename, ctx->current_file) == 0) {
        return fn;
      }
      continue;
    }
    if (!fallback || (!fallback->body && fn->body)) {
      fallback = fn;
    }
  }
  return fallback;
}
