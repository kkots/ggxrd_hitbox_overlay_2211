#include "pch.h"
#include "memoryFunctions.h"
#include <Psapi.h>
#include "logging.h"
#include <cstdarg>

static int findChar(const char* str, char searchChar) {
	for (const char* c = str; *c != '\0'; ++c) {
		if (*c == searchChar) return c - str;
	}
	return -1;
}

bool getModuleBounds(const char* name, uintptr_t* start, uintptr_t* end)
{
	char moduleName[256] {0};
	char sectionName[16] {0};
	splitOutModuleName(name, moduleName, sectionName);
	if (sectionName[0] == '\0') {
		strncpy_s(sectionName, ".text", 5);
	}
	return getModuleBounds(moduleName, sectionName, start, end);
}

bool getModuleBounds(const char* name, const char* sectionName, uintptr_t* start, uintptr_t* end)
{
	const auto module = GetModuleHandleA(name);
	if (module == nullptr)
		return false;

	return getModuleBoundsHandle(module, sectionName, start, end);
}

bool getModuleBoundsHandle(HMODULE hModule, const char* sectionName, uintptr_t* start, uintptr_t* end)
{
	MODULEINFO info;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info))) return false;
	*start = (uintptr_t)(info.lpBaseOfDll);
	*end = *start + info.SizeOfImage;
	const uintptr_t peHeaderStart = *start + *(uintptr_t*)(*start + 0x3C);
	unsigned short numberOfSections = *(unsigned short*)(peHeaderStart + 0x6);
	const unsigned short optionalHeaderSize = *(unsigned short*)(peHeaderStart + 0x14);
	const uintptr_t optionalHeaderStart = peHeaderStart + 0x18;
	uintptr_t sectionStart = optionalHeaderStart + optionalHeaderSize;
	for (; numberOfSections != 0; --numberOfSections) {
		if (strncmp((const char*)(sectionStart), sectionName, 8) == 0) {
			*start = *start + *(unsigned int*)(sectionStart + 12);
			*end = *start + *(unsigned int*)(sectionStart + 8);
			break;
		}
		sectionStart += 40;
	}
	return true;
}

bool getModuleBoundsHandle(HMODULE hModule, uintptr_t* start, uintptr_t* end)
{
	return getModuleBoundsHandle(hModule, ".text", start, end);
}

// byteSpecification is of the format "00 8f 1e ??". ?? means unknown byte.
// Converts a "00 8f 1e ??" string into two vectors:
// sig vector will contain bytes '00 8f 1e' for the first 3 bytes and 00 for every ?? byte.
// sig vector will be terminated with an extra 0 byte.
// mask vector will contain an 'x' character for every non-?? byte and a '?' character for every ?? byte.
// mask vector will be terminated with an extra 0 byte.
void byteSpecificationToSigMask(const char* byteSpecification, std::vector<char>& sig, std::vector<char>& mask) {
	unsigned long long accumulatedNibbles = 0;
	int nibbleCount = 0;
	bool nibblesUnknown = false;
	const char* byteSpecificationPtr = byteSpecification;
	while (true) {
		char currentChar = *byteSpecificationPtr;
		if (currentChar != ' ' && currentChar != '\0') {
			char currentNibble = 0;
			if (currentChar >= '0' && currentChar <= '9' && !nibblesUnknown) {
				currentNibble = currentChar - '0';
			} else if (currentChar >= 'a' && currentChar <= 'f' && !nibblesUnknown) {
				currentNibble = currentChar - 'a' + 10;
			} else if (currentChar >= 'A' && currentChar <= 'F' && !nibblesUnknown) {
				currentNibble = currentChar - 'A' + 10;
			} else if (currentChar == '?' && (nibbleCount == 0 || nibblesUnknown)) {
				nibblesUnknown = true;
			} else {
				logwrap(fprintf(logfile, "Wrong byte specification: %s\n", byteSpecification));
				return;
			}
			accumulatedNibbles = (accumulatedNibbles << 4) | currentNibble;
			++nibbleCount;
			if (nibbleCount > 16) {
				logwrap(fprintf(logfile, "Wrong byte specification: %s\n", byteSpecification));
				return;
			}
		} else if (nibbleCount) {
			do {
				if (!nibblesUnknown) {
					sig.push_back(accumulatedNibbles & 0xff);
					mask.push_back('x');
					accumulatedNibbles >>= 8;
				} else {
					sig.push_back(0);
					mask.push_back('?');
				}
				nibbleCount -= 2;
			} while (nibbleCount > 0);
			nibbleCount = 0;
			nibblesUnknown = false;
			if (currentChar == '\0') {
				break;
			}
		}
		++byteSpecificationPtr;
	}
	sig.push_back('\0');
	mask.push_back('\0');
}

