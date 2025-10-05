#pragma once
#include <vector>

extern const char* GUILTY_GEAR_XRD_EXE;

// if namePtr is provided, it must point to an array that is at least 256 bytes in size!
// if sectionPtr is provided, it must point to an array that is at least 16 bytes in size!
bool getModuleBounds(const char* name, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin, char* namePtr = nullptr, char* sectionPtr = nullptr);
bool getModuleBounds(const char* name, const char* sectionName, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin);
bool getModuleBoundsHandle(HMODULE hModule, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin);
bool getModuleBoundsHandle(HMODULE hModule, const char* sectionName, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin);

// sigscan cache
/* When you sigscan something that has a logname, it gets saved into the sigscan cache file.
Later, when you repeat said sigscan, it is first checked with the file. The existing sigscan gets
validated using the maskForCaching and, if it passes the check, reused.
Because you can sometimes sigscan for addresses that may relocate, you can include relative addresses (to
the base of the module) in the byteSpecification.
byteSpecification specifies it as rel(?? ?? ?? ??) when it wants it to be relative to the game's exe module
byteSpecification specifies it as kernel32.dll(?? ?? ?? ??) when it wants it to be relative to kernel32.dll
We store it in maskForCaching as:
rel_GuiltyGearXrd.exe(????) for the first case
rel_kernel32.dll(????) for the second case

Other than that, maskForCaching gets encoded as
x for the actual byte value
? for byte that shouldn't be validated in the cache
r.... starts the parsing of a module name, followed by what can only equal (????) to denote that those
      4 bytes are an address relative to the base of that module

The name of the section, to which the search for confined, and maskForCaching are dumped directly
into the cache file, along with the actual sig and the mask.
Mask and section are supposed to never change, but for sig, we use the relative and ? indicators from
maskForCaching to check (equality check) or relative-check (relative to the base of some module; done
in groups of 4 bytes), or omit from the check (?) some bytes from the sig.

If maskForCaching is null, that means to just use the mask
*/

// byteSpecification is of the format "00 8f 1e ??". ?? means unknown byte.
// Converts a "00 8f 1e ??" string into two vectors:
// sig vector will contain bytes '00 8f 1e' for the first 3 bytes and 00 for every ?? byte.
// sig vector will be terminated with an extra 0 byte.
// mask vector will contain an 'x' character for every non-?? byte and a '?' character for every ?? byte.
// mask vector will be terminated with an extra 0 byte.
// Can additionally provide an size_t* position argument. If the byteSpecification contains a ">" character, position will store the offset of that byte.
// If multiple ">" characters are present, position must be an array able to hold all positions, and positionLength specifies the length of the array.
// If positionLength is 0, it is assumed the array is large enough to hold all > positions.
// Returns the number of > characters.
// We're also adding maskForCaching. It can output a mask that can be used to validate sigscan cache.
// How sigscan cache works is described in memoryFunctions.h at the top of the file
size_t byteSpecificationToSigMask(const char* byteSpecification, std::vector<char>& sig, std::vector<char>& mask, size_t* position = nullptr, size_t positionLength = 0, std::vector<char>* maskForCaching = nullptr);

void splitOutModuleName(const char* name, char* moduleName, size_t moduleNameBufSize, char* sectionName, size_t sectionNameBufSize);

uintptr_t sigscan(const char* name, const char* sig, size_t sigLength, const char* logname, const char* maskForCaching);

uintptr_t sigscan(const char* name, const char* sig, const char* mask, const char* logname, const char* maskForCaching);

uintptr_t sigscan(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength, const char* logname, const char* maskForCaching, const char* name = nullptr, const char* section = nullptr, uintptr_t wholeModuleBegin = 0);
uintptr_t sigscanFundamental(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength);

uintptr_t sigscan(uintptr_t start, uintptr_t end, const char* sig, const char* mask, const char* logname, const char* maskForCaching, const char* name = nullptr, const char* section = nullptr, uintptr_t wholeModuleBegin = 0);
uintptr_t sigscanFundamental(uintptr_t start, uintptr_t end, const char* sig, const char* mask);

uintptr_t sigscanBackwards(uintptr_t startBottom, uintptr_t endTop, const char* sig, const char* mask);

// for finding function starts
uintptr_t sigscanBackwards16ByteAligned(uintptr_t startBottom, uintptr_t endTop, const char* sig, const char* mask);

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, bool* error, const char* logname, const char* maskForCaching);

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, bool* error, const char* logname, const char* maskForCaching);

uintptr_t sigscanOffset(const char* name, const std::vector<char>& sig, const std::vector<char>& mask, bool* error, const char* logname, const char* maskForCaching);

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching);

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching);

uintptr_t sigscanOffset(const char* name, const std::vector<char>& sig, const std::vector<char>& mask, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching);

uintptr_t sigscanOffsetMain(const char* name, const char* sig, const size_t sigLength, const char* mask = nullptr, const std::initializer_list<int>& offsets = std::initializer_list<int>{}, bool* error = nullptr, const char* logname = nullptr, const char* maskForCaching = nullptr);



uintptr_t sigscanStrOffset(const char* name, const char* str, bool* error, const char* logname, const char* maskForCaching);

