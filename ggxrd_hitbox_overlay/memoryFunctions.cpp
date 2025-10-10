#include "pch.h"
#include "memoryFunctions.h"
#include <Psapi.h>
#include "logging.h"
#include <cstdarg>
#include <unordered_map>
#include "WError.h"
#include "LineReaderFromString.h"
#include "Settings.h"

std::vector<HMODULE> allModules;

const char* SIGSCAN_CACHE_FILE_NAME = "ggxrd_hitbox_overlay_sigscanCache.txt";

int sigscanOrder = 0;
bool sigscanCacheLoaded = false;
bool sigscansChanged = false;

static DWORD hashStringCaseInsensitive(const char* str) {
	DWORD hash = 0;
	for (const char* c = str; *c != '\0'; ++c) {
		char cVal = *c;
		if (cVal >= 'A' && cVal <= 'Z') cVal = 'a' + cVal - 'A';
		hash = hash * 0x89 + cVal;
	}
	return hash;
}

static DWORD hashString(const char* str) {
	DWORD hash = 0;
	for (const char* c = str; *c != '\0'; ++c) {
		hash = hash * 0x89 + *c;
	}
	return hash;
}

struct MyHashFunction {
	inline std::size_t operator()(const SigscanCacheEntry& k) const {
		return k.logname.empty() ? 0 : hashString(k.logname.data());
	}
};
struct MyCompareFunction {
	inline bool operator()(const SigscanCacheEntry& k, const SigscanCacheEntry& other) const {
		return (k.logname.empty() ? other.logname.empty() : strcmp(k.logname.data(), other.logname.data()) == 0);
	}
};

using sigscanCacheType = std::unordered_map<SigscanCacheEntry, SigscanCacheValue, MyHashFunction, MyCompareFunction>;
sigscanCacheType sigscanCache;

