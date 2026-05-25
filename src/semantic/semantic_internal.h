#pragma once

#include "semantic/semantic.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct Symbol {
  const char *name;
  size_t length;
  AstType type;
  AstStorageClass storage;
  const char *filename;
  int32_t is_definition;
  struct Symbol *next;
} Symbol;

typedef struct Scope {
  Symbol *symbols;
  struct Scope *parent;
} Scope;

typedef struct FunctionSymbol {
  const char *name;
  size_t length;
  size_t param_count;
  const AstFunction *function;
  AstStorageClass storage;
  const char *filename;
  int32_t is_definition;
  struct FunctionSymbol *next;
} FunctionSymbol;

typedef struct {
  const char *name;
  size_t param_count;
} BuiltinFunction;

typedef struct LabelSymbol {
  const char *name;
  size_t length;
  const AstNode *node;
  struct LabelSymbol *next;
} LabelSymbol;

typedef struct GotoUse {
  const char *name;
  size_t length;
  const AstNode *node;
  struct GotoUse *next;
} GotoUse;

typedef struct IntValueNode {
  int32_t value;
  struct IntValueNode *next;
} IntValueNode;

typedef struct SwitchContext {
  IntValueNode *cases;
  int32_t has_default;
  struct SwitchContext *next;
} SwitchContext;

typedef struct {
  const char *filename;
  int32_t had_error;
  Scope *scope;
  FunctionSymbol *functions;
  LabelSymbol *labels;
  GotoUse *gotos;
  SwitchContext *switch_stack;
  int32_t loop_depth;
  int32_t switch_depth;
  AstType current_return_type;
  const char *current_file;
} SemanticContext;

int32_t sem_names_equal(const char *lhs, size_t lhs_len, const char *rhs, size_t rhs_len);
void semantic_error(SemanticContext *ctx, const AstNode *node, const char *fmt, ...);

Scope *sem_scope_push(SemanticContext *ctx);
void sem_scope_pop(SemanticContext *ctx);
int32_t sem_scope_declare_current(SemanticContext *ctx, const char *name, size_t length, AstType type);
int32_t sem_scope_declare_global(SemanticContext *ctx, const AstNode *global);
const Symbol *sem_scope_find(const SemanticContext *ctx, const char *name, size_t length);
int32_t sem_scope_lookup(const SemanticContext *ctx, const char *name, size_t length);

void function_add(SemanticContext *ctx, const AstFunction *function);
const FunctionSymbol *function_find(const SemanticContext *ctx, const char *name, size_t length);
const BuiltinFunction *builtin_find(const char *name, size_t length);
void functions_destroy(SemanticContext *ctx);

void labels_destroy(SemanticContext *ctx);
void gotos_destroy(SemanticContext *ctx);
void switch_context_pop(SemanticContext *ctx);
void switch_context_push(SemanticContext *ctx);
int32_t switch_context_has_case(const SwitchContext *ctx, int32_t value);
void switch_context_add_case(SwitchContext *ctx, int32_t value);
void label_add(SemanticContext *ctx, const AstNode *node, const char *name, size_t length);
void goto_add(SemanticContext *ctx, const AstNode *node, const char *name, size_t length);
void validate_gotos(SemanticContext *ctx);
