#ifndef _DEBUG
#define _DEBUG


#ifdef OLD_DEBUG

	#ifdef DEBUG
		#include "wine/debug.h"
		WINE_DEFAULT_DEBUG_CHANNEL(asio);
	#else // DEBUG
		#define TRACE(...) {}
		#define WARN(fmt, ...) {} fprintf(stdout, fmt, ##__VA_ARGS__)
		#define ERR(fmt, ...) {} fprintf(stderr, fmt, ##__VA_ARGS__)
	#endif // DEBUG

#else // OLD_DEBUG

#endif // OLD_DEBUG
	#include <stdio.h>

	FILE* ensureLogFile();

#ifdef __i386__
	#define DEBUG_ARCH "32"
#else
	#define DEBUG_ARCH "64"
#endif

	#define TRACE(fmt, ...) {} fprintf(ensureLogFile(), DEBUG_ARCH   "TRACE %s:%i\t %s\t\t" fmt "\n",   __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(ensureLogFile())
	#define WARN(fmt, ...) {}  fprintf(ensureLogFile(), DEBUG_ARCH   "WARN  %s:%i\t %s\t\t" fmt "\n",   __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(ensureLogFile())
	#define ERR(fmt, ...) {}   fprintf(ensureLogFile(), DEBUG_ARCH "\nERROR %s:%i\t %s\t\t" fmt "\n\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(ensureLogFile())

#endif // _DEBUG