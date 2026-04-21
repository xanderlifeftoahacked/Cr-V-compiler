#pragma once

#include <stdint.h>

#include "parser/ast.h"

int32_t semantic_analyze(const AstModule *module, const char *filename);