/// <param name="byteSpecification">Example: "80 f0 c7 ?? ?? ?? ?? e8". If contains only one > character, will return that specific byte instead</param>
uintptr_t sigscanOffset(const char* name, const char* byteSpecification, bool* error, const char* logname, size_t* position = nullptr);

uintptr_t sigscanStrOffset(const char* name, const char* str, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching);

/// <param name="byteSpecification">Example: "80 f0 c7 ?? ?? ?? ?? e8". If contains only one > character, will return that specific byte instead</param>
uintptr_t sigscanOffset(const char* name, const char* byteSpecification, const std::initializer_list<int>& offsets, bool* error, const char* logname, size_t* position = nullptr);

uintptr_t followRelativeCallNoLogs(uintptr_t relativeCallAddr);
uintptr_t followRelativeCall(uintptr_t callInstructionAddr);
uintptr_t followSinglyByteJump(uintptr_t jumpInstrAddr);

int calculateRelativeCallOffset(uintptr_t callInstructionAddr, uintptr_t destinationAddr);

char* findWildcard(char* mask, unsigned int indexOfWildcard = 0);

void substituteWildcard(std::vector<char>& sig, std::vector<char>& mask, unsigned int indexOfWildcard, void* ptrToSubstitute);

void substituteWildcard(char* sig, char* mask, unsigned int indexOfWildcard, char* sourceBuffer, size_t size);

void substituteWildcard(char* sig, char* mask, unsigned int indexOfWildcard, void* ptrToSubstitute);

char* scrollUpToInt3(char* ptr);

char* scrollUpToBytes(char* ptr, const char* buf, int bufSize, size_t searchLimit = 1000);

// If byteSpecification contains only one > character, returned uintptr_t will point to that byte if found and be 0 if the signature is not found
uintptr_t sigscanBackwards(uintptr_t, const char* byteSpecification, size_t searchLimit = 1000, size_t* position = nullptr);

// If byteSpecification contains only one > character, returned uintptr_t will point to that byte if found and be 0 if the signature is not found
uintptr_t sigscanBackwards16ByteAligned(uintptr_t, const char* byteSpecification, size_t searchLimit = 1000, size_t* position = nullptr);

// If byteSpecification contains only one > character, returned uintptr_t will point to that byte if found and be 0 if the signature is not found
uintptr_t sigscanForward(uintptr_t ptr, const char* byteSpecification, size_t searchLimit = 1000, size_t* position = nullptr);

uintptr_t sigscanForward(uintptr_t ptr, const char* sig, const char* mask, size_t searchLimit = 1000);

uintptr_t sigscanForward(uintptr_t ptr, const std::vector<char>& sig, const std::vector<char>& mask, size_t searchLimit = 1000);

uintptr_t findImportedFunction(const char* module, const char* dll, const char* function);

void sigscanCaseInsensitivePrepare(const char* sig, size_t sigLength, size_t* step);

uintptr_t sigscanCaseInsensitive(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength, size_t* step);

enum Register {
	REGISTER_EAX,
	REGISTER_ECX,
	REGISTER_EDX,
	REGISTER_EBX,
	REGISTER_ESP,
	REGISTER_EBP,
	REGISTER_ESI,
	REGISTER_EDI
};

// length of the instruction is always 2
bool isMovInstructionFromRegToReg(BYTE* ptr, Register* registerDst = nullptr, Register* registerSrc = nullptr);

// returns length of the instruction in bytes, if successful
int isTestInstructionRegImm(BYTE* ptr, Register* registerLeft = nullptr, DWORD* offset = nullptr, DWORD* imm = nullptr);

struct SigscanCacheEntry {
	// must include the null character
	std::vector<char> logname;
	// must include the null character
	std::vector<char> moduleName;
	// must include the null character
	std::vector<char> section;
	uintptr_t startRelativeToWholeModule;
	uintptr_t endRelativeToWholeModule;
	// must include the terminating null character
	// bytes that correspond to rel bytes in maskForCaching must have already been converted to relative offsets
	// and where mask is ?, sig must be \x00
	std::vector<char> sig;
	// must include the null character
	std::vector<char> mask;
	// must include the null character
	std::vector<char> maskForCaching;
	static inline void assignFromStr(std::vector<char>& vec, const char* src) {
		if (!src || *src == '\0') {
			vec.clear();
			return;
		}
		int len = strlen(src);
		vec.resize(len + 1);
		memcpy(vec.data(), src, len + 1);
	}
	static inline void assignFromStr(std::vector<char>& vec, const char* src, size_t len) {
		if (!len) {
			vec.clear();
			return;
		}
		vec.resize(len + 1);
		memcpy(vec.data(), src, len);
		vec.back() = '\0';
	}
	void parseSigFromHex(const char* start, const char* end);
	void applyMaskForCachingToSig();
	bool equal(const SigscanCacheEntry& other) const;
};

struct SigscanCacheValue {
	DWORD offset;
	int order;
};

void loadSigscanCache();

// used to signal when it is time to write sigscan cache results into the file
void finishedSigscanning();

extern const char* SIGSCAN_CACHE_FILE_NAME;
