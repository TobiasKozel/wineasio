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

	#define TRACE(fmt, ...) {} fprintf(ensureLogFile(),   "TRACE %s:%i\t %s\t\t" fmt "\n",   __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
	#define WARN(fmt, ...) {}  fprintf(ensureLogFile(),   "WARN  %s:%i\t %s\t\t" fmt "\n",   __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
	#define ERR(fmt, ...) {}   fprintf(ensureLogFile(), "\nERROR %s:%i\t %s\t\t" fmt "\n\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#endif // _DEBUG