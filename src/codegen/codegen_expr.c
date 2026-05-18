#include "codegen_internal.h"

static void emit_push_a0(CodegenContext *ctx) {
  emit_line(ctx, "  addi sp, sp, -4");
  emit_line(ctx, "  sw a0, 0(sp)");
}

static void emit_pop_t0(CodegenContext *ctx) {
  emit_line(ctx, "  lw t0, 0(sp)");
  emit_line(ctx, "  addi sp, sp, 4");
}

static void emit_load_address(CodegenContext *ctx, const CodegenSymbol *symbol) {
  if (symbol->storage == CODEGEN_STORAGE_GLOBAL) {
    emit_line(ctx, "  la a0, %.*s", (int32_t) symbol->length, symbol->name);
  } else {
    emit_line(ctx, "  addi a0, s0, %d", symbol->offset);
  }
}

static void emit_load_from_address(CodegenContext *ctx, AstType type) {
  if (type.kind == AST_TYPE_CHAR) {
    emit_line(ctx, "  lb a0, 0(a0)");
  } else {
    emit_line(ctx, "  lw a0, 0(a0)");
  }
}

static void emit_store_to_address(CodegenContext *ctx, AstType type) {
  if (type.kind == AST_TYPE_CHAR) {
    emit_line(ctx, "  sb a0, 0(t0)");
  } else {
    emit_line(ctx, "  sw a0, 0(t0)");
  }
}

static void emit_load_symbol(CodegenContext *ctx, const CodegenSymbol *symbol) {
  emit_load_address(ctx, symbol);
  if (symbol->type.kind == AST_TYPE_ARRAY) {
    return;
  }

  emit_load_from_address(ctx, symbol->type);
}

static int32_t is_scalar_type(AstType type) {
  return type.kind == AST_TYPE_INT || type.kind == AST_TYPE_CHAR;
}

static AstType pointer_type_to(AstType type) {
  AstType pointer = {
      .kind = AST_TYPE_POINTER,
      .element_kind = type.kind == AST_TYPE_ARRAY ? type.element_kind : type.kind,
      .array_size = 0,
  };
  return pointer;
}

static AstType expr_type(CodegenContext *ctx, const AstNode *node) {
  AstType invalid = {.kind = AST_TYPE_INT, .element_kind = AST_TYPE_INT, .array_size = 0};

  if (!node) {
    return invalid;
  }

  switch (node->kind) {
    case AST_NODE_IDENTIFIER: {
      CodegenSymbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
      return symbol ? symbol->type : invalid;
    }
    case AST_NODE_SUBSCRIPT_EXPR: {
      AstType base_type = expr_type(ctx, node->data.subscript.base);
      if (base_type.kind == AST_TYPE_ARRAY) {
        return array_element_type(base_type);
      }
      if (base_type.kind == AST_TYPE_POINTER) {
        return pointer_element_type(base_type);
      }
      return invalid;
    }
    case AST_NODE_UNARY_EXPR:
      if (node->data.unary.op == TOKEN_STAR) {
        AstType operand_type = expr_type(ctx, node->data.unary.operand);
        return operand_type.kind == AST_TYPE_POINTER ? pointer_element_type(operand_type) : invalid;
      }
      if (node->data.unary.op == TOKEN_AMPERSAND) {
        AstType operand_type = expr_type(ctx, node->data.unary.operand);
        return pointer_type_to(operand_type);
      }
      return invalid;
    case AST_NODE_BINARY_EXPR: {
      AstType left_type = expr_type(ctx, node->data.binary.left);
      AstType right_type = expr_type(ctx, node->data.binary.right);
      int32_t left_ptr = left_type.kind == AST_TYPE_POINTER;
      int32_t right_ptr = right_type.kind == AST_TYPE_POINTER;
      int32_t left_scalar = is_scalar_type(left_type);
      int32_t right_scalar = is_scalar_type(right_type);

      if (node->data.binary.op == TOKEN_PLUS) {
        if (left_ptr && right_scalar) {
          return left_type;
        }
        if (right_ptr && left_scalar) {
          return right_type;
        }
      } else if (node->data.binary.op == TOKEN_MINUS) {
        if (left_ptr && right_scalar) {
          return left_type;
        }
        if (left_ptr && right_ptr) {
          return invalid;
        }
      }
      return invalid;
    }
    default:
      return invalid;
  }
}

