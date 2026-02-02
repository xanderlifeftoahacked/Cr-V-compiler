#include "parser/ast_printer.h"
#include "lexer/token.h"
#include <stdio.h>

static void print_indent(int depth) {
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
}

static const char *type_name(AstTypeKind kind) {
  switch (kind) {
    case AST_TYPE_INT: return "int";
    case AST_TYPE_CHAR: return "char";
    case AST_TYPE_ARRAY: return "array";
    default: return "?";
  }
}

static void print_type(const AstType *type) {
  if (!type) {
    printf("<no type>");
    return;
  }
  if (type->kind == AST_TYPE_ARRAY) {
    printf("%s[%d]", type_name(type->element_kind), type->array_size);
  } else {
    printf("%s", type_name(type->kind));
  }
}

static const char *op_string(TokenKind op) {
  const char *punct = token_punctuator_string(op);
  if (punct) {
    return punct;
  }
  return token_kind_name(op);
}

static void ast_print_node(const AstNode *node, int depth);

static void ast_print_block(const AstNode *node, int depth) {
  print_indent(depth);
  printf("block {\n");
  for (size_t i = 0; i < node->data.block.statements.count; i++) {
    ast_print_node(node->data.block.statements.items[i], depth + 1);
  }
  print_indent(depth);
  printf("}\n");
}

static void ast_print_function_header(const AstFunction *fn, int depth) {
  print_indent(depth);
  printf("fn %s : ", fn->name);
  print_type(&fn->return_type);
  if (fn->params.count > 0) {
    printf(" (");
    for (size_t i = 0; i < fn->params.count; i++) {
      if (i) printf(", ");
      print_type(&fn->params.items[i].type);
      printf(" %s", fn->params.items[i].name);
    }
    printf(")");
  }
  printf("\n");
}

static void ast_print_node(const AstNode *node, int depth) {
  if (!node) {
    print_indent(depth);
    printf("<null>\n");
    return;
  }
  switch (node->kind) {
    case AST_NODE_BLOCK:
      ast_print_block(node, depth);
      break;
    case AST_NODE_RETURN_STMT:
      print_indent(depth);
      printf("return\n");
      ast_print_node(node->data.return_stmt.expr, depth + 1);
      break;
    case AST_NODE_EXPR_STMT:
      print_indent(depth);
      printf("expr\n");
      ast_print_node(node->data.expr_stmt.expr, depth + 1);
      break;
    case AST_NODE_VAR_DECL:
      print_indent(depth);
      printf("var ");
      print_type(&node->data.var_decl.type);
      printf(" %s", node->data.var_decl.name);
      if (node->data.var_decl.initializer) {
        printf(" =\n");
        ast_print_node(node->data.var_decl.initializer, depth + 1);
      } else {
        printf("\n");
      }
      break;
    case AST_NODE_IF_STMT:
      print_indent(depth);
      printf("if\n");
      ast_print_node(node->data.if_stmt.condition, depth + 1);
      print_indent(depth);
      printf("then\n");
      ast_print_node(node->data.if_stmt.then_branch, depth + 1);
      if (node->data.if_stmt.else_branch) {
        print_indent(depth);
        printf("else\n");
        ast_print_node(node->data.if_stmt.else_branch, depth + 1);
      }
      break;
    case AST_NODE_WHILE_STMT:
      print_indent(depth);
      printf("while\n");
      ast_print_node(node->data.while_stmt.condition, depth + 1);
      ast_print_node(node->data.while_stmt.body, depth + 1);
      break;
    case AST_NODE_BREAK_STMT:
      print_indent(depth);
      printf("break\n");
      break;
    case AST_NODE_BINARY_EXPR:
      print_indent(depth);
      printf("binary %s\n", op_string(node->data.binary.op));
      ast_print_node(node->data.binary.left, depth + 1);
      ast_print_node(node->data.binary.right, depth + 1);
      break;
    case AST_NODE_UNARY_EXPR:
      print_indent(depth);
      printf("unary %s\n", op_string(node->data.unary.op));
      ast_print_node(node->data.unary.operand, depth + 1);
      break;
    case AST_NODE_INT_LITERAL:
      print_indent(depth);
      printf("int %d\n", node->data.int_literal.value);
      break;
    case AST_NODE_IDENTIFIER:
      print_indent(depth);
      printf("id %s\n", node->data.identifier.name);
      break;
    case AST_NODE_SUBSCRIPT_EXPR:
      print_indent(depth);
      printf("subscript\n");
      ast_print_node(node->data.subscript.base, depth + 1);
      ast_print_node(node->data.subscript.index, depth + 1);
      break;
    case AST_NODE_CALL_EXPR:
      print_indent(depth);
      printf("call\n");
      ast_print_node(node->data.call.callee, depth + 1);
      for (size_t i = 0; i < node->data.call.args.count; i++) {
        ast_print_node(node->data.call.args.items[i], depth + 1);
      }
      break;
    case AST_NODE_INIT_LIST:
      print_indent(depth);
      printf("init_list\n");
      for (size_t i = 0; i < node->data.init_list.elements.count; i++) {
        ast_print_node(node->data.init_list.elements.items[i], depth + 1);
      }
      break;
    default:
      print_indent(depth);
      printf("<unknown node %d>\n", node->kind);
      break;
  }
}

void ast_print_module(const AstModule *module) {
  if (!module) {
    printf("<no module>\n");
    return;
  }
  printf("module\n");
  for (size_t i = 0; i < module->functions.count; i++) {
    const AstFunction *fn = module->functions.items[i];
    ast_print_function_header(fn, 1);
    ast_print_node(fn->body, 2);
  }
}
