#include "utils/diagnostic.h"
#include "utils/attributes.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

struct {
  const char *filename;
  int32_t error_count;
  int32_t warning_count;
} diag_state = {NULL, 0, 0};

const char *level_to_string(DiagnosticLevel level) {
  switch (level) {
    case DIAG_LEVEL_INFO: return "[I]";
    case DIAG_LEVEL_WARN: return "[W]";
    case DIAG_LEVEL_ERROR: return "[E]";
    case DIAG_LEVEL_FATAL: return "[F]";
    default: return "[?]";
  }
}

const char *level_to_color(DiagnosticLevel level) {
  switch (level) {
    case DIAG_LEVEL_INFO: return "\033[36m";
    case DIAG_LEVEL_WARN: return "\033[33m";
    case DIAG_LEVEL_ERROR: return "\033[31m";
    case DIAG_LEVEL_FATAL: return "\033[1;31m";
    default: return "\033[0m";
  }
}

INLINE void diagnostic_init(const char *filename) {
  diag_state.filename = filename;
  diag_state.error_count = 0;
  diag_state.warning_count = 0;
}

void diagnostic_log(DiagnosticLevel level, SourceLocation loc,
                    const char *fmt, ...) {
  FILE *out = (level >= DIAG_LEVEL_ERROR) ? stderr : stdout;
  if (out == stderr) {
    fflush(stdout);
  } else {
    fflush(stderr);
  }
  const char *filename = loc.filename ? loc.filename : diag_state.filename;
  int32_t line = loc.line;

  if (level == DIAG_LEVEL_ERROR || level == DIAG_LEVEL_FATAL) {
    diag_state.error_count++;
  } else if (level == DIAG_LEVEL_WARN) {
    diag_state.warning_count++;
  }

  fprintf(out, "%s", level_to_color(level));

  if (filename) {
    fprintf(out, "[%s:%d] ", filename, line);
  }

  fprintf(out, "%s ", level_to_string(level));
  fprintf(out, "\033[0m");

  va_list args;
  va_start(args, fmt);
  vfprintf(out, fmt, args);
  va_end(args);

  fprintf(out, "\n");

  if (loc.source_line && loc.column > 0) {
    fprintf(out, "  %s\n", loc.source_line);
    fprintf(out, "  ");
    for (int32_t i = 1; i < loc.column; i++) {
      fprintf(out, " ");
    }
    fprintf(out, "\033[32m^\033[0m\n");
  }

  if (level == DIAG_LEVEL_FATAL) {
    fflush(out);
    exit(1);
  }

  fflush(out);
}

INLINE int32_t diagnostic_has_errors(void) {
  return diag_state.error_count > 0;
}

INLINE int32_t diagnostic_get_error_count(void) {
  return diag_state.error_count;
}

INLINE int32_t diagnostic_get_warning_count(void) {
  return diag_state.warning_count;
}

INLINE void diagnostic_reset(void) {
  diag_state.error_count = 0;
  diag_state.warning_count = 0;
}