static AstType emit_lvalue_address(CodegenContext *ctx, const AstNode *node) {
  AstType invalid = {.kind = AST_TYPE_INT, .element_kind = AST_TYPE_INT, .array_size = 0};

  if (!node) {
    codegen_error(ctx, node, "expected assignable expression");
    return invalid;
  }

  if (node->kind == AST_NODE_IDENTIFIER) {
    CodegenSymbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
    if (!symbol) {
      codegen_error(ctx, node, "undeclared identifier '%s'", node->data.identifier.name);
      return invalid;
    }

    emit_load_address(ctx, symbol);
    return symbol->type;
  }

  if (node->kind == AST_NODE_UNARY_EXPR && node->data.unary.op == TOKEN_STAR) {
    AstType operand_type = expr_type(ctx, node->data.unary.operand);
    if (operand_type.kind != AST_TYPE_POINTER) {
      codegen_error(ctx, node, "cannot dereference a non-pointer expression");
      return invalid;
    }

    emit_expr(ctx, node->data.unary.operand);
    return pointer_element_type(operand_type);
  }

  if (node->kind != AST_NODE_SUBSCRIPT_EXPR) {
    codegen_error(ctx, node, "expected assignable expression");
    return invalid;
  }

  AstType base_type = expr_type(ctx, node->data.subscript.base);
  AstType element_type = invalid;
  if (base_type.kind == AST_TYPE_ARRAY) {
    element_type = array_element_type(base_type);
  } else if (base_type.kind == AST_TYPE_POINTER) {
    element_type = pointer_element_type(base_type);
  } else {
    codegen_error(ctx, node, "subscript base must be an array or pointer");
    return invalid;
  }

  emit_expr(ctx, node->data.subscript.base);
  emit_push_a0(ctx);

  emit_expr(ctx, node->data.subscript.index);
  if (ctx->had_error) {
    return invalid;
  }

  int32_t element_size = type_size(element_type);
  if (element_size > 1) {
    emit_line(ctx, "  li t0, %d", element_size);
    emit_line(ctx, "  mul a0, a0, t0");
  }

  emit_pop_t0(ctx);
  emit_line(ctx, "  add a0, t0, a0");
  return element_type;
}

static int32_t name_is(const char *name, size_t length, const char *expected, size_t expected_length) {
  return names_equal(name, length, expected, expected_length);
}

static int32_t emit_rars_syscall(CodegenContext *ctx, const AstNode *node, size_t value_arg_count) {
  size_t expected_arg_count = value_arg_count + 1;
  if (node->data.call.args.count != expected_arg_count) {
    codegen_error(ctx, node, "RARS syscall intrinsic expects %zu arguments", expected_arg_count);
    return 1;
  }

  if (value_arg_count == 0) {
    emit_expr(ctx, node->data.call.args.items[0]);
    if (ctx->had_error) {
      return 1;
    }
    emit_line(ctx, "  mv a7, a0");
    emit_line(ctx, "  ecall");
    return 0;
  }

  emit_expr(ctx, node->data.call.args.items[0]);
  if (ctx->had_error) {
    return 1;
  }
  emit_push_a0(ctx);

  emit_expr(ctx, node->data.call.args.items[1]);
  if (ctx->had_error) {
    return 1;
  }

  if (value_arg_count == 1) {
    emit_pop_t0(ctx);
    emit_line(ctx, "  mv a7, t0");
    emit_line(ctx, "  ecall");
    return 0;
  }

  emit_push_a0(ctx);
  emit_expr(ctx, node->data.call.args.items[2]);
  if (ctx->had_error) {
    return 1;
  }
  emit_line(ctx, "  mv a1, a0");
  emit_pop_t0(ctx);
  emit_line(ctx, "  mv a0, t0");
  emit_pop_t0(ctx);
  emit_line(ctx, "  mv a7, t0");
  emit_line(ctx, "  ecall");
  return 0;
}