void loadSigscanCache() {
	#define fileParsingErr(msg, ...) { logwrap(fprintf(logfile, "%s parsing error: " msg, SIGSCAN_CACHE_FILE_NAME, __VA_ARGS__)); return; }
	if (sigscanCacheLoaded) return;
	sigscanCacheLoaded = true;  // we won't try again if we fail X)
	HANDLE file = CreateFileA(SIGSCAN_CACHE_FILE_NAME, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		WinError winErr;
		fileParsingErr("Could not open file: %ls\n", winErr.getMessage());
		return;
	}
	struct FileCloser {
		~FileCloser() {
			if (file) CloseHandle(file);
		}
		HANDLE file;
	} fileCloser {
		file
	};
	DWORD fileSize = GetFileSize(file, NULL);
	if (!fileSize) fileParsingErr("File empty.")
	std::vector<char> data(fileSize + 1);
	char* dataPtr = data.data();
	DWORD bytesRead = 0;
	DWORD totalBytesRead = 0;
	DWORD remainingSize = fileSize;
	while (true) {
		DWORD bytesToRead = min(1024, remainingSize);
		if (bytesToRead == 0) {
			break;
		}
		if (!ReadFile(file, dataPtr, bytesToRead, &bytesRead, NULL)) {
			WinError winErr;
			fileParsingErr("Error reading file: %ls\n", winErr.getMessage());
			// file closed automatically by fileCloser
			return;
		}
		if (bytesRead != bytesToRead) {
			break;
		}
		remainingSize -= bytesRead;
		dataPtr += bytesRead;
	}
	CloseHandle(file);
	fileCloser.file = NULL;
	data.back() = '\0';
	
	LineReaderFromString lineReader(data.data());
	const char* lineStart;
	const char* lineEnd;
	int order = 0;
	while (lineReader.readLine(&lineStart, &lineEnd)) {
		#define shrinkWhitespaceLeft \
			while (lineStart < lineEnd && *lineStart <= 32) ++lineStart;
		#define shrinkWhitespaceRight \
			while (lineEnd > lineStart && *(lineEnd - 1) <= 32) --lineEnd;
		#define shrinkWhitespace \
			shrinkWhitespaceLeft \
			shrinkWhitespaceRight
		shrinkWhitespace
		if (lineEnd <= lineStart) continue;
		SigscanCacheEntry newEntry;
		int setStringLen;
		
		#define setString(vec, start, end) \
			setStringLen = end - (start); \
			if (setStringLen > 0) { \
				vec.resize(setStringLen + 1); \
				memcpy(vec.data(), start, setStringLen); \
				vec.back() = '\0'; \
			}
			
		setString(newEntry.logname, lineStart, lineEnd)
		
		if (!lineReader.readLine(&lineStart, &lineEnd)) fileParsingErr("Missing the first line.")
		shrinkWhitespace
		if (lineEnd <= lineStart) fileParsingErr("First line is empty.")
		
		const char* ptr = (const char*)memchr(lineStart, ':', lineEnd - lineStart);
		if (ptr == nullptr) fileParsingErr(": not found.")
		if (lineEnd - 1 <= ptr) fileParsingErr("Section name to the right of : not found.")
		if (ptr - 1 <= lineStart) fileParsingErr("Module name to the left of : not found.")
		
		setString(newEntry.moduleName, lineStart, ptr)
		setString(newEntry.section, ptr + 1, lineEnd)
		
		if (!lineReader.readLine(&lineStart, &lineEnd)) fileParsingErr("Missing second line (start;end).")
		shrinkWhitespace
		if (lineEnd <= lineStart) fileParsingErr("Second line empty.")
		static const char startStr[] = "start";
		static const char endStr[] = "end";
		#define assertStr(strAr) \
			if (lineEnd - lineStart < sizeof strAr - 1) fileParsingErr("Not enough space to fit in the word '%s'.", strAr) \
			if (strncmp(lineStart, strAr, sizeof strAr - 1) != 0) fileParsingErr("Word '%s' not found.", strAr) \
			lineStart += sizeof strAr - 1;
		assertStr(startStr)
		shrinkWhitespaceLeft
		if (lineStart >= lineEnd || *lineStart != '=') fileParsingErr("Second line doesn't have '=' ('start').")
		++lineStart;
		shrinkWhitespaceLeft
		if (lineStart >= lineEnd) fileParsingErr("Start offset not found on second line.")
		uintptr_t accum = 0;
		const char* numStart = lineStart;
		#define parseNumber \
			if (*lineStart == '0' && lineStart + 1 < lineEnd && *(lineStart + 1) == 'x') { \
				lineStart += 2; \
				while (lineStart < lineEnd) { \
					if (*lineStart >= '0' && *lineStart <= '9') { \
						accum = (accum << 4) | (*lineStart - '0'); \
					} else if (*lineStart >= 'a' && *lineStart <= 'f') { \
						accum = (accum << 4) | (*lineStart - 'a' + 10); \
					} else if (*lineStart >= 'A' && *lineStart <= 'F') { \
						accum = (accum << 4) | (*lineStart - 'A' + 10); \
					} else { \
						break; \
					} \
					++lineStart; \
				} \
			} else { \
				while (lineStart < lineEnd) { \
					if (*lineStart >= '0' && *lineStart <= '9') { \
						accum = accum * 10 + *lineStart - '0'; \
					} else { \
						break; \
					} \
					++lineStart; \
				} \
			}
		parseNumber
		if (lineStart == numStart) fileParsingErr("Start offset empty on second line.")
		newEntry.startRelativeToWholeModule = accum;
		if (lineStart >= lineEnd || *lineStart != ';') fileParsingErr("';' character not found on second line.")
		++lineStart;
		shrinkWhitespaceLeft
		assertStr(endStr)
		shrinkWhitespaceLeft
		if (lineStart >= lineEnd || *lineStart != '=') fileParsingErr("Second line doesn't have '=' ('end').")
		++lineStart;
		shrinkWhitespaceLeft
		numStart = lineStart;
		accum = 0;
		parseNumber
		if (lineStart == numStart) fileParsingErr("End offset empty on second line.")
		newEntry.endRelativeToWholeModule = accum;
		shrinkWhitespaceLeft
		if (lineStart != lineEnd) fileParsingErr("Unknown characters after the end offset on second line.")
		
		#define funcForSetSig(unused, start, end) newEntry.parseSigFromHex(start, end);
		
		#define parseElement(elementName, func) \
			if (!lineReader.readLine(&lineStart, &lineEnd)) fileParsingErr("Missing line (" elementName ").") \
			shrinkWhitespace \
			if (lineEnd <= lineStart) fileParsingErr("The line that was supposed to define " elementName " is empty.") \
			static const char elementName##Str[] = #elementName; \
			assertStr(elementName##Str) \
			shrinkWhitespaceLeft \
			if (lineStart >= lineEnd || *lineStart != '=') fileParsingErr("The line that define the element " elementName " doesn't contain '='.") \
			++lineStart; \
			shrinkWhitespaceLeft \
			if (lineStart < lineEnd) { \
				func(newEntry.elementName, lineStart, lineEnd) \
			}
		
		parseElement(sig, funcForSetSig)
		parseElement(mask, setString)
		parseElement(maskForCaching, setString)
		
		if (!lineReader.readLine(&lineStart, &lineEnd)) fileParsingErr("Missing final line (the sigscan offset).")
		shrinkWhitespace
		if (lineStart >= lineEnd) fileParsingErr("Final line is empty.")
		numStart = lineStart;
		accum = 0;
		parseNumber
		if (lineStart == numStart) fileParsingErr("Empty result on final line.")
		shrinkWhitespaceLeft
		if (lineStart != lineEnd) fileParsingErr("Unknown characters after result on final line.")
		
		sigscanCache[newEntry] = { accum, order++ };
	}
	#undef shrinkWhitespaceLeft
	#undef shrinkWhitespaceRight
	#undef shrinkWhitespace
	#undef setString
	#undef assertStr
	#undef parseNumber
	#undef funcForSetSig
	#undef parseElement
	#undef fileParsingErr
}

static void determineNameSection(uintptr_t start, const char** moduleNameResult, const char** sectionResult, uintptr_t* wholeModuleStart) {
	static char nameBuf[256];
	static char sectionBuf[16] { 0 };
	HANDLE currentProc = GetCurrentProcess();
	
	if (allModules.empty()) {
		DWORD cbNeeded = 0;
		if (!EnumProcessModules(currentProc, NULL, 0, &cbNeeded)) {
			return;
		}
		
		while (true) {
			allModules.resize(cbNeeded / sizeof HMODULE);
			DWORD cbSize = allModules.size() * sizeof HMODULE;
			if (!EnumProcessModules(currentProc, allModules.data(), cbSize, &cbNeeded)) {
				return;
			}
			if (cbNeeded <= cbSize) break;
		}
		allModules.resize(cbNeeded / sizeof HMODULE);
	}
	
	MODULEINFO modInfo;
	DWORD hModCount = (DWORD)allModules.size();
	for (DWORD i = 0; i < hModCount; ++i) {
		HMODULE hMod = allModules[i];
		if (!GetModuleInformation(currentProc, hMod, &modInfo, sizeof MODULEINFO)) {
			continue;
		}
		if (start >= (uintptr_t)modInfo.lpBaseOfDll && start < (uintptr_t)modInfo.lpBaseOfDll + modInfo.SizeOfImage) {
			if (!GetModuleBaseNameA(currentProc, hMod, nameBuf, sizeof nameBuf)) {
				return;
			}
			const IMAGE_DOS_HEADER* dosHeader = (const IMAGE_DOS_HEADER*)modInfo.lpBaseOfDll;
			if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return;
			const IMAGE_NT_HEADERS32* ntHeader = (const IMAGE_NT_HEADERS32*)((BYTE*)modInfo.lpBaseOfDll + dosHeader->e_lfanew);
			if (ntHeader->Signature != IMAGE_NT_SIGNATURE) return;
			WORD NumberOfSections = ntHeader->FileHeader.NumberOfSections;
			const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeader);
			for (WORD i = 0; i < NumberOfSections; ++i) {
				uintptr_t vaStart = (uintptr_t)modInfo.lpBaseOfDll + section->VirtualAddress;
				if (start >= vaStart && start < vaStart + section->Misc.VirtualSize) {
					memcpy(sectionBuf, section->Name, IMAGE_SIZEOF_SHORT_NAME);
					// should be null terminated, because we initialized the sectionBuf array with {0}
					*wholeModuleStart = (uintptr_t)modInfo.lpBaseOfDll;
					*moduleNameResult = nameBuf;
					*sectionResult = sectionBuf;
					return;
				}
				++section;
			}
			return;
		}
	}
}

