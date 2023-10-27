#include "pch.h"
#include "memoryFunctions.h"
#include <Psapi.h>
#include "logging.h"
#include <cstdarg>

bool getModuleBounds(const char* name, uintptr_t* start, uintptr_t* end)
{
	const auto module = GetModuleHandle(name);
	if (module == nullptr)
		return false;

	MODULEINFO info;
	GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info));
	*start = (uintptr_t)(info.lpBaseOfDll);
	*end = *start + info.SizeOfImage;
	return true;
}

uintptr_t sigscan(const char* name, const char* sig, const char* mask)
{
	uintptr_t start, end;
	if (!getModuleBounds(name, &start, &end)) {
		logwrap(fputs("Module not loaded\n", logfile));
		return 0;
	}

	if (mask) {
		const auto lastScan = end - strlen(mask) + 1;
		for (auto addr = start; addr < lastScan; addr++) {
			for (size_t i = 0;; i++) {
				if (mask[i] == '\0')
					return addr;
				if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
					break;
			}
		}
	} else {
		const auto lastScan = end - strlen(sig) + 1;
		for (auto addr = start; addr < lastScan; addr++) {
			for (size_t i = 0;; i++) {
				if (sig[i] == '\0')
					return addr;
				if (sig[i] != *(char*)(addr + i))
					break;
			}
		}
	}

	logwrap(fputs("Sigscan failed\n", logfile));
	return 0;
}

uintptr_t sigscanOffset(const char* name, const char* sig, bool* error, const char* logname) {
	return sigscanOffset(name, sig, nullptr, {}, error, logname);
}

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, bool* error, const char* logname) {
	return sigscanOffset(name, sig, mask, {}, error, logname);
}

uintptr_t sigscanOffset(const char* name, const char* sig, const std::vector<int>& offsets, bool* error, const char* logname) {
	return sigscanOffset(name, sig, nullptr, offsets, error, logname);
}

// Offsets work the following way:
// 1) Empty offsets: just return the sigscan result;
// 2) One offset: add number (positive or negative) to the sigscan result and return that;
// 3) Two offsets:
//    Let's say you're looking for code which mentions a function. You enter the code bytes as the sig to find the code.
//    Then you need to apply an offset from the start of the code to find the place where the function is mentioned.
//    Then you need to read 4 bytes of the function's address to get the function itself and return that.
//    (The second offset would be 0 for this, by the way.)
//    So here are the exact steps this function takes:
//    3.a) Find sigscan (the code in our example);
//    3.b) Add the first offset to sigscan (the offset of the function mention within the code);
//    3.c) Interpret this new position as the start of a 4-byte address which gets read, producing a new address.
//    3.d) The result is another address (the function address). Add the second offset to this address (0 in our example) and return that.
// 4) More than two offsets:
//    4.a) Find sigscan;
//    4.b) Add the first offset to sigscan;
//    4.c) Interpret this new position as the start of a 4-byte address which gets read, producing a new address.
//    4.d) The result is another address. Add the second offset to this address.
//    4.e) Repeat 4.c) and 4.d) for as many offsets as there are left. Return result on the last 4.d).
uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, const std::vector<int>& offsets, bool* error, const char* logname) {
	uintptr_t sigscanResult = sigscan(name, sig, mask);
	if (!sigscanResult) {
		if (error) *error = true;
		if (logname) {
			logwrap(fprintf(logfile, "Couldn't find %s\n", logname));
		}
		return 0;
	}
	if (logname) {
		logwrap(fprintf(logfile, "Found %s at %.8x\n", logname, sigscanResult));
	}
	uintptr_t addr = sigscanResult;
	bool isFirst = true;
	for (auto it = offsets.cbegin(); it != offsets.cend(); ++it) {
		if (!isFirst) {
			addr = *(uintptr_t*)addr;
		}
		addr += *it;
		isFirst = false;
	}
	if (!offsets.empty()) {
		if (logname) {
			logwrap(fprintf(logfile, "Final location of %s at %.8x\n", logname, addr));
		}
	}
	return addr;
}

uintptr_t followRelativeCall(uintptr_t relativeCallAddr) {
	// relativeCallAddr points to the start of a relative call instruction.
	// the relative call instruction is one byte of the instruction command, followed by 4 bytes relative offset. Offset can be negative (start with FF...).
	// The relative offset is relative to the instruction that goes after the call smh, and the call instruction itself is 5 bytes, so
	logwrap(fprintf(logfile, "Following relative call at %.8x, the call looks like ", relativeCallAddr));
	unsigned char* c = (unsigned char*)relativeCallAddr;
	for (int i = 5; i != 0; --i) {
		logwrap(fprintf(logfile, "%.2hhx ", *c));
		++c;
	}
	logwrap(fprintf(logfile, "and specifies offset %d\n", *(int*)(relativeCallAddr + 1)));
	return relativeCallAddr + 5 + *(int*)(relativeCallAddr + 1);
	// Calls can also have absolute addresses so check which one you got
}

int calculateRelativeCallOffset(uintptr_t relativeCallAddr, uintptr_t destinationAddr) {
	// relativeCallAddr points to the start of a relative call instruction.
	// destinationAddr points to the function that you would like to call.
	// This function calculates the offset necessary to put in the relative call instruction so that it
	// reaches the destinationAddr.
	// See followRelativeCall(...) for details of a relative call instruction.
	return (int)(destinationAddr - (relativeCallAddr + 5));
}

char* findWildcard(char* mask, unsigned int numberOfWildcard) {
	// Every contiguous sequence of ? characters in mask is treated as a single "wildcard".
	// Every "wildcard" is assigned a number, starting from 0.
	// This function looks for a "wildcard" number 'numberOfWildcard'
	bool lastIsWildcard = false;
	unsigned int currentWildcardNumber = 0;
	bool isFirstWildcard = true;
	char* c = mask;
	for (; *c != '\0'; ++c) {
		const char cChar = *c;
		if (cChar == '?') {
			if (isFirstWildcard) {
				isFirstWildcard = false;
				lastIsWildcard = true;
				currentWildcardNumber = 0;
			}
			else if (!lastIsWildcard) {
				lastIsWildcard = true;
				++currentWildcardNumber;
			}
			if (currentWildcardNumber == numberOfWildcard) {
				return c;
			}
		}
		else {
			lastIsWildcard = false;
		}
	}
	return nullptr;
}

void substituteWildcard(char* mask, char* sig, char* sourceBuffer, size_t size, unsigned int numberOfWildcard) {
	// Every contiguous sequence of ? characters in mask is treated as a single "wildcard".
	// Every "wildcard" is assigned a number, starting from 0.
	// This function looks for a "wildcard" number 'numberOfWildcard' and replaces
	// the corresponding positions in sig with bytes from
	// the 'sourceBuffer' of size 'size'.
	// If the "wildcard" is smaller than 'size', the extra bytes outside the mask will be overwritten (in the sig) (watch out for buffer overflow).
	char* c = findWildcard(mask, numberOfWildcard);
	if (!c) return;

	c = sig + (c - mask);
	char* s = sourceBuffer;
	for (size_t i = size; i != 0; --i) {
		*c = *s;
		++c;
		++s;
	}
}
