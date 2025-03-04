#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include "WinError.h"

// original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/

DWORD findOpenGgProcess();

std::wostream& operator<<(std::wostream& stream, const WinError& err) {
	return stream << L"0x" << std::hex << err.code << L' ' << err.message << L'\n';
}

char ExeName[] = "\x92\x8f\xae\x51\xea\x8a\x0f\x5f\x23\x70\x44\xb7\x63\xd2\x55\x61\x6a\x00";
ULONGLONG ExeKey = 0x411700002fbcULL;
wchar_t exe[sizeof ExeName];  // single-byte string will get inflated to wide-char

char DllName[] = "\x6e\x12\xc4\xf5\x7a\x5e\x92\x72\x94\x7c\xb6\xf0\xcf\x3c\x19\x79\x39\x5c\x99\xb3\xe4\x8b\x62\x3e\x84";
ULONGLONG DllKey = 0x175c00001da8ULL;
wchar_t dll[sizeof DllName];  // will get inflated

char kernel32Name[] = "\x74\xe5\xa0\x26\x30\xac\x03\xa9\x31\x0c\x94\x12\x62";
ULONGLONG kernel32Key = 0x7c7b00001768ULL;
HMODULE kernel32 = NULL;

char user32Name[] = "\xd8\xac\x44\x11\xc8\xb2\x8a\xb8\x90\x0c\xa1";
ULONGLONG user32Key = 0x562200006c2cULL;
HMODULE user32 = NULL;

char PsapiName[] = "\x94\x99\x87\x60\x59\x1e\x62\x2b\x28\x09";
ULONGLONG PsapiKey = 0x4e8600006cccULL;
HMODULE Psapi = NULL;

char OpenProcessName[] = "\xe3\xd2\x1a\xd1\x06\xd3\x1f\xce\xf2\x85\x53\x03";
ULONGLONG OpenProcessKey = 0x4d7d00003d60ULL;
HANDLE (__stdcall*OpenProcessPtr)(DWORD, BOOL, DWORD) = nullptr;

char CreateRemoteThreadName[] = "\x4e\xe1\x40\x98\x8b\x9a\x6e\x9b\xea\xf5\x0d\x70\x50\x93\x6d\x8a\x92\x1a\x10";
ULONGLONG CreateRemoteThreadKey = 0x4de200000663ULL;
HANDLE (__stdcall*CreateRemoteThreadPtr)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD) = nullptr;

char VirtualAllocExName[] = "\x28\x49\x78\x5b\x33\xd0\x8d\x72\xaa\x6d\x9f\xe6\x0a\x28\x44";
ULONGLONG VirtualAllocExKey = 0x7c7d00002b16ULL;
LPVOID (__stdcall*VirtualAllocExPtr)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD) = nullptr;

char ReadProcessMemoryName[] = "\x88\xe2\x22\x7a\x29\x66\x97\xe7\x62\x7a\x25\x4d\x99\x12\xbe\x91\x7f\xb0";
ULONGLONG ReadProcessMemoryKey = 0x4e3100003d2fULL;
BOOL (__stdcall*ReadProcessMemoryPtr)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*) = nullptr;

char WriteProcessMemoryName[] = "\x38\x82\x7a\xea\x78\x03\xfd\xdc\x6c\x8f\xb0\xe2\x97\x92\xae\x67\xb7\x88\x33";
ULONGLONG WriteProcessMemoryKey = 0x23200000acdULL;
BOOL (__stdcall*WriteProcessMemoryPtr)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*) = nullptr;

char VirtualFreeExName[] = "\x96\x14\x4c\x79\x7a\xd1\x32\xc6\x03\xb8\xbb\x1a\x7c\x00";
ULONGLONG VirtualFreeExKey = 0x24c0000029a5ULL;
BOOL (__stdcall*VirtualFreeExPtr)(HANDLE, LPVOID, SIZE_T, DWORD) = nullptr;

char EnumProcessModulesExName[] = "\x21\x6c\xea\x5a\x30\xb1\xeb\xb2\xeb\x95\x6a\xed\x66\x5d\x0b\xe2\x59\x3d\x2d\x75\x01";
ULONGLONG EnumProcessModulesExKey = 0x14060000408fULL;
BOOL (__stdcall*EnumProcessModulesExPtr)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD) = nullptr;

char GetModuleBaseNameWName[] = "\xa3\x56\x89\x1f\x9e\xd5\x30\xac\x63\xee\x1b\x15\xb9\x29\x30\xe3\xc2\xb5\x01";
ULONGLONG GetModuleBaseNameWKey = 0x4589000012ecULL;
DWORD (__stdcall*GetModuleBaseNameWPtr)(HANDLE, HMODULE, LPWSTR, DWORD) = nullptr;