static uintptr_t fillInEntry(const char* logname, const char* name, const char* section, uintptr_t start, uintptr_t end, uintptr_t wholeModuleBegin,
				const char* sig, size_t sigLength, const char* mask, const char* maskForCaching, SigscanCacheEntry* result) {
	if (!name || !section) {
		determineNameSection(start, &name, &section, &wholeModuleBegin);
	} else if (wholeModuleBegin == 0) {
		HMODULE moduleHandle = GetModuleHandleA(name);
		if (moduleHandle) {
			MODULEINFO moduleInfo;
			if (GetModuleInformation(GetCurrentProcess(), moduleHandle, &moduleInfo, sizeof MODULEINFO)) {
				wholeModuleBegin = (uintptr_t)moduleInfo.lpBaseOfDll;
			}
		}
	}
	if (wholeModuleBegin && name && section) {
		result->assignFromStr(result->logname, logname);
		result->assignFromStr(result->moduleName, name);
		result->assignFromStr(result->section, section);
		result->startRelativeToWholeModule = start - wholeModuleBegin;
		result->endRelativeToWholeModule = end - wholeModuleBegin;
		result->assignFromStr(result->sig, sig, sigLength);
		result->assignFromStr(result->mask, mask);
		result->assignFromStr(result->maskForCaching, maskForCaching);
		result->applyMaskForCachingToSig();
		return wholeModuleBegin;
	}
	return 0;
}

static void recordResult(const SigscanCacheEntry& entry, sigscanCacheType::iterator& oldResult, uintptr_t relativeResult) {
	if (oldResult != sigscanCache.end()) {
		if (oldResult->second.offset != relativeResult) {
			sigscansChanged = true;
			oldResult->second.offset = relativeResult;
		}
		oldResult->second.order = sigscanOrder;
	} else {
		sigscanCache[entry] = { relativeResult, sigscanOrder };
		sigscansChanged = true;
	}
}

static int findChar(const char* str, char searchChar) {
	for (const char* c = str; *c != '\0'; ++c) {
		if (*c == searchChar) return c - str;
	}
	return -1;
}

bool getModuleBounds(const char* name, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin, char* namePtr, char* sectionPtr)
{
	splitOutModuleName(name, namePtr, 256, sectionPtr, 16);
	if (sectionPtr[0] == '\0') {
		strcpy(sectionPtr, ".text");
	}
	return getModuleBounds(namePtr, sectionPtr, start, end, wholeModuleBegin);
}

bool getModuleBounds(const char* name, const char* sectionName, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin)
{
	HMODULE module = GetModuleHandleA(name);
	if (module == nullptr)
		return false;

	return getModuleBoundsHandle(module, sectionName, start, end, wholeModuleBegin);
}

bool getModuleBoundsHandle(HMODULE hModule, const char* sectionName, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin)
{
	MODULEINFO info;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info))) return false;
	*wholeModuleBegin = (uintptr_t)(info.lpBaseOfDll);
	*start = (uintptr_t)(info.lpBaseOfDll);
	*end = *start + info.SizeOfImage;
	if (strcmp(sectionName, "all") == 0) return true;
	const IMAGE_DOS_HEADER* dosHeader = (const IMAGE_DOS_HEADER*)info.lpBaseOfDll;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return true;
	const IMAGE_NT_HEADERS32* ntHeader = (const IMAGE_NT_HEADERS32*)((BYTE*)dosHeader + dosHeader->e_lfanew);
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE) return true;
	WORD numberOfSections = ntHeader->FileHeader.NumberOfSections;
	const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeader);
	for (; numberOfSections != 0; --numberOfSections) {
		if (strncmp((const char*)section->Name, sectionName, IMAGE_SIZEOF_SHORT_NAME) == 0) {
			*start = *start + section->VirtualAddress;
			*end = *start + section->Misc.VirtualSize;
			return true;
		}
		++section;
	}
	return true;
}

bool getModuleBoundsHandle(HMODULE hModule, uintptr_t* start, uintptr_t* end, uintptr_t* wholeModuleBegin)
{
	return getModuleBoundsHandle(hModule, ".text", start, end, wholeModuleBegin);
}

