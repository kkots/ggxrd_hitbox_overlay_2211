#include "pch.h"
#include "Detouring.h"
#include "logging.h"
#include <detours.h>
#include <TlHelp32.h>
#include <algorithm>
#include "memoryFunctions.h"
#ifdef LOG_PATH
#include "WinError.h"
#ifdef UNICODE
#define PRINTF_STRING_ARG "%ls"
#else
#define PRINTF_STRING_ARG "%s"
#endif
#endif

Detouring detouring;

void Detouring::enumerateThreadsRecursively(suspendThreadCallback_t callback) {
	while (enumerateNotYetEnumeratedThreads(callback));
}

bool Detouring::enumerateNotYetEnumeratedThreads(suspendThreadCallback_t callback) {
	DWORD currentThreadId = GetCurrentThreadId();
	DWORD currentProcessId = GetCurrentProcessId();
	THREADENTRY32 th32{0};
	th32.dwSize = sizeof(THREADENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE) {
		#ifdef LOG_PATH
		WinError winErr;
		logwrap(fprintf(logfile, "Error in CreateToolhelp32Snapshot: " PRINTF_STRING_ARG "\n", winErr.getMessage()));
		#endif
		return false;
	}

	bool foundNotYetEnumeratedThread = false;
	if (!Thread32First(hSnapshot, &th32)) {
		#ifdef LOG_PATH
		WinError winErr;
		logwrap(fprintf(logfile, "Error in Thread32First: " PRINTF_STRING_ARG "\n", winErr.getMessage()));
		#endif
		CloseHandle(hSnapshot);
		return false;
	}
	while (true) {
		if (th32.th32OwnerProcessID == currentProcessId
				&& th32.th32ThreadID != dllMainThreadId
				&& th32.th32ThreadID != currentThreadId) {
			if (std::find(suspendedThreads.begin(), suspendedThreads.end(), th32.th32ThreadID) == suspendedThreads.end()) {
				callback(th32.th32ThreadID);
				suspendedThreads.push_back(th32.th32ThreadID);
				foundNotYetEnumeratedThread = true;
			}
		}
		if (!Thread32Next(hSnapshot, &th32)) {
			#ifdef LOG_PATH
			WinError winErr;
			if (winErr.code != ERROR_NO_MORE_FILES) {
				logwrap(fprintf(logfile, "Error in Thread32Next: " PRINTF_STRING_ARG "\n", winErr.getMessage()));
			}
			#endif
			break;
		}
	}
	CloseHandle(hSnapshot);
	return foundNotYetEnumeratedThread;
}

void Detouring::printDetourTransactionBeginError(LONG err) {
	if (err == ERROR_INVALID_OPERATION) {
		logwrap(fputs("DetourTransactionBegin: ERROR_INVALID_OPERATION: A pending transaction already exists.\n", logfile));
	}
	else if (err != NO_ERROR) {
		logwrap(fprintf(logfile, "DetourTransactionBegin: %d\n", err));
	}
}

void Detouring::printDetourUpdateThreadError(LONG err) {
	if (err == ERROR_NOT_ENOUGH_MEMORY) {
		logwrap(fputs("DetourUpdateThread: ERROR_NOT_ENOUGH_MEMORY: Not enough memory to record identity of thread.\n", logfile));
	}
	else if (err != NO_ERROR) {
		logwrap(fprintf(logfile, "DetourUpdateThread: %d\n", err));
	}
}

void Detouring::printDetourDetachError(LONG err) {
	switch (err) {
	case ERROR_INVALID_BLOCK: logwrap(fputs("ERROR_INVALID_BLOCK :  The function to be detached was too small to be detoured.\n", logfile)); break;
	case ERROR_INVALID_HANDLE: logwrap(fputs("ERROR_INVALID_HANDLE : The ppPointer parameter is NULL or references a NULL address.\n", logfile)); break;
	case ERROR_INVALID_OPERATION: logwrap(fputs("ERROR_INVALID_OPERATION : No pending transaction exists.\n", logfile)); break;
	case ERROR_NOT_ENOUGH_MEMORY: logwrap(fputs("ERROR_NOT_ENOUGH_MEMORY : Not enough memory exists to complete the operation.\n", logfile)); break;
	default: {
		if (err != NO_ERROR) {
			logwrap(fprintf(logfile, "DetourDetach: %d\n", err));
		}
	}
	}
}

