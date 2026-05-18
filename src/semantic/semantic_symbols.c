#include "semantic_internal.h"

#include <stdlib.h>

#include "utils/diagnostic.h"

Scope *sem_scope_push(SemanticContext *ctx) {
  Scope *scope = malloc(sizeof(Scope));
  if (!scope) {
    LOG(FATAL, "out of memory");
  }

  scope->symbols = NULL;
  scope->parent = ctx->scope;
  ctx->scope = scope;
  return scope;
}

void sem_scope_pop(SemanticContext *ctx) {
  Scope *scope = ctx->scope;
  if (!scope) {
    return;
  }

  Symbol *symbol = scope->symbols;
  while (symbol) {
    Symbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }

  ctx->scope = scope->parent;
  free(scope);
}

int32_t sem_scope_declare_current(SemanticContext *ctx, const char *name, size_t length, AstType type) {
  if (!ctx->scope) {
    sem_scope_push(ctx);
  }

  for (Symbol *it = ctx->scope->symbols; it; it = it->next) {
    if (sem_names_equal(it->name, it->length, name, length)) {
      return 0;
    }
  }

  Symbol *symbol = malloc(sizeof(Symbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }

  symbol->name = name;
  symbol->length = length;
  symbol->type = type;
  symbol->next = ctx->scope->symbols;
  ctx->scope->symbols = symbol;
  return 1;
}

const Symbol *sem_scope_find(const SemanticContext *ctx, const char *name, size_t length) {
  for (const Scope *scope = ctx->scope; scope; scope = scope->parent) {
    for (const Symbol *it = scope->symbols; it; it = it->next) {
      if (sem_names_equal(it->name, it->length, name, length)) {
        return it;
      }
    }
  }
  return NULL;
}

int32_t sem_scope_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  return sem_scope_find(ctx, name, length) != NULL;
}

static int32_t function_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  for (const FunctionSymbol *it = ctx->functions; it; it = it->next) {
    if (sem_names_equal(it->name, it->length, name, length)) {
      return 1;
    }
  }
  return 0;
}

void function_add(SemanticContext *ctx, const AstFunction *function) {
  if (sem_scope_find(ctx, function->name, function->length)) {
    semantic_error(ctx, function->body, "function '%s' conflicts with a global declaration", function->name);
    return;
  }

  if (function_lookup(ctx, function->name, function->length)) {
    semantic_error(ctx, function->body, "duplicate function '%s'", function->name);
    return;
  }

  FunctionSymbol *symbol = malloc(sizeof(FunctionSymbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }

  symbol->name = function->name;
  symbol->length = function->length;
  symbol->param_count = function->params.count;
  symbol->function = function;
  symbol->next = ctx->functions;
  ctx->functions = symbol;
}

const FunctionSymbol *function_find(const SemanticContext *ctx, const char *name, size_t length) {
  for (const FunctionSymbol *it = ctx->functions; it; it = it->next) {
    if (sem_names_equal(it->name, it->length, name, length)) {
      return it;
    }
  }
  return NULL;
}

const BuiltinFunction *builtin_find(const char *name, size_t length) {
  static const BuiltinFunction builtins[] = {
    {"__rars_syscall0", 1},
    {"__rars_syscall1", 2},
    {"__rars_syscall2", 3},
  };

  for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
    if (sem_names_equal(builtins[i].name, strlen(builtins[i].name), name, length)) {
      return &builtins[i];
    }
  }

  return NULL;
}

void functions_destroy(SemanticContext *ctx) {
  FunctionSymbol *symbol = ctx->functions;
  while (symbol) {
    FunctionSymbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }

  ctx->functions = NULL;
}
