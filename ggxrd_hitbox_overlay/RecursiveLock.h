#pragma once
#include "pch.h"
#include <mutex>

class RecursiveGuard;

class RecursiveLock
{
private:
	std::mutex mutex;
	bool mutexIsLocked = false;
	DWORD mutexLockedBy = 0;
	friend class RecursiveGuard;
	bool lock() {
		DWORD threadId = GetCurrentThreadId();
		bool needUnlock = !(mutexIsLocked && mutexLockedBy == threadId);
		if (needUnlock) {
			mutex.lock();
			mutexIsLocked = true;
			mutexLockedBy = threadId;
		}
		return needUnlock;
	}
	void unlock() {
		mutex.unlock();
		mutexIsLocked = false;
	}
};

class RecursiveGuard {
public:
	RecursiveGuard(RecursiveLock& lock) : lock(lock) {
		needUnlock = lock.lock();
	}
	~RecursiveGuard() {
		if (needUnlock) lock.unlock();
	}
private:
	RecursiveLock& lock;
	bool needUnlock = false;
};