#define byteSpecificationError \
	logwrap(fprintf(logfile, "Wrong byte specification: %s\n", byteSpecification)); \
	return numOfTriangularChars;

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
size_t byteSpecificationToSigMask(const char* byteSpecification, std::vector<char>& sig, std::vector<char>& mask, size_t* position, size_t positionLength, std::vector<char>* maskForCaching) {
	if (position && positionLength == 0) positionLength = UINT_MAX;
	size_t numOfTriangularChars = 0;
	sig.clear();
	mask.clear();
	if (maskForCaching) maskForCaching->clear();
	unsigned long long accumulatedNibbles = 0;
	int nibbleCount = 0;
	bool nibblesUnknown = false;
	const char* byteSpecificationPtr = byteSpecification;
	bool nibbleError = false;
	const char* nibbleSequenceStart = byteSpecification;
	while (true) {
		char currentChar = *byteSpecificationPtr;
		if (currentChar == '>') {
			if (position && numOfTriangularChars < positionLength) {
				*position = sig.size();
				++position;
			}
			++numOfTriangularChars;
			nibbleSequenceStart = byteSpecificationPtr + 1;
		} else if (currentChar == '(') {
			nibbleCount = 0;
			nibbleError = false;
			nibblesUnknown = false;
			accumulatedNibbles = 0;
			if (byteSpecificationPtr <= nibbleSequenceStart) {
				byteSpecificationError
			}
			const char* moduleNameEnd = byteSpecificationPtr;
			++byteSpecificationPtr;
			bool parseOk = true;
			#define skipWhitespace \
				while (*byteSpecificationPtr != '\0' && *byteSpecificationPtr <= 32) { \
					++byteSpecificationPtr; \
				}
			#define checkQuestionMarks \
				if (parseOk) { \
					if (strncmp(byteSpecificationPtr, "??", 2) != 0) { \
						parseOk = false; \
					} else { \
						byteSpecificationPtr += 2; \
					} \
				}
			#define checkWhitespace \
				if (parseOk) { \
					if (*byteSpecificationPtr == '\0' || *byteSpecificationPtr > 32) { \
						parseOk = false; \
					} else { \
						while (*byteSpecificationPtr != '\0' && *byteSpecificationPtr <= 32) { \
							++byteSpecificationPtr; \
						} \
					} \
				}
			skipWhitespace
			checkQuestionMarks
			checkWhitespace
			checkQuestionMarks
			checkWhitespace
			checkQuestionMarks
			checkWhitespace
			checkQuestionMarks
			skipWhitespace
			#undef skipWhitespace
			#undef checkQuestionMarks
			#undef checkWhitespace
			if (*byteSpecificationPtr != ')') {
				parseOk = false;
			}
			if (!parseOk) {
				byteSpecificationError
			}
			if (maskForCaching) {
				size_t oldSize = maskForCaching->size();
				if (moduleNameEnd - nibbleSequenceStart == 3 && strncmp(nibbleSequenceStart, "rel", 3) == 0) {
					static const char additionString[] = "rel_GuiltyGearXrd.exe(????)";
					maskForCaching->resize(oldSize + sizeof additionString - 1);
					memcpy(maskForCaching->data() + oldSize, additionString, sizeof additionString - 1);
				} else {
					maskForCaching->resize(oldSize + 4 + (moduleNameEnd - nibbleSequenceStart) + 6);
					char* dataPtr = maskForCaching->data() + oldSize;
					memcpy(dataPtr, "rel_", 4);
					dataPtr += 4;
					memcpy(dataPtr, nibbleSequenceStart, moduleNameEnd - nibbleSequenceStart);
					dataPtr += moduleNameEnd - nibbleSequenceStart;
					memcpy(dataPtr, "(????)", 6);
				}
				sig.resize(sig.size() + 4, '\0');
				mask.resize(mask.size() + 4, '?');
			}
		} else if (currentChar != ' ' && currentChar != '\0') {
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
				nibbleError = true;
			}
			accumulatedNibbles = (accumulatedNibbles << 4) | currentNibble;
			++nibbleCount;
			if (nibbleCount > 16) {
				nibbleError = true;
			}
		} else {
			if (nibbleCount) {
				if (nibbleError) {
					byteSpecificationError
				}
				do {
					if (!nibblesUnknown) {
						sig.push_back(accumulatedNibbles & 0xff);
						mask.push_back('x');
						if (maskForCaching) maskForCaching->push_back('x');
						accumulatedNibbles >>= 8;
					} else {
						sig.push_back(0);
						mask.push_back('?');
						if (maskForCaching) maskForCaching->push_back('?');
					}
					nibbleCount -= 2;
				} while (nibbleCount > 0);
				nibbleCount = 0;
				nibblesUnknown = false;
			}
			if (currentChar == '\0') {
				break;
			}
			nibbleSequenceStart = byteSpecificationPtr + 1;
		}
		++byteSpecificationPtr;
	}
	sig.push_back('\0');
	mask.push_back('\0');
	if (maskForCaching) maskForCaching->push_back('\0');
	#undef byteSpecificationError
	return numOfTriangularChars;
}

uintptr_t sigscan(const char* name, const char* sig, size_t sigLength, const char* logname, const char* maskForCaching)
{
	uintptr_t start, end, wholeModuleBegin;
	char moduleName[256] { 0 };
	char sectionName[16] { 0 };
	if (!getModuleBounds(name, &start, &end, &wholeModuleBegin, moduleName, sectionName)) {
		logwrap(fputs("Module not loaded\n", logfile));
		return 0;
	}
	
	return sigscan(start, end, sig, sigLength, logname, maskForCaching, moduleName, sectionName, wholeModuleBegin);
}

uintptr_t sigscan(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength, const char* logname, const char* maskForCaching, const char* name, const char* section, uintptr_t wholeModuleBegin) {
	uintptr_t result;
	static SigscanCacheEntry lookupEntry;
	sigscanCacheType::iterator oldResult = sigscanCache.end();
	bool useCache = logname && settings.useSigscanCaching;
	if (useCache) {
		++sigscanOrder;
		loadSigscanCache();
		wholeModuleBegin = fillInEntry(logname, name, section, start, end, wholeModuleBegin, sig, sigLength, nullptr, maskForCaching, &lookupEntry);
		oldResult = sigscanCache.find(lookupEntry);
		if (oldResult != sigscanCache.end()) {
			if (oldResult->first.equal(lookupEntry) && oldResult->second.offset) {
				uintptr_t absoluteFound = oldResult->second.offset + wholeModuleBegin;
				if (absoluteFound >= start && absoluteFound + sigLength <= end) {
					if (memcmp((void*)absoluteFound, sig, sigLength) == 0) {
						oldResult->second.order = sigscanOrder;
						return absoluteFound;
					}
				}
			}
			sigscansChanged = true;
			sigscanCache.erase(oldResult);
			oldResult = sigscanCache.end();
		}
	}
	
	result = sigscanFundamental(start, end, sig, sigLength);
	if (!result) {
		logwrap(fputs("Sigscan failed\n", logfile));
	} else if (useCache) {
		recordResult(lookupEntry, oldResult, result - wholeModuleBegin);
	} 
	return result;
}

uintptr_t sigscanFundamental(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength) {
	
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

	return 0;
}

/// <summary>
/// Prepares the step array before searching the same sig in many places.
/// </summary>
/// <param name="sig">The needle to search for.</param>
/// <param name="sigLength">The length of the needle, in bytes.</param>
/// <param name="step">The array must hold 256 size_t elements.</param>
void sigscanCaseInsensitivePrepare(const char* sig, size_t sigLength, size_t* step) {
	// Boyer-Moore-Horspool substring search
	// A table containing, for each symbol in the alphabet, the number of characters that can safely be skipped
	for (int i = 0; i < 256; ++i) {
		step[i] = sigLength;
	}
	for (size_t i = 0; i < sigLength - 1; i++) {
		step[(BYTE)sig[i]] = sigLength - 1 - i;
	}
}