void Detouring::printDetourAttachError(LONG err) {
	switch (err) {
	case ERROR_INVALID_BLOCK: logwrap(fputs("ERROR_INVALID_BLOCK : The function referenced is too small to be detoured.\n", logfile)); break;
	case ERROR_INVALID_HANDLE: logwrap(fputs("ERROR_INVALID_HANDLE : The ppPointer parameter is NULL or points to a NULL pointer.\n", logfile)); break;
	case ERROR_INVALID_OPERATION: logwrap(fputs("ERROR_INVALID_OPERATION : No pending transaction exists.\n", logfile)); break;
	case ERROR_NOT_ENOUGH_MEMORY: logwrap(fputs("ERROR_NOT_ENOUGH_MEMORY : Not enough memory exists to complete the operation.\n", logfile)); break;
	default: {
		if (err != NO_ERROR) {
			logwrap(fprintf(logfile, "DetourAttach: %d\n", err));
		}
	}
	}
}

void Detouring::printDetourTransactionCommitError(LONG err) {
	if (err == ERROR_INVALID_DATA) {
		logwrap(fputs("DetourTransactionCommit: ERROR_INVALID_DATA: Target function was changed by third party between steps of the transaction.\n", logfile));
	}
	else if (err == ERROR_INVALID_OPERATION) {
		logwrap(fputs("DetourTransactionCommit: ERROR_INVALID_OPERATION: No pending transaction exists..\n", logfile));
	}
	else if (err != NO_ERROR) {
		logwrap(fprintf(logfile, "DetourTransactionCommit: %d\n", err));
	}
}

void Detouring::detachAll() {
	detachAllButThese();
}

void Detouring::detachAllButThese(const std::vector<PVOID>& dontDetachThese) {
	logwrap(fputs("Detouring::detachAllButThese(...) called\n", logfile));
	std::vector<MutexWhichTellsWhatThreadItsLockedBy*> lockedMutexes;
	
	DWORD currentThreadId = GetCurrentThreadId();

	for (auto it = thingsToUndetourAtTheEnd.begin(); it != thingsToUndetourAtTheEnd.end(); ++it) {
		const ThingToBeUndetouredAtTheEnd& thing = *it;
		if (std::find(dontDetachThese.cbegin(), dontDetachThese.cend(), thing.pDetour) != dontDetachThese.cend()) {
			if (thing.name) {
				logwrap(fprintf(logfile, "Detouring::detachAllButThese(...): Will not unhook %s\n", thing.name));
			}
			continue;
		}

		if (thing.mutex->isLocked && thing.mutex->threadId == currentThreadId) {
			if (thing.name) {
				logwrap(fprintf(logfile, "Detouring::detachAllButThese(...): mutex for %s is already owned by this thread, so it won't be relocked\n", thing.name));
			}
			continue;
		}

		if (thing.name) {
			logwrap(fprintf(logfile, "Detouring::detachAllButThese(...): locking mutex for %s\n", thing.name));
		}
		it->mutex->lock();
		lockedMutexes.push_back(it->mutex);
	}
	logwrap(fputs("Detouring::detachAllButThese(...): locked all needed mutexes\n", logfile));

	if (beginTransaction()) {
		bool allSuccess = true;

		auto it = thingsToUndetourAtTheEnd.begin();
		while (it != thingsToUndetourAtTheEnd.end()) {
			const ThingToBeUndetouredAtTheEnd& thing = *it;
			if (std::find(dontDetachThese.cbegin(), dontDetachThese.cend(), thing.pDetour) != dontDetachThese.cend()) {
				++it;
				continue;
			}

			DWORD detourResult = DetourDetach(
				thing.ppPointer,
				thing.pDetour);

			if (detourResult != NO_ERROR) {
				printDetourDetachError(detourResult);
				if (thing.name) {
					logwrap(fprintf(logfile, "Failed to undetour %s\n", thing.name));
				}
				allSuccess = false;
				break;
			}
			it = thingsToUndetourAtTheEnd.erase(it);
		}
		if (allSuccess) {
			logwrap(fputs("Successfully undetoured all of the hooks\n", logfile));
		}
		else {
			logwrap(fputs("Failed to undetour some or all of the hooks\n", logfile));
		}

		endTransaction();
	}

	for (MutexWhichTellsWhatThreadItsLockedBy* mutexPtr : lockedMutexes) {
		mutexPtr->unlock();
	}
}