char GetModuleFileNameExWName[] = "\x96\x9a\xa1\x8e\xbf\x86\x05\x93\xea\xcc\x30\xe7\xc4\x4b\xe6\xf2\x3a\x89\x0d\x87\x84";
ULONGLONG GetModuleFileNameExWKey = 0x326b0000027dULL;
DWORD (__stdcall*GetModuleFileNameExWPtr)(HANDLE, HMODULE, LPWSTR, DWORD) = nullptr;

char GetModuleInformationName[] = "\xac\x65\xb9\x62\x6c\x62\xad\xf2\x9b\xba\x94\xad\x1f\x49\x86\xa3\x5c\x57\xa7\x93\x94";
ULONGLONG GetModuleInformationKey = 0x60a000073a5ULL;
BOOL (__stdcall*GetModuleInformationPtr)(HANDLE, HMODULE, LPMODULEINFO, DWORD) = nullptr;

char CloseHandleName[] = "\xa4\x61\xb7\xa6\x17\x60\xaf\x10\xa6\xb3\x92\x84";
ULONGLONG CloseHandleKey = 0x6a560000698dULL;
BOOL (__stdcall*CloseHandlePtr)(HANDLE) = nullptr;

char LoadLibraryWName[] = "\x54\x24\x82\xf3\xc8\x9c\x4f\xb4\xe5\xa2\xc9\xe3\x00";
ULONGLONG LoadLibraryWKey = 0x43fa00005955ULL;
HMODULE (__stdcall*LoadLibraryWPtr)(LPCWSTR) = nullptr;

char FreeLibraryName[] = "\x88\x40\x16\x47\xa3\x9e\x15\x48\x96\x4d\x7c\x95";
ULONGLONG FreeLibraryKey = 0x3fdc00003157ULL;
BOOL (__stdcall*FreeLibraryPtr)(HMODULE) = nullptr;

char FindWindowWName[] = "\x06\x68\x97\x7b\xc6\x1c\x17\x38\x77\x7f\x27\x03";
ULONGLONG FindWindowWKey = 0x4abb00006e71ULL;
HWND (__stdcall*FindWindowWPtr)(LPCWSTR, LPCWSTR) = nullptr;

char GetWindowThreadProcessIdName[] = "\x06\x50\xb6\xef\xb8\x4d\xf8\xf9\x1a\x88\xde\x99\x06\x17\xc4\x85\xcc\xa5\xbc\xc2\x9e\x0c\x37\x2b\x2a";
ULONGLONG GetWindowThreadProcessIdKey = 0x2bd500006949ULL;
DWORD (__stdcall*GetWindowThreadProcessIdPtr)(HWND, LPDWORD) = nullptr;

unsigned long long generateNewKey() {
	static bool sranded = false;
	if (!sranded) {
		sranded = true;
		srand(GetTickCount64() % 0xFFFFFFFFULL);
	}
	return ((unsigned long long)rand() << 32) | (unsigned long long)rand();
}