uintptr_t sigscanCaseInsensitive(uintptr_t start, uintptr_t end, const char* sig, size_t sigLength, size_t* step) {
	
	// Boyer-Moore-Horspool substring search
	
	char pNext;
	char pCheck;
	end -= sigLength;
	for (uintptr_t p = start; p <= end; p += step[(BYTE)pNext]) {
		int j = sigLength - 1;
		pNext = *(char*)(p + j);
		if (pNext >= 'A' && pNext <= 'Z') pNext = 'a' + pNext - 'A';
		if (sig[j] == pNext) {
			for (--j; j >= 0; --j) {
				pCheck = *(char*)(p + j);
				if (pCheck >= 'A' && pCheck <= 'Z') pCheck = 'a' + pCheck - 'A';
				if (sig[j] != pCheck) {
					break;
				}
			}
			if (j < 0) {
				return p;
			}
		}
	}

	return 0;
}

void splitOutModuleName(const char* name, char* moduleName, size_t moduleNameBufSize, char* sectionName, size_t sectionNameBufSize) {
	bool foundColon = false;
	for (const char* c = name; *c != '\0'; ++c) {
		if (*c == ':') {
			foundColon = true;
		} else if (!foundColon) {
			if (moduleNameBufSize <= 1) continue;
			*moduleName = *c;
			++moduleName;
			--moduleNameBufSize;
		} else {
			if (sectionNameBufSize <= 1) continue;
			*sectionName = *c;
			++sectionName;
			--sectionNameBufSize;
		}
	}
	if (moduleNameBufSize) *moduleName = '\0';
	if (sectionNameBufSize) *sectionName = '\0';
}

uintptr_t sigscan(const char* name, const char* sig, const char* mask, const char* logname, const char* maskForCaching)
{
	uintptr_t start, end, wholeModuleBegin;
	char moduleName[256] { 0 };
	char sectionName[16] { 0 };
	if (!getModuleBounds(name, &start, &end, &wholeModuleBegin, moduleName, sectionName)) {
		logwrap(fputs("Module not loaded\n", logfile));
		return 0;
	}
	
	return sigscan(start, end, sig, mask, logname, maskForCaching, moduleName, sectionName, wholeModuleBegin);
}

uintptr_t sigscan(uintptr_t start, uintptr_t end, const char* sig, const char* mask, const char* logname, const char* maskForCaching, const char* name, const char* section, uintptr_t wholeModuleBegin) {
	uintptr_t result;
	static SigscanCacheEntry lookupEntry;
	auto oldResult = sigscanCache.end();
	bool useCache = logname && settings.useSigscanCaching;
	if (useCache) {
		++sigscanOrder;
		loadSigscanCache();
		int maskLen = strlen(mask);
		wholeModuleBegin = fillInEntry(logname, name, section, start, end, wholeModuleBegin, sig, maskLen, mask, maskForCaching, &lookupEntry);
		oldResult = sigscanCache.find(lookupEntry);
		if (oldResult != sigscanCache.end()) {
			if (oldResult->first.equal(lookupEntry) && oldResult->second.offset) {
				uintptr_t absoluteFound = oldResult->second.offset + wholeModuleBegin;
				if (absoluteFound >= start && absoluteFound + maskLen <= end) {
					result = sigscanFundamental(absoluteFound, absoluteFound + maskLen, sig, mask);
					if (result == absoluteFound) {
						oldResult->second.order = sigscanOrder;
						return result;
					}
				}
			}
			sigscansChanged = true;
			sigscanCache.erase(oldResult);
			oldResult = sigscanCache.end();
		}
	}
	
	result = sigscanFundamental(start, end, sig, mask);
	if (!result) {
		logwrap(fputs("Sigscan failed\n", logfile));
	} else if (useCache) {
		recordResult(lookupEntry, oldResult, result - wholeModuleBegin);
	}
	return result;
}

uintptr_t sigscanFundamental(uintptr_t start, uintptr_t end, const char* sig, const char* mask) {
	uintptr_t lastScan = end - strlen(mask) + 1;
	for (auto addr = start; addr < lastScan; addr++) {
		for (size_t i = 0;; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
				break;
		}
	}

	return 0;
}

uintptr_t sigscanBackwards(uintptr_t startBottom, uintptr_t endTop, const char* sig, const char* mask) {
	uintptr_t lastScan = endTop;
	for (auto addr = startBottom - strlen(mask) + 1; addr >= lastScan; addr--) {
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
// for finding function starts
uintptr_t sigscanBackwards16ByteAligned(uintptr_t startBottom, uintptr_t endTop, const char* sig, const char* mask) {
	uintptr_t lastScan = endTop;
	for (auto addr = (startBottom - strlen(mask) + 1) & 0xFFFFFFF0; addr >= lastScan; addr-=16) {
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
uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, sig, sigLength, nullptr, {}, error, logname, maskForCaching);
}

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, sig, 0, mask, {}, error, logname, maskForCaching);
}

uintptr_t sigscanOffset(const char* name, const std::vector<char>& sig, const std::vector<char>& mask, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, sig.data(), 0, mask.data(), {}, error, logname, maskForCaching);
}

uintptr_t sigscanBufOffset(const char* name, const char* sig, const size_t sigLength, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, sig, sigLength, nullptr, offsets, error, logname, maskForCaching);
}

uintptr_t sigscanOffset(const char* name, const char* sig, const char* mask, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, sig, 0, mask, offsets, error, logname, maskForCaching);
}

uintptr_t sigscanOffset(const char* name, const std::vector<char>& sig, const std::vector<char>& mask, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, sig.data(), 0, mask.data(), offsets, error, logname, maskForCaching);
}

uintptr_t sigscanStrOffset(const char* name, const char* str, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, str, strlen(str), nullptr, {}, error, logname, maskForCaching);
}

uintptr_t sigscanOffset(const char* name, const char* byteSpecification, bool* error, const char* logname, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	std::vector<char> maskForCaching;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength, &maskForCaching);
	uintptr_t result = sigscanOffsetMain(name, sig.data(), 0, mask.data(), {}, error, logname, maskForCaching.data());
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
}

uintptr_t sigscanStrOffset(const char* name, const char* str, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching) {
	return sigscanOffsetMain(name, str, strlen(str), nullptr, offsets, error, logname, maskForCaching);
}

