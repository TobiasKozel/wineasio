
#include "./debug.h"

FILE* logFile = NULL;

FILE* ensureLogFile() {
	if (logFile != NULL) {
		return logFile;
	}
	logFile = fopen("/tmp/wineasiopw.log", "a");
	return logFile;
}