#pragma once
#include <vector>
#include <mutex>
#include <functional>
#include <atomic>
#include "logging.h"

using suspendThreadCallback_t = std::function<void(DWORD threadId)>;

class Detouring
{
public:
	bool beginTransaction();

	// See comments and example inside struct ThingToBeUndetouredAtTheEnd
	bool attach(PVOID* ppPointer, PVOID pDetour, std::mutex* mutex, const char* name = nullptr);  // See ThingToBeUndetouredAtTheEnd for explanation of arguments
	bool endTransaction();
	bool cancelTransaction();
	void detachAll();
	void detachAllButThese(const std::vector<PVOID>& dontDetachThese = std::vector<PVOID>{});
	bool someThreadsAreExecutingThisModule(HMODULE hModule);
	void markHookRunning(std::string name, bool running);
	DWORD dllMainThreadId = 0;
	std::atomic_int hooksCounter{0};
private:
	struct ThingToBeUndetouredAtTheEnd {
		PVOID* ppPointer = nullptr;  // pointer to a pointer to the original function
		PVOID pDetour = nullptr;  // pointer to the new function - the hook function
		std::mutex* mutex = nullptr;  // a mutex through which you must read and call the pointer to the original function after attaching

		// You can't just call the original function. The sequence must be:
		// 1) Lock the same mutex as the one you provided in the 'mutex' element of 'ThingToBeUndetouredAtTheEnd' struct;
		// 2) Grab a pointer to the original function from the same place as the one referred to in ppPointer;
		// 3) Call the function and wait for it to return;
		// 4) Unlock the mutex;
		// 5) You can do whatever you want now.
		// 
		// 
		// Example:
		//
		// #include <mutex>
		// using orig_t = int(__cdecl*)(void);
		// orig_t orig = ...;  // find orig using some means
		// std::mutex origMutex;
		// 
		// void attach_my_hook() {
		//    ... detouring.beginTransaction() called in dllmain.cpp
		//    detouring.attach(&(PVOID&)orig, my_hook, &origMutex, "the_name_of_my_hook");
		//    ... detouring.endTransaction() called in dllmain.cpp
		// }
		// 
		// int my_hook() {
		//   std::unique_lock<std::mutex> guard(origMutex); // locks the mutex. Unlocks automatically upon destruction, i.e. when exiting the my_hook function
		//   return orig();
		// }
		const char* name = nullptr;  // the name of the hook for logging purposes only
	};
	std::vector<ThingToBeUndetouredAtTheEnd> thingsToUndetourAtTheEnd;
	void printDetourTransactionBeginError(LONG err);
	void printDetourUpdateThreadError(LONG err);
	void printDetourAttachError(LONG err);
	void printDetourDetachError(LONG err);
	void printDetourTransactionCommitError(LONG err);
	bool enumerateNotYetEnumeratedThreads(suspendThreadCallback_t callback);
	void enumerateThreadsRecursively(suspendThreadCallback_t callback);
	void closeAllThreadHandles();
	bool beganTransaction = false;
	std::vector<DWORD> suspendedThreads;
	std::vector<HANDLE> suspendedThreadHandles;

	#ifdef LOG_PATH
	std::mutex runningHooksMutex;
	std::vector<std::string> runningHooks;
	#endif
};

extern Detouring detouring;
