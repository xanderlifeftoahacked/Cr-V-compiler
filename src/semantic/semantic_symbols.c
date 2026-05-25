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
  symbol->storage = AST_STORAGE_NONE;
  symbol->filename = NULL;
  symbol->is_definition = 1;
  symbol->next = ctx->scope->symbols;
  ctx->scope->symbols = symbol;
  return 1;
}

int32_t sem_scope_declare_global(SemanticContext *ctx, const AstNode *global) {
  if (!ctx->scope || !global || global->kind != AST_NODE_VAR_DECL) {
    return 0;
  }

  const char *name = global->data.var_decl.name;
  size_t length = global->data.var_decl.length;
  AstStorageClass storage = global->data.var_decl.storage;
  const char *filename = global->data.var_decl.filename;
  int32_t is_definition = storage != AST_STORAGE_EXTERN;

  for (Symbol *it = ctx->scope->symbols; it; it = it->next) {
    if (!sem_names_equal(it->name, it->length, name, length)) {
      continue;
    }

    int32_t same_file = it->filename && filename && strcmp(it->filename, filename) == 0;
    if (it->storage == AST_STORAGE_STATIC || storage == AST_STORAGE_STATIC) {
      if (same_file) {
        return 0;
      }
      continue;
    }

    if (it->is_definition && is_definition) {
      return 0;
    }
  }

  Symbol *symbol = malloc(sizeof(Symbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }

  symbol->name = name;
  symbol->length = length;
  symbol->type = global->data.var_decl.type;
  symbol->storage = storage;
  symbol->filename = filename;
  symbol->is_definition = is_definition;
  symbol->next = ctx->scope->symbols;
  ctx->scope->symbols = symbol;
  return 1;
}

const Symbol *sem_scope_find(const SemanticContext *ctx, const char *name, size_t length) {
  const Symbol *fallback = NULL;
  for (const Scope *scope = ctx->scope; scope; scope = scope->parent) {
    for (const Symbol *it = scope->symbols; it; it = it->next) {
      if (sem_names_equal(it->name, it->length, name, length)) {
        if (it->storage == AST_STORAGE_NONE && !it->filename) {
          return it;
        }
        if (it->storage == AST_STORAGE_STATIC) {
          if (it->filename && ctx->current_file && strcmp(it->filename, ctx->current_file) == 0) {
            return it;
          }
          continue;
        }
        if (!fallback) {
          fallback = it;
        }
      }
    }
  }
  return fallback;
}

int32_t sem_scope_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  return sem_scope_find(ctx, name, length) != NULL;
}

static int32_t same_filename(const char *lhs, const char *rhs) {
  return lhs && rhs && strcmp(lhs, rhs) == 0;
}

static int32_t global_conflicts_with_function(const Symbol *global, const AstFunction *function) {
  int32_t same_file = same_filename(global->filename, function->filename);
  if (global->storage == AST_STORAGE_STATIC || function->storage == AST_STORAGE_STATIC) {
    return same_file;
  }
  return 1;
}

void function_add(SemanticContext *ctx, const AstFunction *function) {
  if (ctx->scope) {
    for (const Symbol *global = ctx->scope->symbols; global; global = global->next) {
      if (sem_names_equal(global->name, global->length, function->name, function->length) &&
          global_conflicts_with_function(global, function)) {
        semantic_error(ctx, function->body, "function '%s' conflicts with a global declaration", function->name);
        return;
      }
    }
  }

  int32_t is_definition = function->body != NULL;
  for (FunctionSymbol *it = ctx->functions; it; it = it->next) {
    if (!sem_names_equal(it->name, it->length, function->name, function->length)) {
      continue;
    }

    int32_t same_file = it->filename && function->filename && strcmp(it->filename, function->filename) == 0;
    if (it->storage == AST_STORAGE_STATIC || function->storage == AST_STORAGE_STATIC) {
      if (!same_file) {
        continue;
      }
      if (it->storage != function->storage) {
        if (it->storage == AST_STORAGE_STATIC) {
          semantic_error(ctx, function->body, "non-static declaration of '%s' follows static declaration",
                         function->name);
        } else {
          semantic_error(ctx, function->body, "static declaration of '%s' follows non-static declaration",
                         function->name);
        }
        return;
      }
    }

    if (it->param_count != function->params.count) {
      semantic_error(ctx, function->body, "conflicting declaration for function '%s'", function->name);
      return;
    }
    if (it->is_definition && is_definition) {
      semantic_error(ctx, function->body, "duplicate function '%s'", function->name);
      return;
    }
  }

  FunctionSymbol *symbol = malloc(sizeof(FunctionSymbol));
  if (!symbol) {
    LOG(FATAL, "out of memory");
  }

  symbol->name = function->name;
  symbol->length = function->length;
  symbol->param_count = function->params.count;
  symbol->function = function;
  symbol->storage = function->storage;
  symbol->filename = function->filename;
  symbol->is_definition = is_definition;
  symbol->next = ctx->functions;
  ctx->functions = symbol;
}

const FunctionSymbol *function_find(const SemanticContext *ctx, const char *name, size_t length) {
  const FunctionSymbol *fallback = NULL;
  for (const FunctionSymbol *it = ctx->functions; it; it = it->next) {
    if (sem_names_equal(it->name, it->length, name, length)) {
      if (it->storage == AST_STORAGE_STATIC) {
        if (it->filename && ctx->current_file && strcmp(it->filename, ctx->current_file) == 0) {
          return it;
        }
        continue;
      }
      if (!fallback || (!fallback->is_definition && it->is_definition)) {
        fallback = it;
      }
    }
  }
  return fallback;
}

const BuiltinFunction *builtin_find(const char *name, size_t length) {
  static const BuiltinFunction builtins[] = {
    {"rars_print_int", 1},
    {"rars_print_string", 1},
    {"rars_read_int", 0},
    {"rars_read_string", 2},
    {"rars_sbrk", 1},
    {"rars_exit", 0},
    {"rars_print_char", 1},
    {"rars_read_char", 0},
    {"rars_get_cwd", 2},
    {"rars_time", 0},
    {"rars_time_low", 0},
    {"rars_time_high", 0},
    {"rars_midi_out", 4},
    {"rars_sleep_ms", 1},
    {"rars_midi_out_sync", 4},
    {"rars_print_int_hex", 1},
    {"rars_print_int_binary", 1},
    {"rars_print_uint", 1},
    {"rars_rand_seed", 2},
    {"rars_rand_int", 1},
    {"rars_rand_range", 2},
    {"rars_confirm_dialog", 1},
    {"rars_input_dialog_int", 1},
    {"rars_input_dialog_string", 3},
    {"rars_message_dialog", 2},
    {"rars_message_dialog_int", 2},
    {"rars_close", 1},
    {"rars_message_dialog_string", 2},
    {"rars_lseek", 3},
    {"rars_read", 3},
    {"rars_write", 3},
    {"rars_exit2", 1},
    {"rars_open", 2},
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
