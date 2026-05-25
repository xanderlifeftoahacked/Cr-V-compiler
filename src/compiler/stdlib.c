#include "compiler/stdlib.h"

#include <stdlib.h>

int32_t crv_merge_module_list(AstModule *out, const AstModule *const *modules, size_t module_count) {
  if (!out || (module_count > 0 && !modules)) {
    return 1;
  }

  out->globals.items = NULL;
  out->globals.count = 0;
  out->globals.capacity = 0;
  out->functions.items = NULL;
  out->functions.count = 0;
  out->functions.capacity = 0;

  size_t global_total = 0;
  size_t function_total = 0;
  for (size_t i = 0; i < module_count; i++) {
    if (!modules[i]) {
      continue;
    }
    global_total += modules[i]->globals.count;
    function_total += modules[i]->functions.count;
  }

  if (global_total > 0) {
    AstNode **globals = malloc(global_total * sizeof(AstNode *));
    if (!globals) {
      return 1;
    }

    size_t out_index = 0;
    for (size_t i = 0; i < module_count; i++) {
      if (!modules[i]) {
        continue;
      }
      for (size_t j = 0; j < modules[i]->globals.count; j++) {
        globals[out_index++] = modules[i]->globals.items[j];
      }
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

  size_t out_index = 0;
  for (size_t i = 0; i < module_count; i++) {
    if (!modules[i]) {
      continue;
    }
    for (size_t j = 0; j < modules[i]->functions.count; j++) {
      items[out_index++] = modules[i]->functions.items[j];
    }
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
