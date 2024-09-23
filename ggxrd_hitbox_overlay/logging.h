#pragma once

// Logging class for the dll
// Usage:
// define #define LOG_PATH L"C:\\your\\log\\path.txt". The path can be relative and that will be relative to the game executable's location (GuiltyGearXrd.exe)
// write to the log using:
// logwrap(fputs("string", logfile));  // logs every time
// logOnce(fputs("string", logfile)); // logs only when didWriteOnce is false and has a limit of 1000 messages
// set didWriteOnce to false to stop log messages wrapped in logOnce(...) from logging any further

#ifdef LOG_PATH
#include <iostream>
#include <mutex>
extern FILE* logfile;
extern std::mutex logfileMutex;
#define logwrap(things) \
{ \
	if (logfile) { \
		std::unique_lock<std::mutex> logfileGuard(logfileMutex); \
		things; \
		fflush(logfile); \
	} \
}

extern bool didWriteOnce;
extern int msgLimit;
#define logOnce(things) { \
	if (logfile) { \
		std::unique_lock<std::mutex> logfileGuard(logfileMutex); \
		if (msgLimit>=0 && !didWriteOnce) { \
			things; \
			fflush(logfile); \
		} \
		msgLimit--; \
	} \
}
void logColor(unsigned int d3dColor);
#else
#define logwrap(things)
#define logOnce(things)
#define logColor(things)
#endif
