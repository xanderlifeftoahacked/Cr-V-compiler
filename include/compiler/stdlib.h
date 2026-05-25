#pragma once

#include <stdint.h>
#include <stddef.h>

#include "parser/ast.h"

int32_t crv_merge_module_list(AstModule *out, const AstModule *const *modules, size_t module_count);

void crv_free_merged_module(AstModule *module);