bool Detouring::beginTransaction() {
	DWORD detourResult = DetourTransactionBegin();
	if (detourResult != NO_ERROR) {
		printDetourTransactionBeginError(detourResult);
		return false;
	}
	beganTransaction = true;

	// Suspend all threads
	enumerateThreadsRecursively([&](DWORD threadId){
		HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
		if (hThread == NULL || hThread == INVALID_HANDLE_VALUE) {
			#ifdef LOG_PATH
			WinError winErr;
			logwrap(fprintf(logfile, "Error in OpenThread: " PRINTF_STRING_ARG "\n", winErr.getMessage()));
			#endif
		}
		else {
			logwrap(fprintf(logfile, "Suspending thread ID %d\n", threadId));
			DWORD detourResult = DetourUpdateThread(hThread);  // oh god it still needs the thread handles by the time DetourTransactionCommit() is called
			if (detourResult != NO_ERROR) {
				printDetourUpdateThreadError(detourResult);
			}
			suspendedThreadHandles.push_back(hThread);
		}
	});
	return true;
}

bool Detouring::attach(PVOID* ppPointer, PVOID pDetour, MutexWhichTellsWhatThreadItsLockedBy* mutex, const char * name) {
	if (*ppPointer == NULL || mutex == nullptr) return false;
	DWORD detourResult = DetourAttach(
		ppPointer,
		pDetour);

	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return false;
	}
	if (name) {
		logwrap(fprintf(logfile, "Successfully detoured %s\n", name));
	}
	thingsToUndetourAtTheEnd.push_back({ ppPointer, pDetour, mutex, name });
	return true;
}

bool Detouring::endTransaction() {
	bool result = false;
	DWORD detourResult = DetourTransactionCommit();
	if (detourResult != NO_ERROR) {
		printDetourTransactionCommitError(detourResult);
	} else {
		beganTransaction = false;
		result = true;
		logwrap(fputs("Successfully committed detour/undetour transaction\n", logfile));
	}
	closeAllThreadHandles();
	return result;
}

bool Detouring::cancelTransaction() {
	if (!beganTransaction) return true;
	DetourTransactionAbort();
	closeAllThreadHandles();
	beganTransaction = false;
	return true;
}

void Detouring::closeAllThreadHandles() {
	for (HANDLE hThread : suspendedThreadHandles) {
		CloseHandle(hThread);
	}
	suspendedThreadHandles.clear();
	suspendedThreads.clear();
}

bool Detouring::someThreadsAreExecutingThisModule() {
	
	uintptr_t dllStart;
	uintptr_t dllEnd;
	if (!getModuleBounds(DLL_NAME, &dllStart, &dllEnd)) return false;

	bool threadEipInThisModule = false;

	// Suspend all threads
	enumerateThreadsRecursively([&](DWORD threadId) {
		if (threadEipInThisModule) return;
		HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
		if (hThread == NULL || hThread == INVALID_HANDLE_VALUE) {
#ifdef LOG_PATH
			WinError winErr;
			logwrap(fprintf(logfile, "Error in OpenThread: " PRINTF_STRING_ARG "\n", winErr.getMessage()));
#endif
		}
		else {
			logwrap(fprintf(logfile, "Suspending thread ID %d\n", threadId));
			SuspendThread(hThread);
			suspendedThreadHandles.push_back(hThread);

			CONTEXT ctx{0};
			ctx.ContextFlags = CONTEXT_CONTROL;
			GetThreadContext(hThread, &ctx);

			if (ctx.Eip >= dllStart && ctx.Eip < dllEnd) threadEipInThisModule = true;
		}
	});

	threadEipInThisModule = threadEipInThisModule || hooksCounter > 0;  // some hooks may call functions that lead outside the module

	for (HANDLE hThread : suspendedThreadHandles) {
		ResumeThread(hThread);
	}
	closeAllThreadHandles();
	return threadEipInThisModule;
}
