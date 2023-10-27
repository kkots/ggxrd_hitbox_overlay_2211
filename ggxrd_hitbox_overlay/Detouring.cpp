#include "pch.h"
#include "Detouring.h"
#include "logging.h"
#include <detours.h>

Detouring detouring;

void Detouring::onDllDetach() {
	detachAll();
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
	if (!beginTransaction()) return;

	bool allSuccess = true;

	for (const ThingToBeUndetouredAtTheEnd& thing : thingsToUndetourAtTheEnd) {
		DWORD detourResult = DetourDetach(
			thing.ppPointer,
			thing.pDetour);

		if (detourResult != NO_ERROR) {
			printDetourDetachError(detourResult);
			if (thing.name) logwrap(fprintf(logfile, "Failed to undetour %s\n", thing.name));
			allSuccess = false;
			break;
		}
	}
	if (allSuccess) {
		logwrap(fputs("Successfully undetoured all of the hooks\n", logfile));
	}
	else {
		logwrap(fputs("Failed to undetour some or all of the hooks\n", logfile));
	}
	thingsToUndetourAtTheEnd.clear();

	if (!endTransaction()) return;
}

bool Detouring::beginTransaction() {
	DWORD detourResult = DetourTransactionBegin();
	if (detourResult != NO_ERROR) {
		printDetourTransactionBeginError(detourResult);
		return false;
	}

	detourResult = DetourUpdateThread(GetCurrentThread());
	if (detourResult != NO_ERROR) {
		printDetourUpdateThreadError(detourResult);
		return false;
	}
	return true;
}

bool Detouring::attach(PVOID* ppPointer, PVOID pDetour, const char * name) {
	DWORD detourResult = DetourAttach(
		ppPointer,
		pDetour);

	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return false;
	}
	if (name) logwrap(fprintf(logfile, "Successfully detoured %s\n", name));
	thingsToUndetourAtTheEnd.push_back({ ppPointer, pDetour, name });
	return true;
}

bool Detouring::endTransaction() {
	DWORD detourResult = DetourTransactionCommit();
	if (detourResult != NO_ERROR) {
		printDetourTransactionCommitError(detourResult);
		return false;
	}
	logwrap(fputs("Successfully committed detour/undetour transaction\n", logfile));
	return true;
}
