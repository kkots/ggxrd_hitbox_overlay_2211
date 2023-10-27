#pragma once
#include <vector>

bool getModuleBounds(const char* name, uintptr_t* start, uintptr_t* end);

uintptr_t sigscan(const char* name, const char* sig, const char* mask = nullptr);

uintptr_t sigscanOffset(const char* name, const char* sig, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const char* sig, const std::vector<int>& offsets, bool* error, const char* logname);

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask = nullptr, const std::vector<int>& offsets = std::vector<int>{}, bool* error = nullptr, const char* logname = nullptr);

uintptr_t followRelativeCall(uintptr_t relativeCallAddr);

int calculateRelativeCallOffset(uintptr_t relativeCallAddr, uintptr_t destinationAddr);

char* findWildcard(char* mask, unsigned int numberOfWildcard = 0);

void substituteWildcard(char* mask, char* sig, char* sourceBuffer, size_t size, unsigned int numberOfWildcard = 0);
