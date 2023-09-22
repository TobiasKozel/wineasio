#ifndef _DEBUG
#define _DEBUG

#ifdef DEBUG
#include "wine/debug.h"
#else
#define TRACE(...) {}
#define WARN(fmt, ...) {} fprintf(stdout, fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) {} fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#ifdef DEBUG
WINE_DEFAULT_DEBUG_CHANNEL(asio);
#endif

#endif // _DEBUG