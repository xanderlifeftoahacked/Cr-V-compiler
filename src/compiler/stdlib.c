#include "compiler/stdlib.h"

#include <stdlib.h>

const char *crv_default_stdlib_path(void) {
  return CRV_STDLIB_PATH;
}

int32_t crv_merge_modules(AstModule *out, const AstModule *stdlib_module, const AstModule *user_module) {
  if (!out || !user_module) {
    return 1;
  }

  size_t stdlib_global_count = stdlib_module ? stdlib_module->globals.count : 0;
  size_t user_global_count = user_module->globals.count;
  size_t global_total = stdlib_global_count + user_global_count;

  size_t stdlib_function_count = stdlib_module ? stdlib_module->functions.count : 0;
  size_t user_function_count = user_module->functions.count;
  size_t function_total = stdlib_function_count + user_function_count;

  out->globals.items = NULL;
  out->globals.count = 0;
  out->globals.capacity = 0;
  out->functions.items = NULL;
  out->functions.count = 0;
  out->functions.capacity = 0;

  if (global_total > 0) {
    AstNode **globals = malloc(global_total * sizeof(AstNode *));
    if (!globals) {
      return 1;
    }

    for (size_t i = 0; i < stdlib_global_count; i++) {
      globals[i] = stdlib_module->globals.items[i];
    }
    for (size_t i = 0; i < user_global_count; i++) {
      globals[stdlib_global_count + i] = user_module->globals.items[i];
    }

    out->globals.items = globals;
    out->globals.count = global_total;
    out->globals.capacity = global_total;
  }

  if (function_total == 0) {
    return 0;
  }

  AstFunction **items = malloc(function_total * sizeof(AstFunction *));
  if (!items) {
    free(out->globals.items);
    out->globals.items = NULL;
    out->globals.count = 0;
    out->globals.capacity = 0;
    return 1;
  }

  for (size_t i = 0; i < stdlib_function_count; i++) {
    items[i] = stdlib_module->functions.items[i];
  }
  for (size_t i = 0; i < user_function_count; i++) {
    items[stdlib_function_count + i] = user_module->functions.items[i];
  }

  out->functions.items = items;
  out->functions.count = function_total;
  out->functions.capacity = function_total;
  return 0;
}

void crv_free_merged_module(AstModule *module) {
  if (!module) {
    return;
  }
  free(module->globals.items);
  module->globals.items = NULL;
  module->globals.count = 0;
  module->globals.capacity = 0;

  free(module->functions.items);
  module->functions.items = NULL;
  module->functions.count = 0;
  module->functions.capacity = 0;
}
