#pragma once

#include <stdint.h>
#include <stdio.h>

#include "parser/ast.h"

int32_t codegen_emit_module(FILE *out, const AstModule *module, const char *filename);