uintptr_t sigscan(const char* name, const char* sig, size_t sigLength)
{
	uintptr_t start, end;
	if (!getModuleBounds(name, &start, &end)) {
		logwrap(fputs("Module not loaded\n", logfile));
		return 0;
	}
	
	// Boyer-Moore-Horspool substring search
	// A table containing, for each symbol in the alphabet, the number of characters that can safely be skipped
	size_t step[256];
	for (int i = 0; i < _countof(step); ++i) {
		step[i] = sigLength;
	}
    for (size_t i = 0; i < sigLength - 1; i++) {
        step[(BYTE)sig[i]] = sigLength - 1 - i;
    }
	
	BYTE pNext;
    end -= sigLength;
    for (uintptr_t p = start; p <= end; p += step[pNext]) {
        int j = sigLength - 1;
    	pNext = *(BYTE*)(p + j);
    	if (sig[j] == (char)pNext) {
	        for (--j; j >= 0; --j) {
	            if (sig[j] != *(char*)(p + j)) {
	                break;
	            }
	        }
	        if (j < 0) {
	            return p;
	        }
    	}
    }

	logwrap(fputs("Sigscan failed\n", logfile));
	return 0;
}

void splitOutModuleName(const char* name, char* moduleName, char* sectionName) {
	bool foundColon = false;
	for (const char* c = name; *c != '\0'; ++c) {
		if (*c == ':') {
			foundColon = true;
		} else if (!foundColon) {
			*moduleName = *c;
			++moduleName;
		} else {
			*sectionName = *c;
			++sectionName;
		}
	}
}

uintptr_t sigscan(const char* name, const char* sig, const char* mask)
{
	uintptr_t start, end;
	if (!getModuleBounds(name, &start, &end)) {
		logwrap(fputs("Module not loaded\n", logfile));
		return 0;
	}

	const auto lastScan = end - strlen(mask) + 1;
	for (auto addr = start; addr < lastScan; addr++) {
		for (size_t i = 0;; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
				break;
		}
	}

	logwrap(fputs("Sigscan failed\n", logfile));
	return 0;
}

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, bool* error, const char* logname) {
	return sigscanOffsetMain(name, sig, sigLength, nullptr, {}, error, logname);
}

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, bool* error, const char* logname) {
	return sigscanOffsetMain(name, sig, 0, mask, {}, error, logname);
}

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, const std::vector<int>& offsets, bool* error, const char* logname) {
	return sigscanOffsetMain(name, sig, sigLength, nullptr, offsets, error, logname);
}

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, const std::vector<int>& offsets, bool* error, const char* logname) {
	return sigscanOffsetMain(name, sig, 0, mask, offsets, error, logname);
}

uintptr_t sigscanStrOffset(const char* name, const char* str, bool* error, const char* logname) {
	return sigscanOffsetMain(name, str, strlen(str), nullptr, {}, error, logname);
}

uintptr_t sigscanOffset(const char* name, const char* byteSpecification, bool* error, const char* logname) {
	std::vector<char> sig;
	std::vector<char> mask;
	byteSpecificationToSigMask(byteSpecification, sig, mask);
	return sigscanOffsetMain(name, sig.data(), 0, mask.data(), {}, error, logname);
}

uintptr_t sigscanStrOffset(const char* name, const char* str, const std::vector<int>& offsets, bool* error, const char* logname) {
	return sigscanOffsetMain(name, str, strlen(str), nullptr, offsets, error, logname);
}

