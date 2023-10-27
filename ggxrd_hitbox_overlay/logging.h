#pragma once

// Logging class for the dll
// Usage:
// define #define LOG_PATH L"C:\\your\\log\\path.txt"
// write to the log using:
// logwrap(fputs("string", logfile));  // logs every time
// log(fputs("string", logfile)); // logs only when didWriteOnce is false and has a limit of 1000 messages
// set didWriteOnce to false to stop log messages wrapped in log(...) from logging any further

#ifdef LOG_PATH
#include <iostream>
#include <mutex>
extern FILE* logfile;
extern std::mutex logfileMutex;
#define logwrap(things) \
{ \
	std::unique_lock<std::mutex> logfileGuard(logfileMutex); \
	errno_t err = _wfopen_s(&logfile, LOG_PATH, L"at+"); \
	if (err == 0 && logfile) { \
		things; \
		fclose(logfile); \
	} \
	logfile = NULL; \
}

extern bool didWriteOnce;
extern int msgLimit;
#define log(things) { \
	std::unique_lock<std::mutex> logfileGuard(logfileMutex); \
	if (msgLimit>=0 && !didWriteOnce) { \
		errno_t err = _wfopen_s(&logfile, LOG_PATH, L"at+"); \
		if (err == 0 && logfile) { \
			things; \
			fclose(logfile); \
		} \
		logfile = NULL; \
	} \
	msgLimit--; \
}
void logColor(unsigned int d3dColor);
#else
#define logwrap(things)
#define log(things)
#define logColor(things)
#endif
