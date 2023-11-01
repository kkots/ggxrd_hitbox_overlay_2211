#include "pch.h"
#include "MutexWhichTellsWhatThreadItsLockedBy.h"

void MutexWhichTellsWhatThreadItsLockedBy::lock() {
	mutex.lock();
	threadId = GetCurrentThreadId();
	isLocked = true;
}

void MutexWhichTellsWhatThreadItsLockedBy::unlock() {
	mutex.unlock();
	threadId = 0;
	isLocked = false;
}

MutexWhichTellsWhatThreadItsLockedByGuard::MutexWhichTellsWhatThreadItsLockedByGuard(MutexWhichTellsWhatThreadItsLockedBy& mutex)
		: mutex(mutex) {
	mutex.lock();
	isLocked = true;
}

MutexWhichTellsWhatThreadItsLockedByGuard::~MutexWhichTellsWhatThreadItsLockedByGuard() {
	if (!isLocked) return;
	mutex.unlock();
	isLocked = false;
}

void MutexWhichTellsWhatThreadItsLockedByGuard::lock() {
	mutex.lock();
	isLocked = true;
}

void MutexWhichTellsWhatThreadItsLockedByGuard::unlock() {
	mutex.unlock();
	isLocked = false;
}
