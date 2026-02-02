#pragma once

#if defined(DEBUG)
#define INLINE
#else
#if defined(__GNUC__) || defined(__clang__)
#define INLINE inline __attribute__((always_inline))
#else
#define INLINE inline
#endif
#endif