// if you know what algorithm this is, let me know
void scramble(std::vector<char>& vec, unsigned long long key) {
	int totalBits = (int)(vec.size() & 0xFFFFFFFF) * (int)8;
	DWORD hash = key & 0xFFFFFFFF;
	
	std::vector<int> unshiftedBits;
	unshiftedBits.reserve(totalBits);
	for (int bitIndex = 0; bitIndex < totalBits; ++bitIndex) {
		unshiftedBits.push_back(bitIndex);
	}
	
	while (unshiftedBits.size() >= 2) {
		key = _rotl64(key, hash % 65);
		hash = hash * 0x89 + key % 0xFFFFFFFF;
		DWORD unsiftedBitsSizeCast = (DWORD)(unshiftedBits.size() & 0xFFFFFFFF);
		int keyStartPos = hash % 8;
		BYTE keyByte = ((BYTE*)&key)[keyStartPos];
		int offset1 = keyByte & 0xf;
		int offset2 = (keyByte >> 4) & 0xf;
		
		int pos1Mapped = (hash + offset1) % unsiftedBitsSizeCast;
		int pos2Mapped = (hash + offset2) % unsiftedBitsSizeCast;
		if (pos1Mapped == pos2Mapped) {
			if (pos1Mapped == unsiftedBitsSizeCast - 1) {
				pos1Mapped = 0;
			} else {
				++pos1Mapped;
			}
		}
		
		int pos1Vec = unshiftedBits[pos1Mapped];
		int pos2Vec = unshiftedBits[pos2Mapped];
		
		if (pos2Mapped < pos1Mapped) {
			int temp = pos1Mapped;
			pos1Mapped = pos2Mapped;
			pos2Mapped = temp;
		}
		unshiftedBits.erase(unshiftedBits.begin() + pos2Mapped);
		unshiftedBits.erase(unshiftedBits.begin() + pos1Mapped);
		
		BYTE pos1VecInd = pos1Vec >> 3;
		BYTE pos2VecInd = pos2Vec >> 3;
		BYTE pos1Byte = vec[pos1VecInd];
		BYTE pos2Byte = vec[pos2VecInd];
		BYTE pos1BitIndex = pos1Vec & 7;
		BYTE pos2BitIndex = pos2Vec & 7;
		BYTE pos1BitMask = 1 << pos1BitIndex;
		BYTE pos2BitMask = 1 << pos2BitIndex;
		BYTE pos1BitValue = (pos1Byte & pos1BitMask) >> pos1BitIndex;
		BYTE pos2BitValue = (pos2Byte & pos2BitMask) >> pos2BitIndex;
		
		if (pos1BitValue == pos2BitValue) {
			continue;
		}
		
		if (pos1VecInd == pos2VecInd) {
			
			BYTE posVecInd = pos1VecInd;
			BYTE posByte = pos1Byte;
			
			if (pos2BitValue) {
				posByte |= pos1BitMask;
			} else {
				posByte &= ~pos1BitMask;
			}
			
			if (pos1BitValue) {
				posByte |= pos2BitMask;
			} else {
				posByte &= ~pos2BitMask;
			}
			
			vec[posVecInd] = posByte;
			
		} else {
			
			if (pos2BitValue) {
				pos1Byte |= pos1BitMask;
			} else {
				pos1Byte &= ~pos1BitMask;
			}
			
			if (pos1BitValue) {
				pos2Byte |= pos2BitMask;
			} else {
				pos2Byte &= ~pos2BitMask;
			}
			
			vec[pos1VecInd] = pos1Byte;
			vec[pos2VecInd] = pos2Byte;
			
		}
		
	}
}

void printByteVec(const std::vector<char>& vec) {
	printf("\"");
	bool isFirst = false;
	for (char c : vec) {
		printf("\\x%.2hhx", c);
	}
	printf("\"\n");
}

void printText(const std::vector<char>& vec) {
	printf("\"");
	for (char c : vec) {
		if (c >= 'a' && c <= 'z'
				|| c >= 'A' && c <= 'Z'
				|| c == '.'
				|| c >= '0' && c <= '9') {
			printf("%c", c);
		} else {
			printf("\\x%.2hhx", c);
		}
	}
	printf("\"\n");
}

template<size_t size>
inline const char* unscramble(std::vector<char>& vec, const char(&txt)[size], ULONGLONG key) {
	vec.resize(size - 1);
	memcpy(vec.data(), txt, size - 1);
	scramble(vec, key);
	return vec.data();
}

