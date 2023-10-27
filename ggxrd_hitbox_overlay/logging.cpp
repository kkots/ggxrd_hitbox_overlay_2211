#include "pch.h"
#include "logging.h"

#ifdef LOG_PATH
FILE* logfile = NULL;
bool didWriteOnce = false;
int msgLimit = 1000;
std::mutex logfileMutex;
void logColor(unsigned int d3dColor) {
	log(fprintf(logfile, "{ Red: %hhu, Green: %hhu, Blue: %hhu, Alpha: %hhu }",
		(d3dColor >> 16) & 0xff, (d3dColor >> 8) & 0xff, d3dColor & 0xff, (d3dColor >> 24) & 0xff));
}
#endif