uintptr_t sigscanOffset(const char* name, const char* byteSpecification, const std::initializer_list<int>& offsets, bool* error, const char* logname, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	std::vector<char> maskForCaching;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength, &maskForCaching);
	uintptr_t result = sigscanOffsetMain(name, sig.data(), 0, mask.data(), offsets, error, logname, maskForCaching.data());
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
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
uintptr_t sigscanOffsetMain(const char* name, const char* sig, size_t sigLength, const char* mask, const std::initializer_list<int>& offsets, bool* error, const char* logname, const char* maskForCaching) {
	uintptr_t sigscanResult;
	if (mask) {
		if (findChar(mask, '?') == -1) {
			sigLength = strlen(mask);
			sigscanResult = sigscan(name, sig, sigLength, logname, maskForCaching);
		} else {
			sigscanResult = sigscan(name, sig, mask, logname, maskForCaching);
		}
	} else {
		sigscanResult = sigscan(name, sig, sigLength, logname, maskForCaching);
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
	for (int it : offsets) {
		if (!isFirst) {
			addr = *(uintptr_t*)addr;
		}
		addr += it;
		isFirst = false;
	}
	if (offsets.size()) {
		if (logname) {
			logwrap(fprintf(logfile, "Final location of %s at %.8x\n", logname, addr));
		}
	}
	return addr;
}

// callInstructionAddr points to the start of a relative call instruction.
// the relative call instruction is one byte of the instruction command, followed by 4 bytes relative offset. Offset can be negative (start with FF...).
// The relative offset is relative to the instruction that goes after the call smh, and the call instruction itself is 5 bytes
uintptr_t followRelativeCall(uintptr_t callInstructionAddr) {
	logwrap(fprintf(logfile, "Following relative call at %.8x, the call looks like ", callInstructionAddr));
	unsigned char* c = (unsigned char*)callInstructionAddr;
	for (int i = 5; i != 0; --i) {
		logwrap(fprintf(logfile, "%.2hhx ", *c));
		++c;
	}
	logwrap(fprintf(logfile, "and specifies offset %d\n", *(int*)(callInstructionAddr + 1)));
	return callInstructionAddr + 5 + *(int*)(callInstructionAddr + 1);
	// Calls can also have absolute addresses so check which one you got
}

uintptr_t followSinglyByteJump(uintptr_t jumpInstrAddr) {
	return jumpInstrAddr + 2 + *(char*)(jumpInstrAddr + 1);
}

uintptr_t followRelativeCallNoLogs(uintptr_t relativeCallAddr) {
	return relativeCallAddr + 5 + *(int*)(relativeCallAddr + 1);
	// Calls can also have absolute addresses so check which one you got
}

int calculateRelativeCallOffset(uintptr_t callInstructionAddr, uintptr_t destinationAddr) {
	// relativeCallAddr points to the start of a relative call instruction.
	// destinationAddr points to the function that you would like to call.
	// This function calculates the offset necessary to put in the relative call instruction so that it
	// reaches the destinationAddr.
	// See followRelativeCall(...) for details of a relative call instruction.
	return (int)(destinationAddr - (callInstructionAddr + 5));
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

void substituteWildcard(std::vector<char>& sig, std::vector<char>& mask, unsigned int indexOfWildcard, void* ptrToSubstitute) {
	substituteWildcard(sig.data(), mask.data(), indexOfWildcard, ptrToSubstitute);
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

char* scrollUpToBytes(char *ptr, const char* buf, int bufSize, size_t searchLimit) {
	while (searchLimit) {
		--searchLimit;
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

uintptr_t sigscanBackwards(uintptr_t ptr, const char* byteSpecification, size_t searchLimit, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength);
	uintptr_t result = sigscanBackwards(ptr, ptr - searchLimit, sig.data(), mask.data());
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
}
uintptr_t sigscanBackwards16ByteAligned(uintptr_t ptr, const char* byteSpecification, size_t searchLimit, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength);
	uintptr_t result = sigscanBackwards16ByteAligned(ptr, ptr - searchLimit, sig.data(), mask.data());
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
}
uintptr_t sigscanForward(uintptr_t ptr, const char* byteSpecification, size_t searchLimit, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength);
	uintptr_t result;
	if (findChar(mask.data(), '?') == -1) {
		result = sigscan(ptr, ptr + searchLimit, sig.data(), sig.size() - 1, nullptr, nullptr);
	} else {
		result = sigscan(ptr, ptr + searchLimit, sig.data(), mask.data(), nullptr, nullptr);
	}
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
}

uintptr_t sigscanForward(uintptr_t ptr, const char* sig, const char* mask, size_t searchLimit) {
	if (findChar(mask, '?') == -1) {
		size_t strLen = strlen(mask);
		return sigscan(ptr, ptr + searchLimit, sig, strLen, nullptr, nullptr);
	} else {
		return sigscan(ptr, ptr + searchLimit, sig, mask, nullptr, nullptr);
	}
}

uintptr_t sigscanForward(uintptr_t ptr, const std::vector<char>& sig, const std::vector<char>& mask, size_t searchLimit) {
	return sigscanForward(ptr, sig.data(), mask.data(), searchLimit);
}



/// <summary>
/// Finds the address which holds a pointer to a function with the given name imported from the given DLL.
/// For example, searching USER32.DLL, GetKeyState would return a non-0 value on successful find, and
/// if you cast that value to a short (__stdcall**)(int) and dereference it, you would get a pointer to
/// GetKeyState that you can call. Or swap out for hooks.
/// 
/// This function is useless because the calls to imported functions are relative
/// and even then they call thunk functions (a thunk function is a function consisting only
/// of a jump instruction to some other function).
/// </summary>
/// <param name="module">Type "GuiltyGearXrd.exe" here.</param>
/// <param name="dll">Include ".DLL" in the DLL's name here. Case-insensitive.</param>
/// <param name="function">The name of the function. Case-sensitive.</param>
/// <returns>The address which holds a pointer to a function. 0 if not found.</returns>
uintptr_t findImportedFunction(const char* module, const char* dll, const char* function) {
	HMODULE hModule = GetModuleHandleA(module);
	if (!hModule) return 0;
	
	MODULEINFO info;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info))) return false;
	uintptr_t base = (uintptr_t)(info.lpBaseOfDll);
	uintptr_t peHeaderStart = base + *(uintptr_t*)(base + 0x3C);  // PE file header start
	struct RvaAndSize {
		DWORD rva;
		DWORD size;
	};
	const RvaAndSize* importsDataDirectoryRvaAndSize = (const RvaAndSize*)(peHeaderStart + 0x80);
	struct ImageImportDescriptor {
		DWORD ImportLookupTableRVA;  // The RVA of the import lookup table. This table contains a name or ordinal for each import. (The name "Characteristics" is used in Winnt.h, but no longer describes this field.)
		DWORD TimeDateStamp;  // The stamp that is set to zero until the image is bound. After the image is bound, this field is set to the time/data stamp of the DLL. LIES, this field is 0 for me at runtime.
		DWORD ForwarderChain;  // The index of the first forwarder reference. 0 for me.
		DWORD NameRVA;  // The address of an ASCII string that contains the name of the DLL. This address is relative to the image base.
		DWORD ImportAddressTableRVA;  // The RVA of the import address table. The contents of this table are identical to the contents of the import lookup table until the image is bound.
	};
	DWORD importsSize = importsDataDirectoryRvaAndSize->size;  // in bytes
	const ImageImportDescriptor* importPtrNext = (const ImageImportDescriptor*)(base + importsDataDirectoryRvaAndSize->rva);
	for (; importsSize > 0; importsSize -= sizeof ImageImportDescriptor) {
		const ImageImportDescriptor* importPtr = importPtrNext++;
		if (!importPtr->ImportLookupTableRVA) break;
		const char* dllName = (const char*)(base + importPtr->NameRVA);
		if (_stricmp(dllName, dll) != 0) continue;
		void** funcPtr = (void**)(base + importPtr->ImportAddressTableRVA);
		DWORD* imageImportByNameRvaPtr = (DWORD*)(base + importPtr->ImportLookupTableRVA);
		struct ImageImportByName {
			short importIndex;  // if you know this index you can use it for lookup. Name is just convenience for programmers.
			char name[1];  // arbitrary length, zero-terminated ASCII string
		};
		for (; *imageImportByNameRvaPtr != 0; ++imageImportByNameRvaPtr) {
			const ImageImportByName* importByName = (const ImageImportByName*)(base + *imageImportByNameRvaPtr);
			if (strcmp(importByName->name, function) == 0) {
				return (uintptr_t)funcPtr;
			}
			++funcPtr;
		}
		return 0;
	}
	return 0;
}