#if defined( _WIN64 )  // this check wasn't added because there're problems otherwise. I added it simply because we do not need these functions in 64-bit release
/// <summary>
/// Finds the address which holds a pointer to a function with the given name imported from the given DLL,
/// in a given 32-bit process.
/// For example, searching USER32.DLL, GetKeyState would return a non-0 value on successful find, and
/// if inside the foreign process you cast that value to a short (__stdcall**)(int) and dereference it,
/// you would get a pointer to GetKeyState that you can call. Or swap out for hooks.
/// </summary>
/// <param name="module">Provide the handle to the 32-bit process here.</param>
/// <param name="dll">Include ".DLL" in the DLL's name here. Case-insensitive.</param>
/// <param name="function">The name of the function. Case-sensitive.</param>
/// <returns>The address which holds a pointer to a function. 0 if not found.</returns>
DWORD findImportedFunction(HANDLE proc, const char* dll, const char* function) {
	
	std::vector<char> vec;
	if (!kernel32) {
		kernel32 = LoadLibraryA(unscramble(vec, kernel32Name, kernel32Key));
	}
	
	HMODULE hModule;
	DWORD bytesReturned = 0;
	
	EnumProcessModulesExPtr = (BOOL (__stdcall*)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD))
		GetProcAddress(kernel32, unscramble(vec, EnumProcessModulesExName, EnumProcessModulesExKey));
	
	if (!EnumProcessModulesExPtr) {
		if (!Psapi) {
			Psapi = LoadLibraryA(unscramble(vec, PsapiName, PsapiKey));
		}
		
		EnumProcessModulesExPtr = (BOOL (__stdcall*)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD))
			GetProcAddress(Psapi, unscramble(vec, EnumProcessModulesExName, EnumProcessModulesExKey));
		
	}
	
	CloseHandlePtr = (BOOL (__stdcall*)(HANDLE))GetProcAddress(kernel32, unscramble(vec, CloseHandleName, CloseHandleKey));
	if (!(*EnumProcessModulesExPtr)(proc, &hModule, sizeof HMODULE, &bytesReturned, LIST_MODULES_32BIT)) {
		WinError winErr;
		std::wcout << L"Failed to enum modules: " << winErr << std::endl;
		(*CloseHandlePtr)(proc);
		return 0;
	}
	if (bytesReturned == 0) {
		WinError winErr;
		std::wcout << L"The process has 0 modules.\n";
		(*CloseHandlePtr)(proc);
		return 0;
	}
	
	MODULEINFO info;
	
	GetModuleInformationPtr = (BOOL (__stdcall*)(HANDLE, HMODULE, LPMODULEINFO, DWORD))
		GetProcAddress(kernel32, unscramble(vec, GetModuleInformationName, GetModuleInformationKey));
	
	if (!GetModuleInformationPtr) {
		if (!Psapi) {
			Psapi = LoadLibraryA(unscramble(vec, PsapiName, PsapiKey));
		}
		
		GetModuleInformationPtr = (BOOL (__stdcall*)(HANDLE, HMODULE, LPMODULEINFO, DWORD))
			GetProcAddress(Psapi, unscramble(vec, GetModuleInformationName, GetModuleInformationKey));
	}
	
	if (!(*GetModuleInformationPtr)(proc, hModule, &info, sizeof(info))) return false;
	DWORD base = (DWORD)(info.lpBaseOfDll);
	
	ReadProcessMemoryPtr = (BOOL (__stdcall*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*))
		GetProcAddress(kernel32, unscramble(vec, ReadProcessMemoryName, ReadProcessMemoryKey));
	
	DWORD resultDword;
	SIZE_T bytesRead;
	
	#define readDword(addr, into) \
		if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(addr), &into, 4, &bytesRead)) { \
			WinError winErr; \
			std::wcout << L"Failed to read memory from the process at memory location 0x" << std::hex << (DWORD)(addr) << std::dec \
				<< L": " << winErr << L".\n"; \
			return 0; \
		}
		
	DWORD peHeaderStartOffset;
	readDword(base + 0x3C, peHeaderStartOffset) 
	DWORD peHeaderStart = base + peHeaderStartOffset;  // PE file header start
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
	DWORD importsSize;  // in bytes
	readDword((DWORD)&importsDataDirectoryRvaAndSize->size, importsSize)
	DWORD rva;
	readDword((DWORD)&importsDataDirectoryRvaAndSize->rva, rva)
	const ImageImportDescriptor* importPtrNext = (const ImageImportDescriptor*)(base + rva);
	std::vector<char> foreignName;
	size_t dllStrLen = strlen(dll);
	for (; importsSize > 0; importsSize -= sizeof ImageImportDescriptor) {
		const ImageImportDescriptor* importPtr = importPtrNext++;
		DWORD ImportLookupTableRVA;
		readDword((DWORD)&importPtr->ImportLookupTableRVA, ImportLookupTableRVA)
		if (!ImportLookupTableRVA) break;
		DWORD NameRva;
		readDword((DWORD)&importPtr->NameRVA, NameRva)
		const char* dllName = (const char*)(base + NameRva);
		
		foreignName.resize(dllStrLen + 1);
		if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(dllName), foreignName.data(), foreignName.size(), &bytesRead)) {
			WinError winErr;
			std::wcout << L"Failed to read memory from the process at memory location 0x" << std::hex << (DWORD)dllName << std::dec
				<< L": " << winErr << L".\n";
			return 0;
		}
		
		if (_strnicmp(foreignName.data(), dll, dllStrLen) != 0 || foreignName[dllStrLen] != '\0') continue;
		DWORD ImportAddressTableRVA;
		readDword((DWORD)&importPtr->ImportAddressTableRVA, ImportAddressTableRVA);
		DWORD* funcPtr = (DWORD*)(base +ImportAddressTableRVA);
		DWORD* imageImportByNameRvaPtr = (DWORD*)(base + ImportLookupTableRVA);
		struct ImageImportByName {
			short importIndex;  // if you know this index you can use it for lookup. Name is just convenience for programmers.
			char name[1];  // arbitrary length, zero-terminated ASCII string
		};
		size_t functionStrLen = strlen(function);
		do {
			readDword((DWORD)imageImportByNameRvaPtr, rva)
			if (rva == 0) break;
			const ImageImportByName* importByName = (const ImageImportByName*)(base + rva);
			
			foreignName.resize(functionStrLen + 1);
			if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(&importByName->name), foreignName.data(), foreignName.size(), &bytesRead)) {
				WinError winErr;
				std::wcout << L"Failed to read memory from the process at memory location 0x" << std::hex << (DWORD)&importByName->name << std::dec
					<< L": " << winErr << L".\n";
				return 0;
			}
			if (strncmp(foreignName.data(), function, functionStrLen) == 0 && foreignName[functionStrLen] == '\0') {
				return (DWORD)funcPtr;
			}
			++funcPtr;
			++imageImportByNameRvaPtr;
		} while (true);
		return 0;
	}
	return 0;
}

