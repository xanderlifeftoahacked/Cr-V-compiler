#include "semantic_internal.h"

#include <stdlib.h>

#include "utils/diagnostic.h"

void labels_destroy(SemanticContext *ctx) {
  LabelSymbol *symbol = ctx->labels;
  while (symbol) {
    LabelSymbol *next = symbol->next;
    free(symbol);
    symbol = next;
  }

  ctx->labels = NULL;
}

void gotos_destroy(SemanticContext *ctx) {
  GotoUse *use = ctx->gotos;
  while (use) {
    GotoUse *next = use->next;
    free(use);
    use = next;
  }

  ctx->gotos = NULL;
}

void switch_context_pop(SemanticContext *ctx) {
  if (!ctx->switch_stack) {
    return;
  }

  SwitchContext *current = ctx->switch_stack;
  IntValueNode *it = current->cases;

  while (it) {
    IntValueNode *next = it->next;
    free(it);
    it = next;
  }

  ctx->switch_stack = current->next;
  free(current);
}

void switch_context_push(SemanticContext *ctx) {
  SwitchContext *current = malloc(sizeof(SwitchContext));
  if (!current) {
    LOG(FATAL, "out of memory");
  }

  current->cases = NULL;
  current->has_default = 0;
  current->next = ctx->switch_stack;
  ctx->switch_stack = current;
}

int32_t switch_context_has_case(const SwitchContext *ctx, int32_t value) {
  for (const IntValueNode *it = ctx->cases; it; it = it->next) {
    if (it->value == value) {
      return 1;
    }
  }
  return 0;
}

void switch_context_add_case(SwitchContext *ctx, int32_t value) {
  IntValueNode *node = malloc(sizeof(IntValueNode));
  if (!node) {
    LOG(FATAL, "out of memory");
  }

  node->value = value;
  node->next = ctx->cases;
  ctx->cases = node;
}

static int32_t label_lookup(const SemanticContext *ctx, const char *name, size_t length) {
  for (const LabelSymbol *it = ctx->labels; it; it = it->next) {
    if (sem_names_equal(it->name, it->length, name, length)) {
      return 1;
    }
  }
  return 0;
}

void label_add(SemanticContext *ctx, const AstNode *node, const char *name, size_t length) {
  for (const LabelSymbol *it = ctx->labels; it; it = it->next) {
    if (sem_names_equal(it->name, it->length, name, length)) {
      semantic_error(ctx, node, "duplicate label '%s'", name);
      return;
    }
  }
  LabelSymbol *label = malloc(sizeof(LabelSymbol));
  if (!label) {
    LOG(FATAL, "out of memory");
  }

  label->name = name;
  label->length = length;
  label->node = node;
  label->next = ctx->labels;
  ctx->labels = label;
}

void goto_add(SemanticContext *ctx, const AstNode *node, const char *name, size_t length) {
  GotoUse *use = malloc(sizeof(GotoUse));
  if (!use) {
    LOG(FATAL, "out of memory");
  }

  use->name = name;
  use->length = length;
  use->node = node;
  use->next = ctx->gotos;
  ctx->gotos = use;
}

void validate_gotos(SemanticContext *ctx) {
  for (const GotoUse *use = ctx->gotos; use; use = use->next) {
    if (!label_lookup(ctx, use->name, use->length)) {
      semantic_error(ctx, use->node, "unknown label '%s'", use->name);
    }
  }
}
