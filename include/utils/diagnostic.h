#pragma once

#include <stdint.h>

typedef enum {
  DIAG_LEVEL_INFO,
  DIAG_LEVEL_WARN,
  DIAG_LEVEL_WARNING = DIAG_LEVEL_WARN,
  DIAG_LEVEL_ERROR,
  DIAG_LEVEL_FATAL
} DiagnosticLevel;

typedef struct {
  const char *filename;
  int32_t line;
  int32_t column;
  const char *source_line;
} SourceLocation;

void diagnostic_init(const char *filename);

void diagnostic_log(DiagnosticLevel level, SourceLocation loc,
                    const char *fmt, ...);

#ifdef DEBUG
#define LOG(level, fmt, ...)                                                    \
    diagnostic_log(DIAG_LEVEL_##level,                                          \
                   (SourceLocation){__FILE__, __LINE__, 0, NULL},               \
                   (fmt), ##__VA_ARGS__)
#else
#define LOG(level, fmt, ...)                                                   \
    do {                                                                       \
        if (DIAG_LEVEL_##level != DIAG_LEVEL_INFO) {                           \
            diagnostic_log(DIAG_LEVEL_##level,                                 \
                           (SourceLocation){__FILE__, __LINE__, 0, NULL},      \
                           (fmt), ##__VA_ARGS__);                              \
        }                                                                      \
    } while (0)
#endif

int32_t diagnostic_has_errors(void);

int32_t diagnostic_get_error_count(void);

int32_t diagnostic_get_warning_count(void);

void diagnostic_reset(void);
