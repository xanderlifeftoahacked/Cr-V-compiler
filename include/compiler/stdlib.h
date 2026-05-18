#pragma once

#include <stdint.h>

#include "parser/ast.h"

#ifndef CRV_STDLIB_PATH
#define CRV_STDLIB_PATH "stdlib/rars.c"
#endif

const char *crv_default_stdlib_path(void);

int32_t crv_merge_modules(AstModule *out, const AstModule *stdlib_module, const AstModule *user_module);

void crv_free_merged_module(AstModule *module);