bool isMovInstructionFromRegToReg(BYTE* ptr, Register* registerDst, Register* registerSrc) {
	BYTE instr = *ptr;
	// xrd doesn't even use the 0x89 mov instruction
	if (instr != 0x89 && instr != 0x8b) return false;
	++ptr;
	BYTE val = *ptr;
	if ((val & 0b11000000) != 0b11000000) return false;
	if (instr == 0x8b) {
		Register* tmp = registerDst;
		registerDst = registerSrc;
		registerSrc = tmp;
	}
	if (registerDst) *registerDst = (Register)(val & 0b111);
	if (registerSrc) *registerSrc = (Register)((val & 0b111000) >> 3);
	return true;
}

int isTestInstructionRegImm(BYTE* ptr, Register* registerLeft, DWORD* offset, DWORD* imm) {
	if (*ptr != 0xf7) return false;
	++ptr;
	BYTE val = *ptr;
	BYTE upper4Bits = (val & 0xf0) >> 4;
	if (upper4Bits != 4 && upper4Bits != 8) return false;
	BYTE reg = val & 0x0f;
	if (reg > 7) return false;
	if (registerLeft) *registerLeft = (Register)reg;
	if (upper4Bits == 4) {
		if (offset) *offset = *(ptr + 1);
		if (imm) *imm = *(DWORD*)(ptr + 2);
		return 7;
	} else {
		if (offset) *offset = *(DWORD*)(ptr + 1);
		if (imm) *imm = *(DWORD*)(ptr + 5);
		return 10;
	}
}

const char* GUILTY_GEAR_XRD_EXE = "GuiltyGearXrd.exe";

void SigscanCacheEntry::parseSigFromHex(const char* start, const char* end) {
	if (end <= start) {
		sig.clear();
		return;
	}
	static std::vector<char> startToEndWithNullTerminator;
	startToEndWithNullTerminator.resize(end - start + 1);
	memcpy(startToEndWithNullTerminator.data(), start, end - start);
	startToEndWithNullTerminator.back() = '\0';
	
	static std::vector<char> maskBuf;
	byteSpecificationToSigMask(startToEndWithNullTerminator.data(), sig, maskBuf);
}

void SigscanCacheEntry::applyMaskForCachingToSig() {
	if (maskForCaching.empty() && mask.empty() || sig.empty()) return;
	const char* ptr = maskForCaching.empty() ? mask.data() : maskForCaching.data();
	char* sigPtr = sig.data();
	while (true) {
		const char c = *ptr;
		if (c == '\0') return;
		if (c == '?') {
			*sigPtr = '\0';
		} else if (c == 'r') {
			if (strncmp(ptr, "rel_", 4) != 0) return;
			ptr += 4;
			const char* moduleNameStart = ptr;
			while (*ptr != '\0' && *ptr != '(') ++ptr;
			const char* moduleNameEnd = ptr;
			if (strncmp(ptr, "(????)", 6) != 0) return;
			ptr += 5;
			static std::vector<char> moduleLookup;
			moduleLookup.resize(moduleNameEnd - moduleNameStart + 1);
			if (moduleNameEnd > moduleNameStart) {
				memcpy(moduleLookup.data(), moduleNameStart, moduleNameEnd - moduleNameStart);
			}
			moduleLookup.back() = '\0';
			HMODULE hMod = GetModuleHandleA(moduleLookup.data());
			if (hMod) {
				MODULEINFO moduleInfo;
				if (GetModuleInformation(GetCurrentProcess(), hMod, &moduleInfo, sizeof MODULEINFO)) {
					DWORD addr;
					memcpy(&addr, sigPtr, 4);
					DWORD offset = addr - (DWORD)moduleInfo.lpBaseOfDll;
					memcpy(sigPtr, &offset, 4);
				}
			}
			sigPtr += 3;
			++ptr;
		}
		++ptr;
		++sigPtr;
	}
}