static void emit_binary_stack(CodegenContext *ctx, TokenKind op) {
  emit_pop_t0(ctx);
  switch (op) {
    case TOKEN_PLUS:
      emit_line(ctx, "  add a0, t0, a0");
      return;
    case TOKEN_MINUS:
      emit_line(ctx, "  sub a0, t0, a0");
      return;
    case TOKEN_STAR:
      emit_line(ctx, "  mul a0, t0, a0");
      return;
    case TOKEN_DIV:
      emit_line(ctx, "  div a0, t0, a0");
      return;
    case TOKEN_MOD:
      emit_line(ctx, "  rem a0, t0, a0");
      return;
    case TOKEN_AMPERSAND:
      emit_line(ctx, "  and a0, t0, a0");
      return;
    case TOKEN_PIPE:
      emit_line(ctx, "  or a0, t0, a0");
      return;
    case TOKEN_CARET:
      emit_line(ctx, "  xor a0, t0, a0");
      return;
    case TOKEN_LSHIFT:
      emit_line(ctx, "  sll a0, t0, a0");
      return;
    case TOKEN_RSHIFT:
      emit_line(ctx, "  sra a0, t0, a0");
      return;
    case TOKEN_LESS:
      emit_line(ctx, "  slt a0, t0, a0");
      return;
    case TOKEN_GREATER:
      emit_line(ctx, "  slt a0, a0, t0");
      return;
    case TOKEN_EQUAL:
      emit_line(ctx, "  sub a0, t0, a0");
      emit_line(ctx, "  seqz a0, a0");
      return;
    case TOKEN_NOT_EQUAL:
      emit_line(ctx, "  sub a0, t0, a0");
      emit_line(ctx, "  snez a0, a0");
      return;
    case TOKEN_LESS_EQUAL:
      emit_line(ctx, "  slt a0, a0, t0");
      emit_line(ctx, "  seqz a0, a0");
      return;
    case TOKEN_GREATER_EQUAL:
      emit_line(ctx, "  slt a0, t0, a0");
      emit_line(ctx, "  seqz a0, a0");
      return;
    case TOKEN_LOGICAL_AND:
      emit_line(ctx, "  snez t0, t0");
      emit_line(ctx, "  snez a0, a0");
      emit_line(ctx, "  and a0, t0, a0");
      return;
    case TOKEN_LOGICAL_OR:
      emit_line(ctx, "  snez t0, t0");
      emit_line(ctx, "  snez a0, a0");
      emit_line(ctx, "  or a0, t0, a0");
      return;
    default:
      codegen_error(ctx, NULL, "unsupported binary operator");
      return;
  }
}