uintptr_t sigscanOffset(const char* name, const char* byteSpecification, const std::vector<int>& offsets, bool* error, const char* logname) {
	std::vector<char> sig;
	std::vector<char> mask;
	byteSpecificationToSigMask(byteSpecification, sig, mask);
	return sigscanOffsetMain(name, sig.data(), 0, mask.data(), offsets, error, logname);
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
uintptr_t sigscanOffsetMain(const char* name, const char* sig, size_t sigLength, const char* mask, const std::vector<int>& offsets, bool* error, const char* logname) {
	uintptr_t sigscanResult;
	if (mask) {
		if (findChar(mask, '?') == -1) {
			sigLength = strlen(mask);
			sigscanResult = sigscan(name, sig, sigLength);
		} else {
			sigscanResult = sigscan(name, sig, mask);
		}
	} else {
		sigscanResult = sigscan(name, sig, sigLength);
	}
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

// relativeCallAddr points to the start of a relative call instruction.
// the relative call instruction is one byte of the instruction command, followed by 4 bytes relative offset. Offset can be negative (start with FF...).
// The relative offset is relative to the instruction that goes after the call smh, and the call instruction itself is 5 bytes
uintptr_t followRelativeCall(uintptr_t relativeCallAddr) {
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

char* findWildcard(char* mask, unsigned int indexOfWildcard) {
	// Every contiguous sequence of ? characters in mask is treated as a single "wildcard".
	// Every "wildcard" is assigned a number, starting from 0.
	// This function looks for a "wildcard" number 'numberOfWildcard'
	bool lastIsWildcard = false;
	unsigned int currentWildcardIndex = 0;
	bool isFirstWildcard = true;
	char* c = mask;
	for (; *c != '\0'; ++c) {
		const char cChar = *c;
		if (cChar == '?') {
			if (isFirstWildcard) {
				isFirstWildcard = false;
				lastIsWildcard = true;
				currentWildcardIndex = 0;
			}
			else if (!lastIsWildcard) {
				lastIsWildcard = true;
				++currentWildcardIndex;
			}
			if (currentWildcardIndex == indexOfWildcard) {
				return c;
			}
		}
		else {
			lastIsWildcard = false;
		}
	}
	return nullptr;
}

void substituteWildcard(char* sig, char* mask, unsigned int indexOfWildcard, void* ptrToSubstitute) {
	substituteWildcard(sig, mask, indexOfWildcard, (char*)&ptrToSubstitute, 4);
}

void substituteWildcard(char* sig, char* mask, unsigned int indexOfWildcard, char* sourceBuffer, size_t size) {
	// Every contiguous sequence of ? characters in mask is treated as a single "wildcard".
	// Every "wildcard" is assigned a number, starting from 0.
	// This function looks for a "wildcard" number 'numberOfWildcard' and replaces
	// the corresponding positions in sig with bytes from
	// the 'sourceBuffer' of size 'size'.
	// If the "wildcard" is smaller than 'size', the extra bytes outside the mask will be overwritten (in the sig) (watch out for buffer overflow).
	char* c = findWildcard(mask, indexOfWildcard);
	if (!c) return;

	char* maskPtr = c;
	char* sigPtr = sig + (c - mask);
	char* sourcePtr = sourceBuffer;
	for (size_t i = size; i != 0; --i) {
		*sigPtr = *sourcePtr;
		*maskPtr = 'x';
		++sigPtr;
		++sourcePtr;
		++maskPtr;
	}
}

char* scrollUpToInt3(char* ptr) {
	int limit = 0;
	while (true) {
		if (*(ptr - 1) == '\xCC' && *(ptr - 2) == '\xCC') break;
		// could use a full-on disassembler here
		--ptr;
		if (++limit > 10000) return nullptr;
	}
	return ptr;
}

char* scrollUpToBytes(char *ptr, const char* buf, int bufSize, int limit) {
	while (--limit >= 0) {
		bool match = true;
		const char* ptrPtr = ptr;
		const char* bufPtr = buf;
		for (int bufCounter = bufSize; bufCounter > 0; --bufCounter) {
			if (*bufPtr != *ptrPtr) {
				match = false;
				break;
			}
			++bufPtr;
			++ptrPtr;
		}
		if (match) return ptr;
		--ptr;
	}
	return nullptr;
}
