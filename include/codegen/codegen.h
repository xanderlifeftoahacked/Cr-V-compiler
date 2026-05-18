#pragma once

#include "parser/ast.h"

#include <stdint.h>
#include <stdio.h>

int32_t codegen_emit_module(FILE *out, const AstModule *module, const char *filename);