void emit_expr(CodegenContext *ctx, const AstNode *node) {
  if (!node || ctx->had_error) {
    return;
  }

  switch (node->kind) {
    case AST_NODE_INT_LITERAL:
      emit_line(ctx, "  li a0, %d", node->data.int_literal.value);
      return;
    case AST_NODE_IDENTIFIER: {
      CodegenSymbol *symbol = scope_find(ctx, node->data.identifier.name, node->data.identifier.length);
      if (!symbol) {
        codegen_error(ctx, node, "undeclared identifier '%s'", node->data.identifier.name);
        return;
      }
      emit_load_symbol(ctx, symbol);
      return;
    }
    case AST_NODE_UNARY_EXPR:
      if (node->data.unary.op == TOKEN_AMPERSAND) {
        AstType unused = emit_lvalue_address(ctx, node->data.unary.operand);
        (void) unused;
        return;
      }
      if (node->data.unary.op == TOKEN_STAR) {
        AstType type = emit_lvalue_address(ctx, node);
        if (ctx->had_error) {
          return;
        }
        emit_load_from_address(ctx, type);
        return;
      }
      emit_expr(ctx, node->data.unary.operand);
      if (ctx->had_error) {
        return;
      }
      if (is_array_value_expression(ctx, node->data.unary.operand)) {
        codegen_error(ctx, node, "array value cannot be used in unary expression");
        return;
      }
      switch (node->data.unary.op) {
        case TOKEN_PLUS:
          return;
        case TOKEN_MINUS:
          emit_line(ctx, "  sub a0, x0, a0");
          return;
        case TOKEN_EXCLAIM:
          emit_line(ctx, "  seqz a0, a0");
          return;
        case TOKEN_TILDE:
          emit_line(ctx, "  xori a0, a0, -1");
          return;
        default:
          codegen_error(ctx, node, "unsupported unary operator");
          return;
      }
    case AST_NODE_BINARY_EXPR:
      if (node->data.binary.op == TOKEN_ASSIGN) {
        AstType target_type = emit_lvalue_address(ctx, node->data.binary.left);
        if (ctx->had_error) {
          return;
        }
        if (target_type.kind == AST_TYPE_ARRAY) {
          codegen_error(ctx, node, "array value is not assignable");
          return;
        }

        emit_push_a0(ctx);
        emit_expr(ctx, node->data.binary.right);
        if (ctx->had_error) {
          return;
        }
        if (target_type.kind != AST_TYPE_POINTER && is_array_value_expression(ctx, node->data.binary.right)) {
          codegen_error(ctx, node, "array value cannot be assigned to a scalar expression");
          return;
        }
        emit_pop_t0(ctx);
        emit_store_to_address(ctx, target_type);
        return;
      }
      emit_expr(ctx, node->data.binary.left);
      if (ctx->had_error) {
        return;
      }
      emit_push_a0(ctx);
      emit_expr(ctx, node->data.binary.right);
      if (ctx->had_error) {
        return;
      }
      if (is_array_value_expression(ctx, node->data.binary.left) ||
          is_array_value_expression(ctx, node->data.binary.right)) {
        codegen_error(ctx, node, "array value cannot be used in scalar expression");
        return;
      }

      if (node->data.binary.op == TOKEN_PLUS || node->data.binary.op == TOKEN_MINUS) {
        AstType left_type = expr_type(ctx, node->data.binary.left);
        AstType right_type = expr_type(ctx, node->data.binary.right);
        int32_t left_ptr = left_type.kind == AST_TYPE_POINTER;
        int32_t right_ptr = right_type.kind == AST_TYPE_POINTER;
        int32_t left_scalar = is_scalar_type(left_type);
        int32_t right_scalar = is_scalar_type(right_type);

        if (left_ptr && right_scalar) {
          int32_t element_size = type_size(pointer_element_type(left_type));
          emit_pop_t0(ctx);
          if (element_size > 1) {
            emit_line(ctx, "  li t1, %d", element_size);
            emit_line(ctx, "  mul a0, a0, t1");
          }
          if (node->data.binary.op == TOKEN_PLUS) {
            emit_line(ctx, "  add a0, t0, a0");
          } else {
            emit_line(ctx, "  sub a0, t0, a0");
          }
          return;
        }

        if (right_ptr && left_scalar && node->data.binary.op == TOKEN_PLUS) {
          int32_t element_size = type_size(pointer_element_type(right_type));
          emit_pop_t0(ctx);
          if (element_size > 1) {
            emit_line(ctx, "  li t1, %d", element_size);
            emit_line(ctx, "  mul t0, t0, t1");
          }
          emit_line(ctx, "  add a0, a0, t0");
          return;
        }

        if (left_ptr && right_ptr && node->data.binary.op == TOKEN_MINUS) {
          int32_t element_size = type_size(pointer_element_type(left_type));
          emit_pop_t0(ctx);
          emit_line(ctx, "  sub a0, t0, a0");
          if (element_size > 1) {
            emit_line(ctx, "  li t1, %d", element_size);
            emit_line(ctx, "  div a0, a0, t1");
          }
          return;
        }

        if (left_ptr || right_ptr) {
          codegen_error(ctx, node, "unsupported pointer arithmetic");
          return;
        }
      }

      emit_binary_stack(ctx, node->data.binary.op);
      return;
    case AST_NODE_SUBSCRIPT_EXPR: {
      AstType element_type = emit_lvalue_address(ctx, node);
      if (ctx->had_error) {
        return;
      }
      emit_load_from_address(ctx, element_type);
      return;
    }
    case AST_NODE_CALL_EXPR: {
      if (node->data.call.callee->kind != AST_NODE_IDENTIFIER) {
        codegen_error(ctx, node, "call target must be a function identifier");
        return;
      }
      if (node->data.call.args.count > 8) {
        codegen_error(ctx, node, "functions with more than 8 arguments are not supported");
        return;
      }
      const char *name = node->data.call.callee->data.identifier.name;
      size_t length = node->data.call.callee->data.identifier.length;
      if (name_is(name, length, "__rars_syscall0", sizeof("__rars_syscall0") - 1)) {
        emit_rars_syscall(ctx, node, 0);
        return;
      }
      if (name_is(name, length, "__rars_syscall1", sizeof("__rars_syscall1") - 1)) {
        emit_rars_syscall(ctx, node, 1);
        return;
      }
      if (name_is(name, length, "__rars_syscall2", sizeof("__rars_syscall2") - 1)) {
        emit_rars_syscall(ctx, node, 2);
        return;
      }
      for (size_t i = 0; i < node->data.call.args.count; i++) {
        emit_expr(ctx, node->data.call.args.items[i]);
        if (ctx->had_error) {
          return;
        }
        emit_push_a0(ctx);
      }
      for (size_t i = node->data.call.args.count; i > 0; i--) {
        emit_pop_t0(ctx);
        emit_line(ctx, "  mv a%zu, t0", i - 1);
      }
      emit_line(ctx, "  call %.*s", (int32_t) length, name);
      return;
    }
    case AST_NODE_INIT_LIST:
      codegen_error(ctx, node, "initializer lists are not yet supported by the backend");
      return;
    default:
      codegen_error(ctx, node, "unsupported expression in backend");
      return;
  }
}
