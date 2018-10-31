
#ifndef _BASICS_H
#define _BASICS_H

#include <cstdint>

#if defined(__APPLE__) || defined(_WIN32)
#define MAXPATHLEN 256
#else
#include <sys/param.h> // MAXPATHLEN
#endif

#ifndef PATH_MAX
#define PATH_MAX 259
#endif

void stat(const char *fmt, ...);
void staterr(const char *fmt, ...);
#define ASSERT(X)                                                                                                      \
  {                                                                                                                    \
    if (!(X))                                                                                                          \
    {                                                                                                                  \
      staterr("** ASSERT FAILED: '%s' at %s(%d)", #X, __FILE__, __LINE__);                                             \
      exit(1);                                                                                                         \
    }                                                                                                                  \
  }

#define SWAP(A, B)                                                                                                     \
  {                                                                                                                    \
    A ^= B;                                                                                                            \
    B ^= A;                                                                                                            \
    A ^= B;                                                                                                            \
  }

#ifndef MIN
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

#ifndef MAX
#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#endif

#endif
