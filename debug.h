#ifndef _DEBUG
#define _DEBUG

#ifdef OLD_DEBUG

#ifdef DEBUG
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(asio);
#else  // DEBUG
#define TRACE(...) \
  {}
#define WARN(fmt, ...) \
  {}                   \
  fprintf(stdout, fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) \
  {}                  \
  fprintf(stderr, fmt, ##__VA_ARGS__)
#endif  // DEBUG

#else  // OLD_DEBUG

#endif  // OLD_DEBUG
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

FILE* ensureLogFile();

#ifdef __i386__
#define DEBUG_ARCH "32"
#else
#define DEBUG_ARCH "64"
#endif

#define TRACE(fmt, ...)                                                     \
  {}                                                                        \
  fprintf(ensureLogFile(), DEBUG_ARCH " TRACE %lu %s:%i\t%s\t" fmt "\n",    \
          pthread_self(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
  fflush(ensureLogFile());                                                  \
  fsync(fileno(ensureLogFile()))
#define WARN(fmt, ...)                                                      \
  {}                                                                        \
  fprintf(ensureLogFile(), DEBUG_ARCH " WARN  %lu %s:%i\t%s\t" fmt "\n",    \
          pthread_self(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
  fflush(ensureLogFile());                                                  \
  fsync(fileno(ensureLogFile()))
#define ERR(fmt, ...)                                                          \
  {}                                                                           \
  fprintf(ensureLogFile(),                                                     \
          "\n" DEBUG_ARCH " ERROR %lu %s:%i\t%s\t" fmt "\n\n", pthread_self(), \
          __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);                    \
  fflush(ensureLogFile());                                                     \
  fsync(fileno(ensureLogFile()))

#endif  // _DEBUG