void finishedSigscanning() {
	#define fileWriteErr(msg, ...) { logwrap(fprintf(logfile, "%s writing error: " msg, SIGSCAN_CACHE_FILE_NAME, __VA_ARGS__)); return; }
	if (!sigscansChanged) return;
	sigscansChanged = false;
	
	bool failedWrite = false;
	DWORD bytesWritten = 0;
	#define failWrite { failedWrite = true; break; }
	using SortElement = std::pair<const SigscanCacheEntry, SigscanCacheValue>;
	std::vector<const SortElement*> cacheOrdered;
	cacheOrdered.reserve(sigscanCache.size());
	int cacheOrderedSize = 0;
	for (const auto& it : sigscanCache) {
		cacheOrdered.push_back(&it);
	}
	struct MyCompare {
		static int __cdecl TheCompare(void const* elemLeft, void const* elemRight) {
			const SortElement* pairLeft = *(const SortElement**)elemLeft;
			const SortElement* pairRight = *(const SortElement**)elemRight;
			return pairLeft->second.order - pairRight->second.order;
		}
	};
	if (!cacheOrdered.empty()) {
		qsort(cacheOrdered.data(), cacheOrdered.size(), sizeof (void*), MyCompare::TheCompare);
	}
	
	HANDLE file = CreateFileA(SIGSCAN_CACHE_FILE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		WinError winErr;
		fileWriteErr("Could not create file: %ls\n", winErr.getMessage());
		return;
	}
	struct FileCloser {
		~FileCloser() {
			if (file) CloseHandle(file);
		}
		HANDLE file;
	} fileCloser {
		file
	};
	
	for (const SortElement* pair : cacheOrdered) {
		const SigscanCacheEntry& entry = pair->first;
		#define writeNewline if (!WriteFile(file, "\r\n", 2, &bytesWritten, NULL)) failWrite
		if (entry.logname.size() > 1) {
			if (!WriteFile(file, entry.logname.data(), entry.logname.size() - 1, &bytesWritten, NULL)) failWrite
		}
		writeNewline
		if (entry.moduleName.size() > 1) {
			if (!WriteFile(file, entry.moduleName.data(), entry.moduleName.size() - 1, &bytesWritten, NULL)) failWrite
		}
		if (!WriteFile(file, ":", 1, &bytesWritten, NULL)) failWrite
		if (entry.section.size() > 1) {
			if (!WriteFile(file, entry.section.data(), entry.section.size() - 1, &bytesWritten, NULL)) failWrite
		}
		#define writeStr(name, literal) \
			static const char name[] = literal; \
			if (!WriteFile(file, name, sizeof name - 1, &bytesWritten, NULL)) failWrite
		writeNewline
		writeStr(startStr, "start=")
		int result;
		static char strbuf[256];
		#define writeHex(val) \
			result = sprintf_s(strbuf, "0x%.8x", val); \
			if (!WriteFile(file, strbuf, result, &bytesWritten, NULL)) failWrite
		writeHex(entry.startRelativeToWholeModule)
		writeStr(endStr, ";end=")
		writeHex(entry.endRelativeToWholeModule)
		writeNewline
		writeStr(sigStr, "sig=")
		size_t sizeConst = entry.sig.size();
		if (sizeConst > 1) {
			--sizeConst;
			for (size_t iter = 0; iter < sizeConst; ++iter) {
				BYTE b = (BYTE)entry.sig[iter];
				char twoChars[3];
				int twoCharsSize = 2;
				for (int i = 0; i < 2; ++i) {
					BYTE nibble = b & 0x0F;
					b >>= 4;
					if (nibble >= 10) {
						twoChars[1 - i] = 'A' + nibble - 10;
					} else {
						twoChars[1 - i] = '0' + nibble;
					}
				}
				if (iter < sizeConst - 1) {
					twoChars[2] = ' ';
					twoCharsSize = 3;
				}
				if (!WriteFile(file, twoChars, twoCharsSize, &bytesWritten, NULL)) failWrite
			}
		}
		writeNewline
		writeStr(maskStr, "mask=")
		if (entry.mask.size() > 1) {
			if (!WriteFile(file, entry.mask.data(), entry.mask.size() - 1, &bytesWritten, NULL)) failWrite
		}
		writeNewline
		writeStr(maskForCachingStr, "maskForCaching=")
		if (entry.maskForCaching.size() > 1) {
			if (!WriteFile(file, entry.maskForCaching.data(), entry.maskForCaching.size() - 1, &bytesWritten, NULL)) failWrite
		}
		writeNewline
		writeHex(pair->second.offset)
		writeNewline
		writeNewline
	}
	
	if (failedWrite) {
		WinError winErr;
		fileWriteErr("Failed to write to file: %ls\n", winErr.getMessage());
	}
}

bool SigscanCacheEntry::equal(const SigscanCacheEntry& other) const {
	return (logname.empty() ? other.logname.empty() : strcmp(logname.data(), other.logname.data()) == 0)
			&& (moduleName.empty() ? other.moduleName.empty() : _stricmp(moduleName.data(), other.moduleName.data()) == 0)
			&& (section.empty() ? other.section.empty() : strcmp(section.data(), other.section.data()) == 0)
			&& startRelativeToWholeModule == other.startRelativeToWholeModule
			&& endRelativeToWholeModule == other.endRelativeToWholeModule
			&& sig.size() == other.sig.size()
			&& (sig.empty() || memcmp(sig.data(), other.sig.data(), sig.size()) == 0)
			&& mask.size() == other.mask.size()
			&& (mask.empty() || memcmp(mask.data(), other.mask.data(), mask.size()) == 0)
			&& maskForCaching.size() == other.maskForCaching.size()
			&& (maskForCaching.empty() || memcmp(maskForCaching.data(), other.maskForCaching.data(), maskForCaching.size()) == 0);
}

bool thisIsOurFunction(uintptr_t functionAddr) {
	uintptr_t textStart, textEnd, wholeModuleBegin;
	getModuleBoundsHandle(hInst, ".text", &textStart, &textEnd, &wholeModuleBegin);
	return functionAddr >= textStart && functionAddr < textEnd;
}
