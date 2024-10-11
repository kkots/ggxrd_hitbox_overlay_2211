#pragma once
#include <vector>

bool getModuleBounds(const char* name, uintptr_t* start, uintptr_t* end);
bool getModuleBounds(const char* name, const char* sectionName, uintptr_t* start, uintptr_t* end);
bool getModuleBoundsHandle(HMODULE hModule, uintptr_t* start, uintptr_t* end);
bool getModuleBoundsHandle(HMODULE hModule, const char* sectionName, uintptr_t* start, uintptr_t* end);

// byteSpecification is of the format "00 8f 1e ??". ?? means unknown byte.
// Converts a "00 8f 1e ??" string into two vectors:
// sig vector will contain bytes '00 8f 1e' for the first 3 bytes and 00 for every ?? byte.
// sig vector will be terminated with an extra 0 byte.
// mask vector will contain an 'x' character for every non-?? byte and a '?' character for every ?? byte.
// mask vector will be terminated with an extra 0 byte.
void byteSpecificationToSigMask(const char* byteSpecification, std::vector<char>& sig, std::vector<char>& mask);

void splitOutModuleName(const char* name, char* moduleName, char* sectionName);

uintptr_t sigscan(const char* name, const char* sig, size_t sigLength);

uintptr_t sigscan(const char* name, const char* sig, const char* mask);

uintptr_t sigscan(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength);

uintptr_t sigscan(uintptr_t start, uintptr_t end, const char* sig, const char* mask);

uintptr_t sigscanBackwards(uintptr_t startBottom, uintptr_t endTop, const char* sig, const char* mask);

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const std::vector<char>& sig, const std::vector<char>& mask, bool* error, const char* logname);

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, const std::vector<int>& offsets, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, const std::vector<int>& offsets, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const std::vector<char>& sig, const std::vector<char>& mask, const std::vector<int>& offsets, bool* error, const char* logname);

uintptr_t sigscanOffsetMain(const char* name, const char* sig, const size_t sigLength, const char* mask = nullptr, const std::vector<int>& offsets = std::vector<int>{}, bool* error = nullptr, const char* logname = nullptr);



uintptr_t sigscanStrOffset(const char* name, const char* str, bool* error, const char* logname);

/// <param name="byteSpecification">Example: "80 f0 c7 ?? ?? ?? ?? e8"</param>
uintptr_t sigscanOffset(const char* name, const char* byteSpecification, bool* error, const char* logname);

uintptr_t sigscanStrOffset(const char* name, const char* str, const std::vector<int>& offsets, bool* error, const char* logname);

/// <param name="byteSpecification">Example: "80 f0 c7 ?? ?? ?? ?? e8"</param>
uintptr_t sigscanOffset(const char* name, const char* byteSpecification, const std::vector<int>& offsets, bool* error, const char* logname);


uintptr_t followRelativeCall(uintptr_t relativeCallAddr);

int calculateRelativeCallOffset(uintptr_t relativeCallAddr, uintptr_t destinationAddr);

char* findWildcard(char* mask, unsigned int indexOfWildcard = 0);

void substituteWildcard(std::vector<char>& sig, std::vector<char>& mask, unsigned int indexOfWildcard, void* ptrToSubstitute);

void substituteWildcard(char* sig, char* mask, unsigned int indexOfWildcard, char* sourceBuffer, size_t size);

void substituteWildcard(char* sig, char* mask, unsigned int indexOfWildcard, void* ptrToSubstitute);

char* scrollUpToInt3(char* ptr);

char* scrollUpToBytes(char* ptr, const char* buf, int bufSize, size_t searchLimit = 1000);

uintptr_t sigscanBackwards(uintptr_t, const char* byteSpecification, size_t searchLimit = 1000);

uintptr_t sigscanForward(uintptr_t ptr, const char* byteSpecification, size_t searchLimit = 1000);

uintptr_t sigscanForward(uintptr_t ptr, const char* sig, const char* mask, size_t searchLimit = 1000);

uintptr_t sigscanForward(uintptr_t ptr, const std::vector<char>& sig, const std::vector<char>& mask, size_t searchLimit = 1000);

uintptr_t findImportedFunction(const char* module, const char* dll, const char* function);
