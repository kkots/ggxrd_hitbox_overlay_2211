#pragma once
#include <mutex>

class MutexWhichTellsWhatThreadItsLockedBy
{
public:
	bool isLocked = false;
	unsigned long threadId = 0;
	std::mutex mutex;
	void lock();
	void unlock();
};

class MutexWhichTellsWhatThreadItsLockedByGuard
{
public:
	MutexWhichTellsWhatThreadItsLockedByGuard(MutexWhichTellsWhatThreadItsLockedBy& mutex);
	~MutexWhichTellsWhatThreadItsLockedByGuard();
	MutexWhichTellsWhatThreadItsLockedBy& mutex;
	bool isLocked = false;
	void lock();
	void unlock();
};