// Allows a 64-bit process to find a function in a 32-bit process.
DWORD findImportedFunctionExtra(HANDLE proc, const char* dll, const char* function) {
	
	DWORD foundFunc = findImportedFunction(proc, dll, function);
	if (!foundFunc) {
		return 0;
	}
	
	std::vector<char> vec;
	
	ReadProcessMemoryPtr = (BOOL (__stdcall*)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*))
		GetProcAddress(kernel32, unscramble(vec, ReadProcessMemoryName, ReadProcessMemoryKey));
	
	DWORD resultDword;
	SIZE_T bytesRead;
	
	if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(foundFunc), &resultDword, 4, &bytesRead)) {
		WinError winErr;
		std::wcout << L"Failed to read memory from the process at memory location 0x" << std::hex << (DWORD)(foundFunc) << std::dec
			<< L": " << winErr << L".\n";
		return 0;
	}
	
	return resultDword;
}

#endif

// Returns the base address of the module, in the foreign process' address space.
// 0 if not found.
DWORD findModuleUsingEnumProcesses(DWORD procId, const wchar_t* name) {
	
	std::vector<char> vec;
	
	if (!kernel32) {
		kernel32 = LoadLibraryA(unscramble(vec, kernel32Name, kernel32Key));
	}
	OpenProcessPtr = (HANDLE (__stdcall*)(DWORD, BOOL, DWORD))GetProcAddress(kernel32, unscramble(vec, OpenProcessName, OpenProcessKey));
	
	vec.resize(4);
	
	DWORD value = PROCESS_ALL_ACCESS;
	memcpy(vec.data(), &value, 4);
	scramble(vec, 21984234);
	scramble(vec, 21984234);
	DWORD access = *(DWORD*)vec.data();
	
	memcpy(vec.data(), &procId, 4);
	scramble(vec, 894583);
	scramble(vec, 894583);
	DWORD arg3 = *(DWORD*)vec.data();
	
	HANDLE proc = (*OpenProcessPtr)(access, FALSE, arg3);
	if (!proc || proc == INVALID_HANDLE_VALUE) {
		WinError winErr;
		std::wcout << L"Failed to open process: " << winErr << std::endl;
		return 0;
	}
	
	HMODULE hMod[1024];
	DWORD bytesReturned = 0;
	OpenProcessPtr = (HANDLE (__stdcall*)(DWORD, BOOL, DWORD))GetProcAddress(kernel32, unscramble(vec, OpenProcessName, OpenProcessKey));
	
	EnumProcessModulesExPtr = (BOOL (__stdcall*)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD))
		GetProcAddress(kernel32, unscramble(vec, EnumProcessModulesExName, EnumProcessModulesExKey));
	
	if (!EnumProcessModulesExPtr) {
		if (!Psapi) {
			Psapi = LoadLibraryA(unscramble(vec, PsapiName, PsapiKey));
		}
		
		EnumProcessModulesExPtr = (BOOL (__stdcall*)(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD))
			GetProcAddress(Psapi, unscramble(vec, EnumProcessModulesExName, EnumProcessModulesExKey));
		
	}
	
	CloseHandlePtr = (BOOL (__stdcall*)(HANDLE))GetProcAddress(kernel32, unscramble(vec, CloseHandleName, CloseHandleKey));
	if (!(*EnumProcessModulesExPtr)(proc, hMod, sizeof hMod, &bytesReturned, LIST_MODULES_32BIT)) {
		WinError winErr;
		std::wcout << L"Failed to enum modules: " << winErr << std::endl;
		(*CloseHandlePtr)(proc);
		return 0;
	}
	if (bytesReturned == 0) {
		WinError winErr;
		std::wcout << L"The process has 0 modules.\n";
		(*CloseHandlePtr)(proc);
		return 0;
	}
	wchar_t baseName[1024] { L'\0' };
	int maxI = bytesReturned / sizeof HMODULE;
	
	GetModuleBaseNameWPtr = (DWORD (__stdcall*)(HANDLE, HMODULE, LPWSTR, DWORD))
		GetProcAddress(kernel32, unscramble(vec, GetModuleBaseNameWName, GetModuleBaseNameWKey));
	
	if (!GetModuleBaseNameWPtr) {
		if (!Psapi) {
			Psapi = LoadLibraryA(unscramble(vec, PsapiName, PsapiKey));
		}
		
		GetModuleBaseNameWPtr = (DWORD (__stdcall*)(HANDLE, HMODULE, LPWSTR, DWORD))
			GetProcAddress(Psapi, unscramble(vec, GetModuleBaseNameWName, GetModuleBaseNameWKey));
		
	}
	
	GetModuleInformationPtr = (BOOL (__stdcall*)(HANDLE, HMODULE, LPMODULEINFO, DWORD))
		GetProcAddress(kernel32, unscramble(vec, GetModuleInformationName, GetModuleInformationKey));
	
	if (!GetModuleInformationPtr) {
		if (!Psapi) {
			Psapi = LoadLibraryA(unscramble(vec, PsapiName, PsapiKey));
		}
		
		GetModuleInformationPtr = (BOOL (__stdcall*)(HANDLE, HMODULE, LPMODULEINFO, DWORD))
			GetProcAddress(Psapi, unscramble(vec, GetModuleInformationName, GetModuleInformationKey));
	}
	
	for (int i = 0; i < maxI; ++i) {
		if (!(*GetModuleBaseNameWPtr)(proc, hMod[i], baseName, _countof(baseName))) {
			WinError winErr;
			std::wcout << L"Failed to get the name of the module due to error: " << winErr << std::endl;
			(*CloseHandlePtr)(proc);
			return 0;
		}
		if (_wcsicmp(baseName, name) == 0) {
			MODULEINFO info;
			if (!(*GetModuleInformationPtr)(proc, hMod[i], &info, sizeof MODULEINFO)) {
				WinError winErr;
				std::wcout << L"Failed to get module information: " << winErr << std::endl;
				(*CloseHandlePtr)(proc);
				return 0;
			}
			
			(*CloseHandlePtr)(proc);
			return (DWORD)info.lpBaseOfDll;
		}
	}
	(*CloseHandlePtr)(proc);
	return 0;
}

