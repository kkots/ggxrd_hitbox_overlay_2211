#include "pch.h"
#include "Detouring.h"
#include <detours.h>
#include <TlHelp32.h>
#include <algorithm>
#include "memoryFunctions.h"
#ifdef LOG_PATH
#include "WError.h"
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
		logwrap(fprintf(logfile, "Error in CreateToolhelp32Snapshot: %ls\n", winErr.getMessage()));
		#endif
		return false;
	}

	bool foundNotYetEnumeratedThread = false;
	if (!Thread32First(hSnapshot, &th32)) {
		#ifdef LOG_PATH
		WinError winErr;
		logwrap(fprintf(logfile, "Error in Thread32First: %ls\n", winErr.getMessage()));
		#endif
		CloseHandle(hSnapshot);
		return false;
	}
	while (true) {
		if (th32.th32OwnerProcessID == currentProcessId
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
				logwrap(fprintf(logfile, "Error in Thread32Next: %ls\n", winErr.getMessage()));
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

void Detouring::detachAll(bool freezeAll) {
	detachAllButThese(freezeAll);
}

// This is separate, because if we call VirtualProtect a second time to restore old protection, Detours won't be able to write to the program anymore.
void Detouring::undoPatches() {
	if (instructionsToReplaceWhenUnhooking.empty()) return;
	
	HANDLE thisProcess = GetCurrentProcess();
	
	for (InstructionToReplace& bytes : instructionsToReplaceWhenUnhooking) {
		// If Detours already changed this page's protection prior to us, oldProtect will hold
		// the value that Detours set, and then we will restore the page to that protection.
		// It will be Detours' job to put the original-original protection on the page
		DWORD oldProtect;
		VirtualProtect((void*)bytes.addr, bytes.origBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
		memcpy((void*)bytes.addr, bytes.origBytes.data(), bytes.origBytes.size());
		FlushInstructionCache(thisProcess, (void*)bytes.addr, bytes.origBytes.size());
		DWORD unused;
		VirtualProtect((void*)bytes.addr, bytes.origBytes.size(), oldProtect, &unused);
	}
	
}

void Detouring::detachOnlyThisHook(const char* name) {
	detachOnlyTheseHooks(&name, 1);
}

void Detouring::detachOnlyTheseHooks(const char** names, int namesCount) {

	logwrap(fputs("Detouring::detachOnlyTheseHooks(...) called\n", logfile));
	
	if (beginTransaction(false)) {
		undoPatches();
		
		bool allSuccess = true;

		auto it = thingsToUndetourAtTheEnd.begin();
		while (it != thingsToUndetourAtTheEnd.end()) {
			const ThingToBeUndetouredAtTheEnd& thing = *it;
			int i;
			for (i = 0; i < namesCount; ++i) {
				if (strcmp(thing.name, names[i]) == 0) {
					break;
				}
			}
			if (i == namesCount) {
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
		
		endTransaction();
		
		#ifdef LOG_PATH
		if (logfile) {
			std::unique_lock<std::mutex> logfileGuard(logfileMutex);
			fprintf(logfile, allSuccess ? "Successfully undetoured hooks " : "Failed to undetour one or more of hooks ");
			for (int i = 0; i < namesCount; ++i) {
				if (i == 0) {
					fprintf(logfile, "%s", names[0]);
				} else {
					fprintf(logfile, ", %s", names[i]);
				}
			}
			fputc("\n", logfile);
			fflush(logfile);
		}
		#endif
	}
}

void Detouring::detachAllButThese(bool freezeAll, const std::vector<PVOID>& dontDetachThese) {
	logwrap(fputs("Detouring::detachAllButThese(...) called\n", logfile));

	for (auto it = thingsToUndetourAtTheEnd.begin(); it != thingsToUndetourAtTheEnd.end(); ++it) {
		const ThingToBeUndetouredAtTheEnd& thing = *it;
		if (std::find(dontDetachThese.cbegin(), dontDetachThese.cend(), thing.pDetour) != dontDetachThese.cend()
				&& thing.name) {
			logwrap(fprintf(logfile, "Detouring::detachAllButThese(...): Will not unhook %s\n", thing.name));
		}
	}
	
	if (beginTransaction(freezeAll)) {
		undoPatches();
		
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
		
		endTransaction();
		
		if (allSuccess) {
			logwrap(fputs("Successfully undetoured all of the hooks\n", logfile));
		}
		else {
			logwrap(fputs("Failed to undetour some or all of the hooks\n", logfile));
		}
	}
}

bool Detouring::beginTransaction(bool freezeAllThreads) {
	DWORD detourResult = DetourTransactionBegin();
	if (detourResult != NO_ERROR) {
		printDetourTransactionBeginError(detourResult);
		return false;
	}
	beganTransaction = true;
	
	if (freezeAllThreads) {
		// Suspend all threads
		enumerateThreadsRecursively([&](DWORD threadId){
			HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, threadId);
			if (hThread == NULL || hThread == INVALID_HANDLE_VALUE) {
				#ifdef LOG_PATH
				WinError winErr;
				logwrap(fprintf(logfile, "Error in OpenThread: %ls\n", winErr.getMessage()));
				#endif
			}
			else {
				DWORD detourResult = DetourUpdateThread(hThread);  // oh god it still needs the thread handles by the time DetourTransactionCommit() is called
				if (detourResult != NO_ERROR) {
					printDetourUpdateThreadError(detourResult);
				}
				suspendedThreadHandles.push_back(hThread);
			}
		});
		logwrap(fprintf(logfile, "Suspended %u threads\n", suspendedThreadHandles.size()));
	}
	return true;
}

bool Detouring::attach(PVOID* ppPointer, PVOID pDetour, const char * name) {
	if (*ppPointer == NULL) return false;
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
	thingsToUndetourAtTheEnd.push_back({ ppPointer, pDetour, name });
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
	undoPatches();
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

void Detouring::addInstructionToReplaceWhenUnhooking(uintptr_t addr, const std::vector<char>& origBytes) {
	instructionsToReplaceWhenUnhooking.emplace_back();
	InstructionToReplace& elem = instructionsToReplaceWhenUnhooking.back();
	elem.addr = addr;
	elem.origBytes = origBytes;
}

bool Detouring::patchPlace(uintptr_t addr, const std::vector<char>& newBytes) {
	std::vector<char> origBytes;
	origBytes.resize(newBytes.size());
	memcpy(origBytes.data(), (void*)addr, newBytes.size());
	detouring.addInstructionToReplaceWhenUnhooking(addr, origBytes);
	return patchPlaceNoBackup(addr, newBytes);
}

bool Detouring::patchPlaceNoBackup(uintptr_t addr, const std::vector<char>& newBytes) {
	DWORD oldProtect;
	if (!VirtualProtect((void*)addr, newBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
	memcpy((void*)addr, newBytes.data(), newBytes.size());
	DWORD unused;
	if (!VirtualProtect((void*)addr, newBytes.size(), oldProtect, &unused)) return false;
	FlushInstructionCache(GetCurrentProcess(), (void*)addr, newBytes.size());  // don't care about failure
	return true;
}
