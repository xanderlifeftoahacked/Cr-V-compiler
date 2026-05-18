#pragma once

#include "codegen/codegen.h"

typedef enum {
  CODEGEN_STORAGE_LOCAL,
  CODEGEN_STORAGE_GLOBAL
} CodegenStorage;

typedef struct CodegenSymbol {
  const char *name;
  size_t length;
  AstType type;
  CodegenStorage storage;
  int32_t offset;
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
  int32_t next_local_offset;
  int32_t frame_size;
  int32_t label_seq;
} CodegenContext;

int32_t names_equal(const char *lhs, size_t lhs_len, const char *rhs, size_t rhs_len);
int32_t align16(int32_t value);
int32_t align_to(int32_t value, int32_t alignment);
int32_t type_size(AstType type);
int32_t type_align(AstType type);
AstType array_element_type(AstType type);
AstType pointer_element_type(AstType type);
void codegen_error(CodegenContext *ctx, const AstNode *node, const char *fmt, ...);
void make_label(CodegenContext *ctx, const char *suffix, char *buffer, size_t size);
void emit_line(CodegenContext *ctx, const char *fmt, ...);

CodegenScope *scope_push(CodegenContext *ctx);
void scope_pop(CodegenContext *ctx);
CodegenSymbol *scope_find(const CodegenContext *ctx, const char *name, size_t length);
CodegenSymbol *scope_declare_current(CodegenContext *ctx, const char *name, size_t length, AstType type,
                                      CodegenStorage storage);

BreakTarget *break_push(CodegenContext *ctx, const char *label);
void break_pop(CodegenContext *ctx);
const char *break_top(const CodegenContext *ctx);

SwitchContext *switch_push(CodegenContext *ctx);
void switch_pop(CodegenContext *ctx);
SwitchContext *switch_top(CodegenContext *ctx);
SwitchCaseLabel *switch_case_find(SwitchContext *sw, int32_t value);
SwitchCaseLabel *switch_case_add(CodegenContext *ctx, SwitchContext *sw, int32_t value, const char *suffix);

int32_t measure_decl_node(CodegenContext *ctx, const AstNode *node, int32_t used_bytes);
int32_t is_array_value_expression(const CodegenContext *ctx, const AstNode *node);
void emit_global_data(CodegenContext *ctx, const AstModule *module);
void collect_switch_labels(CodegenContext *ctx, AstNode *node, SwitchContext *sw);
void emit_switch_dispatch(CodegenContext *ctx, const AstNode *expr, SwitchContext *sw);
void emit_break(CodegenContext *ctx, const AstNode *node);

void emit_expr(CodegenContext *ctx, const AstNode *node);
void emit_stmt(CodegenContext *ctx, const AstNode *node);
void emit_function(CodegenContext *ctx, const AstFunction *function);