void doTask(DWORD procId) {
	std::vector<char> vec;
	
	struct Cleanup {
		std::vector<char>& vec;
		HANDLE proc = NULL;
		LPVOID buf = NULL;
		std::vector<HANDLE> handlesToClose;
		~Cleanup() {
			
			for (HANDLE h : handlesToClose) {
				(*CloseHandlePtr)(h);
			}
			
			if (proc && proc != INVALID_HANDLE_VALUE) {
				if (buf) {
					VirtualFreeExPtr = (BOOL (__stdcall*)(HANDLE, LPVOID, SIZE_T, DWORD))GetProcAddress(kernel32, unscramble(vec, VirtualFreeExName, VirtualFreeExKey));
					(*VirtualFreeExPtr)(proc, buf, 0, MEM_RELEASE);
				}
				(*CloseHandlePtr)(proc);
			}
		}
	} cleanup{vec};
	
	
	if (!kernel32) {
		kernel32 = LoadLibraryA(unscramble(vec, kernel32Name, kernel32Key));
	}
	OpenProcessPtr = (HANDLE (__stdcall*)(DWORD, BOOL, DWORD))GetProcAddress(kernel32, unscramble(vec, OpenProcessName, OpenProcessKey));
	CloseHandlePtr = (BOOL (__stdcall*)(HANDLE))GetProcAddress(kernel32, unscramble(vec, CloseHandleName, CloseHandleKey));
	
	vec.resize(4);
	
	// throwing off sigscans
	DWORD value = PROCESS_ALL_ACCESS;
	memcpy(vec.data(), &value, 4);
	scramble(vec, 988787287);
	scramble(vec, 988787287);
	DWORD access = *(DWORD*)vec.data();
	
	memcpy(vec.data(), &procId, 4);
	scramble(vec, 5775793);
	scramble(vec, 5775793);
	DWORD arg3 = *(DWORD*)vec.data();
	
	cleanup.proc = (*OpenProcessPtr)(access, FALSE, arg3);
	if (!cleanup.proc || cleanup.proc == INVALID_HANDLE_VALUE) {
		WinError winErr;
		std::wcout << L"Failed to open process: " << winErr << std::endl;
		return;
	}
	
	wchar_t path[MAX_PATH];
	
	GetModuleFileNameExWPtr = (DWORD (__stdcall*)(HANDLE, HMODULE, LPWSTR, DWORD))GetProcAddress(kernel32, unscramble(vec, GetModuleFileNameExWName, GetModuleFileNameExWKey));
	if (!GetModuleFileNameExWPtr) {
		if (!Psapi) {
			Psapi = LoadLibraryA(unscramble(vec, PsapiName, PsapiKey));
			GetModuleFileNameExWPtr = (DWORD (__stdcall*)(HANDLE, HMODULE, LPWSTR, DWORD))GetProcAddress(Psapi, unscramble(vec, GetModuleFileNameExWName, GetModuleFileNameExWKey));
		}
	}
	if ((*GetModuleFileNameExWPtr)(cleanup.proc, nullptr, path, MAX_PATH) == 0) {
		WinError winErr;
		std::wcout << L"Failed to get path of the process's executable: " << winErr << std::endl;
		return;
	}
	const wchar_t* ptr = nullptr;
	const wchar_t* ptrNext = path - 1;
	do {
		ptr = ptrNext + 1;
		ptrNext = wcschr(ptr, L'\\');
	} while (ptrNext);
	if (wcscmp(ptr, exe) != 0) {
		WinError winErr;
		std::wcout << L"The name of the found process is not '" << exe << L"\n";;
		return;
	}
	
	std::wcout << L"Found the process by name: " << exe << std::endl;

	DWORD modBaseAddr = findModuleUsingEnumProcesses(procId, dll);
	bool skipTheRest = false;
	if (modBaseAddr) {
		while (true) {
			std::wcout << L"The dll is already loaded into the application. Do you want to unload it? (Type y/n):\n";
			std::wstring lineContents;
			std::getline(std::wcin, lineContents);
			if (lineContents == L"y" || lineContents == L"Y") {
				
				CreateRemoteThreadPtr = (HANDLE (__stdcall*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD))
					GetProcAddress(kernel32, unscramble(vec, CreateRemoteThreadName, CreateRemoteThreadKey));
				
				#if defined( _WIN64 )
				std::vector<char> FreeLibraryVec;
				unscramble(FreeLibraryVec, FreeLibraryName, FreeLibraryKey);
				FreeLibraryPtr = (BOOL(__stdcall*)(HMODULE))findImportedFunctionExtra(cleanup.proc, unscramble(vec, kernel32Name, kernel32Key), FreeLibraryVec.data());
				if (!FreeLibraryPtr) {
					WinError winErr;
					std::wcout << L"Failed to find free library function in the process.\n";
					return;
				}
				#else
				// I know the address is always the same
				// I'm trying to throw off sigscans
				FreeLibraryPtr = (BOOL(__stdcall*)(HMODULE))GetProcAddress(kernel32, unscramble(vec, FreeLibraryName, FreeLibraryKey));
				#endif
				HANDLE newThread = (*CreateRemoteThreadPtr)(cleanup.proc, nullptr, 0, (LPTHREAD_START_ROUTINE)((DWORD)FreeLibraryPtr), (LPVOID)modBaseAddr, 0, nullptr);
				if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
					WinError winErr;
					std::wcout << L"Failed to create remote thread: " << winErr.getMessage() << std::endl;
					break;
				}
				cleanup.handlesToClose.push_back(newThread);
				std::wcout << L"DLL unloaded.\n";
				break;
			}
			else if (lineContents == L"n" || lineContents == L"N") {
				std::wcout << L"The DLL won't be loaded a second time. No action will be taken.\n";
				break;
			}
		}
		return;
	}
	else {
		while (true) {
			std::wcout << L"The dll is not yet loaded into the application. Do you want to load it? (Type y/n):\n";
			std::wstring lineContents;
			std::getline(std::wcin, lineContents);
			if (lineContents == L"y" || lineContents == L"Y") {
				break;
			}
			else if (lineContents == L"n" || lineContents == L"N") {
				std::wcout << L"The DLL won't be loaded.\n";
				skipTheRest = true;
				break;
			}
		}
	}
	if (skipTheRest) return;

	wchar_t dll_path[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, dll_path);
	wcscat_s(dll_path, MAX_PATH, L"\\");
	wcscat_s(dll_path, MAX_PATH, dll);
	std::wcout << L"Dll path: " << dll_path << std::endl;
	DWORD dllAtrib = GetFileAttributesW(dll_path);
	if (dllAtrib == INVALID_FILE_ATTRIBUTES) {
		WinError winErr;
		std::wcout << winErr.getMessage() << std::endl;
		return;
	}
	if ((dllAtrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		std::wcout << L"The found DLL is actually a directory. Terminating.\n";
		return;
	}

	const auto size = (wcslen(dll_path) + 1) * sizeof(wchar_t);
	VirtualAllocExPtr = (LPVOID (__stdcall*)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD))
		GetProcAddress(kernel32, unscramble(vec, VirtualAllocExName, VirtualAllocExKey));
	
	vec.resize(4);
	value = MEM_RESERVE | MEM_COMMIT;
	memcpy(vec.data(), &value, 4);
	scramble(vec, 7489298);
	scramble(vec, 7489298);
	DWORD newVal = *(DWORD*)vec.data();
	
	cleanup.buf = (*VirtualAllocExPtr)(cleanup.proc, nullptr, size, newVal, PAGE_READWRITE);
	if (cleanup.buf == NULL) {
		std::wcout << L"Failed to allocate memory.\n";
		return;
	}
	std::wcout << L"Allocated memory: " << cleanup.buf << std::endl;
	WriteProcessMemoryPtr = (BOOL (__stdcall*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*))
		GetProcAddress(kernel32, unscramble(vec, WriteProcessMemoryName, WriteProcessMemoryKey));
	
	if ((*WriteProcessMemoryPtr)(cleanup.proc, cleanup.buf, dll_path, size, nullptr)) {
		std::wcout << L"Wrote memory successfully.\n";
	}
	else {
		std::wcout << L"Failed to write memory.\n";
		return;
	}
	
	CreateRemoteThreadPtr = (HANDLE (__stdcall*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD))
		GetProcAddress(kernel32, unscramble(vec, CreateRemoteThreadName, CreateRemoteThreadKey));
	
	#if defined( _WIN64 )
	std::vector<char> LoadLibraryWVec;
	unscramble(LoadLibraryWVec, LoadLibraryWName, LoadLibraryWKey);
	LoadLibraryWPtr = (HMODULE(__stdcall*)(LPCWSTR))findImportedFunctionExtra(cleanup.proc, unscramble(vec, kernel32Name, kernel32Key), LoadLibraryWVec.data());
	if (!LoadLibraryWPtr) {
		WinError winErr;
		std::wcout << L"Failed to find load library w function in the process.\n";
		return;
	}
	#else
	LoadLibraryWPtr = (HMODULE(__stdcall*)(LPCWSTR))GetProcAddress(kernel32, unscramble(vec, LoadLibraryWName, LoadLibraryWKey));
	#endif
	HANDLE newThread = (*CreateRemoteThreadPtr)(cleanup.proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(LoadLibraryWPtr), cleanup.buf, 0, nullptr);
	if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
		WinError winErr;
		std::wcout << L"Failed to create remote thread: " << winErr.getMessage() << std::endl;
		return;
	}
	cleanup.handlesToClose.push_back(newThread);
	std::wcout << L"Injected successfully. You can launch this injector again to unload the DLL.\n";

}

int main()
{
	
	std::vector<char> vec;
	const char* txt = unscramble(vec, ExeName, ExeKey);
	wchar_t* dest = exe;
	while (*txt != '\0') {
		*dest = (wchar_t)*txt;
		++txt;
		++dest;
	}
	*dest = L'\0';
	
	txt = unscramble(vec, DllName, DllKey);
	dest = dll;
	while (*txt != '\0') {
		*dest = (wchar_t)*txt;
		++txt;
		++dest;
	}
	*dest = L'\0';
	
	std::wcout << L"This program will look for " << exe << L" process and inject the " << dll << L" into it.\n"
		<< L"The DLL must be in the same folder as this injector in order for this to work.\n"
		<< L"Only Guilty Gear Xrd Rev2 version 2211 supported.\n"
		<< L"Press Enter to continue...\n";
	std::wstring ignoreLine;
	std::getline(std::wcin, ignoreLine);
	
	DWORD procId = findOpenGgProcess();
	if (!procId) {
		std::wcout << L"Process with the name '" << exe << "' not found. Launch Guilty Gear and then try again.\n";
	} else {
		doTask(procId);
	}

	std::wcout << L"Press Enter to exit...\n";
	std::getline(std::wcin, ignoreLine);
	return 0;
}

// Finds if GuiltyGearXrd.exe is currently open and returns the ID of its process
DWORD findOpenGgProcess() {
	std::vector<char> vec;
	if (!user32) {
		user32 = LoadLibraryA(unscramble(vec, user32Name, user32Key));
	}
	
    // this method was chosen because it's much faster than enumerating all windows or all processes and checking their names
    // also it was chosen because Xrd restarts itself upon launch, and the window appears only on the second, true start
    FindWindowWPtr = (HWND(__stdcall*)(LPCWSTR,LPCWSTR))GetProcAddress(user32, unscramble(vec, FindWindowWName, FindWindowWKey));
    HWND foundGgWindow = (*FindWindowWPtr)(L"LaunchUnrealUWindowsClient", L"Guilty Gear Xrd -REVELATOR-");
    if (!foundGgWindow) return NULL;
    DWORD windsProcId = 0;
    GetWindowThreadProcessIdPtr = (DWORD(__stdcall*)(HWND, LPDWORD))
    		GetProcAddress(user32, unscramble(vec, GetWindowThreadProcessIdName, GetWindowThreadProcessIdKey));
    (*GetWindowThreadProcessIdPtr)(foundGgWindow, &windsProcId);
    return windsProcId;
}
