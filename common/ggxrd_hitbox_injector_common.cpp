#include "pch.h"
#include "ggxrd_hitbox_injector_common.h"
#include "Psapi.h"
#include <string>
#include "WError.h"
#include "InjectorCommonOut.h"
#include "const_obfuscate.h"

InjectorCommonOut outputObject;
extern void __cdecl GetLine(std::wstring& line);
extern bool force;
extern void pressAnyKeyBegin();
extern bool isAnyKeyPressed();
extern void pressAnyKeyEnd();

inline InjectorCommonOut& operator<<(InjectorCommonOut& stream, const WinError& err) {
	return stream << L"0x" << std::hex << err.code << L' ' << err.message << L'\n';
}
DWORD findOpenGgProcess();

#undef OBF_FUNC
#define OBF_FUNC(name) name##Ptr
#define OBF_FUNC_DOUBLE(name) OBF_FUNC(name)

#define OBF_LOAD(module) \
	if (!module) { \
		module = LoadLibraryA(OBF_DATA(#module)); \
	} \
	if (!module) ExitProcess(1);

#undef OBF_IMPORT
#define OBF_IMPORT(module, func) OBF_FUNC(func) = reinterpret_cast<decltype(&func)>(GetProcAddress(module, OBF_DATA(#func)));
#define OBF_IMPORT_WITH_NAME(module, func, funcName) OBF_FUNC(func) = reinterpret_cast<decltype(&func)>(GetProcAddress(module, OBF_DATA(funcName)));

#define OBF_VALUE(type, name, val) \
	volatile const type name##ar[1] { val }; \
	type name = *const_obfuscate::deobfuscate(const_obfuscate::deobfuscate(name##ar, __LINE__).data, __LINE__).data;

wchar_t exe[18];
wchar_t dll[25];
HMODULE kernel32 = NULL;
HMODULE user32 = NULL;
HMODULE Psapi = NULL;
OBF_IMPORT_DECL(OpenProcess);
OBF_IMPORT_DECL(CreateRemoteThread);
OBF_IMPORT_DECL(VirtualAllocEx);
OBF_IMPORT_DECL(ReadProcessMemory);
OBF_IMPORT_DECL(WriteProcessMemory);
OBF_IMPORT_DECL(VirtualFreeEx);
OBF_IMPORT_DECL(EnumProcessModulesEx);
OBF_IMPORT_DECL(GetModuleBaseNameW);
OBF_IMPORT_DECL(GetModuleFileNameExW);
OBF_IMPORT_DECL(GetModuleInformation);
OBF_IMPORT_DECL(CloseHandle);
OBF_IMPORT_DECL(LoadLibraryW);
OBF_IMPORT_DECL(FreeLibrary);
OBF_IMPORT_DECL(FindWindowW);
OBF_IMPORT_DECL(GetWindowThreadProcessId);
OBF_IMPORT_DECL(WaitForSingleObject);
OBF_IMPORT_DECL(GetExitCodeThread);

HANDLE openProcessAllAccess(DWORD procId) {
	
	OBF_LOAD(kernel32);
	OBF_IMPORT(kernel32, OpenProcess);
	
	OBF_VALUE(DWORD, access, PROCESS_ALL_ACCESS)
	
	OBF_VALUE(DWORD, arg3, procId)
	
	return (*OBF_FUNC(OpenProcess))(access, FALSE, arg3);
}

#if defined( _WIN64 )  // this check wasn't added because there're problems otherwise. I added it simply because we do not need these functions in 32-bit release
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
	
	OBF_LOAD(kernel32);
	
	HMODULE hModule;
	DWORD bytesReturned = 0;
	
	OBF_IMPORT_WITH_NAME(kernel32, EnumProcessModulesEx, "EnumProcessModulesEx");
	
	// see comment in findModuleUsingEnumProcesses for explanation of why we're using EnumProcessModules over CreateToolhelp32Snapshot.
	if (!OBF_FUNC_DOUBLE(EnumProcessModulesEx)) {
		OBF_LOAD(Psapi);
		OBF_IMPORT_WITH_NAME(Psapi, EnumProcessModulesEx, "EnumProcessModulesEx");
	}
	
	OBF_IMPORT(kernel32, CloseHandle);
	if (!(*OBF_FUNC_DOUBLE(EnumProcessModulesEx))(proc, &hModule, sizeof HMODULE, &bytesReturned, LIST_MODULES_32BIT)) {
		WinError winErr;
		outputObject << L"Failed to enum modules: " << winErr << std::endl;
		(*CloseHandlePtr)(proc);
		return 0;
	}
	if (bytesReturned == 0) {
		WinError winErr;
		outputObject << L"The process has 0 modules.\n";
		(*CloseHandlePtr)(proc);
		return 0;
	}
	
	MODULEINFO info;
	
	OBF_IMPORT_WITH_NAME(kernel32, GetModuleInformation, "GetModuleInformation");
	
	if (!OBF_FUNC_DOUBLE(GetModuleInformation)) {
		OBF_LOAD(Psapi);
		OBF_IMPORT_WITH_NAME(Psapi, GetModuleInformation, "GetModuleInformation");
	}
	
	if (!(*OBF_FUNC_DOUBLE(GetModuleInformation))(proc, hModule, &info, sizeof(info))) return false;
	DWORD base = (DWORD)(uintptr_t)info.lpBaseOfDll;
	
	OBF_IMPORT(kernel32, ReadProcessMemory);
	
	SIZE_T bytesRead;
	
	#define readDword(addr, into) \
		if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)((uintptr_t)addr), &into, 4, &bytesRead)) { \
			WinError winErr; \
			outputObject << L"Failed to read memory from the process at memory location 0x" << std::hex << (DWORD)(addr) << std::dec \
				<< L": " << winErr << L".\n"; \
			return 0; \
		}
		
	DWORD peHeaderStartOffset;
	readDword(base + 0x3C, peHeaderStartOffset) 
	DWORD peHeaderStart = base + peHeaderStartOffset;  // PE file header start
	// see IMAGE_BASE_RELOCATION
	struct RvaAndSize {
		DWORD rva;
		DWORD size;
	};
	const RvaAndSize* importsDataDirectoryRvaAndSize = (const RvaAndSize*)(uintptr_t)(peHeaderStart + 0x80);
	// see IMAGE_IMPORT_DESCRIPTOR
	struct ImageImportDescriptor {
		DWORD ImportLookupTableRVA;  // The RVA of the import lookup table. This table contains a name or ordinal for each import. (The name "Characteristics" is used in Winnt.h, but no longer describes this field.)
		DWORD TimeDateStamp;  // The stamp that is set to zero until the image is bound. After the image is bound, this field is set to the time/data stamp of the DLL. LIES, this field is 0 for me at runtime.
		DWORD ForwarderChain;  // The index of the first forwarder reference. 0 for me.
		DWORD NameRVA;  // The address of an ASCII string that contains the name of the DLL. This address is relative to the image base.
		DWORD ImportAddressTableRVA;  // The RVA of the import address table. The contents of this table are identical to the contents of the import lookup table until the image is bound.
	};
	DWORD importsSize;  // in bytes
	readDword((DWORD)(uintptr_t)&importsDataDirectoryRvaAndSize->size, importsSize)
	DWORD rva;
	readDword((DWORD)(uintptr_t)&importsDataDirectoryRvaAndSize->rva, rva)
	const ImageImportDescriptor* importPtrNext = (const ImageImportDescriptor*)(uintptr_t)(base + rva);
	std::vector<char> foreignName;
	size_t dllStrLen = strlen(dll);
	for (; importsSize > 0; importsSize -= sizeof ImageImportDescriptor) {
		const ImageImportDescriptor* importPtr = importPtrNext++;
		DWORD ImportLookupTableRVA;
		readDword((DWORD)(uintptr_t)&importPtr->ImportLookupTableRVA, ImportLookupTableRVA)
		if (!ImportLookupTableRVA) break;
		DWORD NameRva;
		readDword((DWORD)(uintptr_t)&importPtr->NameRVA, NameRva)
		const char* dllName = (const char*)(uintptr_t)(base + NameRva);
		
		foreignName.resize(dllStrLen + 1);
		if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(dllName), foreignName.data(), foreignName.size(), &bytesRead)) {
			WinError winErr;
			outputObject << L"Failed to read memory from the process at memory location 0x" << std::hex
				<< (DWORD)(uintptr_t)dllName << std::dec
				<< L": " << winErr << L".\n";
			return 0;
		}
		
		if (_strnicmp(foreignName.data(), dll, dllStrLen) != 0 || foreignName[dllStrLen] != '\0') continue;
		DWORD ImportAddressTableRVA;
		readDword((DWORD)(uintptr_t)&importPtr->ImportAddressTableRVA, ImportAddressTableRVA);
		DWORD* funcPtr = (DWORD*)(uintptr_t)(base +ImportAddressTableRVA);
		DWORD* imageImportByNameRvaPtr = (DWORD*)(uintptr_t)(base + ImportLookupTableRVA);
		struct ImageImportByName {
			short importIndex;  // if you know this index you can use it for lookup. Name is just convenience for programmers.
			char name[1];  // arbitrary length, zero-terminated ASCII string
		};
		size_t functionStrLen = strlen(function);
		do {
			readDword((DWORD)(uintptr_t)imageImportByNameRvaPtr, rva)
			if (rva == 0) break;
			const ImageImportByName* importByName = (const ImageImportByName*)(uintptr_t)(base + rva);
			
			foreignName.resize(functionStrLen + 1);
			if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(&importByName->name), foreignName.data(), foreignName.size(), &bytesRead)) {
				WinError winErr;
				outputObject << L"Failed to read memory from the process at memory location 0x" << std::hex
					<< (DWORD)(uintptr_t)&importByName->name << std::dec
					<< L": " << winErr << L".\n";
				return 0;
			}
			if (strncmp(foreignName.data(), function, functionStrLen) == 0 && foreignName[functionStrLen] == '\0') {
				return (DWORD)(uintptr_t)funcPtr;
			}
			++funcPtr;
			++imageImportByNameRvaPtr;
		} while (true);
		return 0;
	}
	return 0;
}

// Allows a 64-bit process to find a function in a 32-bit process.
// For description, see findImportedFunction.
// What this does, in addition to findImportedFunction, is read the function pointer-to-pointer to return the function pointer.
DWORD findImportedFunctionExtra(HANDLE proc, const char* dll, const char* function) {
	
	DWORD foundFunc = findImportedFunction(proc, dll, function);
	if (!foundFunc) {
		return 0;
	}
	
	OBF_IMPORT(kernel32, ReadProcessMemory);
	
	DWORD resultDword;
	SIZE_T bytesRead;
	
	if (!(*ReadProcessMemoryPtr)(proc, (LPCVOID)(uintptr_t)foundFunc, &resultDword, 4, &bytesRead)) {
		WinError winErr;
		outputObject << L"Failed to read memory from the process at memory location 0x" << std::hex << (DWORD)foundFunc << std::dec
			<< L": " << winErr << L".\n";
		return 0;
	}
	
	return resultDword;
}

#endif

// Returns the base address of the module, in the foreign process' address space.
// 0 if not found.
DWORD findModuleUsingEnumProcesses(DWORD procId, const wchar_t* name) {
	
	OBF_LOAD(kernel32);
	
	HANDLE proc = openProcessAllAccess(procId);
	if (!proc || proc == INVALID_HANDLE_VALUE) {
		WinError winErr;
		outputObject << L"Failed to open process: " << winErr << std::endl;
		return 0;
	}
	
	HMODULE hMod[1024];
	DWORD bytesReturned = 0;
	
	OBF_IMPORT_WITH_NAME(kernel32, EnumProcessModulesEx, "EnumProcessModulesEx");
	
	// we're going to use EnumProcessModules, because CreateToolhelp32Snapshot with TH32CS_SNAPMODULE/TH32CS_SNAPMODULE32
	// doesn't work under Linux under Wine under Steam Proton. It returns some specific error code that I forgot.
	// EnumProcessModules, on the other hand, works fine in that environment.
	if (!OBF_FUNC_DOUBLE(EnumProcessModulesEx)) {
		OBF_LOAD(Psapi);
		OBF_IMPORT_WITH_NAME(Psapi, EnumProcessModulesEx, "EnumProcessModulesEx");
	}
	
	OBF_IMPORT(kernel32, CloseHandle);
	if (!(*OBF_FUNC_DOUBLE(EnumProcessModulesEx))(proc, hMod, sizeof hMod, &bytesReturned, LIST_MODULES_32BIT)) {
		WinError winErr;
		outputObject << L"Failed to enum modules: " << winErr << std::endl;
		(*CloseHandlePtr)(proc);
		return 0;
	}
	if (bytesReturned == 0) {
		WinError winErr;
		outputObject << L"The process has 0 modules.\n";
		(*CloseHandlePtr)(proc);
		return 0;
	}
	wchar_t baseName[1024] { L'\0' };
	int maxI = bytesReturned / sizeof HMODULE;
	
	OBF_IMPORT_WITH_NAME(kernel32, GetModuleBaseNameW, "GetModuleBaseNameW");
	if (!OBF_FUNC_DOUBLE(GetModuleBaseNameW)) {
		OBF_LOAD(Psapi);
		OBF_IMPORT_WITH_NAME(Psapi, GetModuleBaseNameW, "GetModuleBaseNameW");
	}
	
	OBF_IMPORT_WITH_NAME(kernel32, GetModuleInformation, "GetModuleInformation");
	if (!OBF_FUNC_DOUBLE(GetModuleInformation)) {
		OBF_LOAD(Psapi);
		OBF_IMPORT_WITH_NAME(Psapi, GetModuleInformation, "GetModuleInformation");
	}
	
	for (int i = 0; i < maxI; ++i) {
		if (!(*OBF_FUNC_DOUBLE(GetModuleBaseNameW))(proc, hMod[i], baseName, _countof(baseName))) {
			WinError winErr;
			outputObject << L"Failed to get the name of the module due to error: " << winErr << std::endl;
			(*CloseHandlePtr)(proc);
			return 0;
		}
		if (_wcsicmp(baseName, name) == 0) {
			MODULEINFO info;
			if (!(*OBF_FUNC_DOUBLE(GetModuleInformation))(proc, hMod[i], &info, sizeof MODULEINFO)) {
				WinError winErr;
				outputObject << L"Failed to get module information: " << winErr << std::endl;
				(*CloseHandlePtr)(proc);
				return 0;
			}
			
			(*CloseHandlePtr)(proc);
			return (DWORD)(uintptr_t)info.lpBaseOfDll;
		}
	}
	(*CloseHandlePtr)(proc);
	return 0;
}

bool injectorTask(DWORD procId) {
	
	struct Cleanup {
		HANDLE proc = NULL;
		LPVOID buf = NULL;
		std::vector<HANDLE> handlesToClose;
		~Cleanup() {
			
			for (HANDLE h : handlesToClose) {
				(*CloseHandlePtr)(h);
			}
			
			if (proc && proc != INVALID_HANDLE_VALUE) {
				if (buf) {
					OBF_IMPORT(kernel32, VirtualFreeEx);
					(*VirtualFreeExPtr)(proc, buf, 0, MEM_RELEASE);
					outputObject << L"Freed memory: " << buf << std::endl;
				}
				(*CloseHandlePtr)(proc);
			}
		}
	} cleanup;
	
	
	OBF_LOAD(kernel32);
	OBF_IMPORT(kernel32, CloseHandle);
	
	cleanup.proc = openProcessAllAccess(procId);
	if (!cleanup.proc || cleanup.proc == INVALID_HANDLE_VALUE) {
		WinError winErr;
		outputObject << L"Failed to open process: " << winErr << std::endl;
		return false;
	}
	
	wchar_t path[MAX_PATH];
	
	OBF_IMPORT_WITH_NAME(kernel32, GetModuleFileNameExW, "GetModuleFileNameExW");
	if (!OBF_FUNC_DOUBLE(GetModuleFileNameExW)) {
		OBF_LOAD(Psapi);
		OBF_IMPORT_WITH_NAME(Psapi, GetModuleFileNameExW, "GetModuleFileNameExW");
	}
	if ((*OBF_FUNC_DOUBLE(GetModuleFileNameExW))(cleanup.proc, nullptr, path, MAX_PATH) == 0) {
		WinError winErr;
		outputObject << L"Failed to get path of the process's executable: " << winErr << std::endl;
		return false;
	}
	const wchar_t* ptr = nullptr;
	const wchar_t* ptrNext = path - 1;
	do {
		ptr = ptrNext + 1;
		ptrNext = wcschr(ptr, L'\\');
	} while (ptrNext);
	
	if (wcscmp(ptr, exe) != 0) {
		WinError winErr;
		outputObject << L"The name of the found process is not '" << exe << L"'\n";;
		return false;
	}
	
	outputObject << L"The name of the process: " << exe << std::endl;

	DWORD modBaseAddr = findModuleUsingEnumProcesses(procId, dll);
	if (modBaseAddr) {
		while (true) {
			outputObject << L"The dll is already loaded into the application. Do you want to unload it? (Type y/n):\n";
			std::wstring lineContents;
			if (force) {
				outputObject << L"Force Y\n";
				lineContents = L"y";
			} else {
				GetLine(lineContents);
			}
			if (lineContents == L"y" || lineContents == L"Y") {
				
				OBF_IMPORT(kernel32, CreateRemoteThread);
				
				DWORD remoteFreeLibrary;
				#if defined( _WIN64 )
				remoteFreeLibrary = findImportedFunctionExtra(cleanup.proc, OBF_DATA("kernel32.dll"), OBF_DATA("FreeLibrary"));
				if (!remoteFreeLibrary) {
					WinError winErr;
					outputObject << L"Failed to find free library function in the process.\n";
					return false;
				}
				#else
				// I know the address is always the same
				// I'm trying to throw off sigscans
				OBF_IMPORT(kernel32, FreeLibrary);
				remoteFreeLibrary = (DWORD)(uintptr_t)&FreeLibrary;
				#endif
				HANDLE newThread = (*CreateRemoteThreadPtr)(cleanup.proc, nullptr, 0,
					(LPTHREAD_START_ROUTINE)(uintptr_t)remoteFreeLibrary, (LPVOID)(uintptr_t)modBaseAddr, 0, nullptr);
				if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
					WinError winErr;
					outputObject << L"Failed to create remote thread: " << winErr.getMessage() << std::endl;
					return false;
				}
				cleanup.handlesToClose.push_back(newThread);
				outputObject << L"DLL unloaded.\n";
				return true;
			}
			else if (lineContents == L"n" || lineContents == L"N") {
				outputObject << L"The DLL won't be loaded a second time. No action will be taken.\n";
				return true;
			}
		}
		return true;
	}
	else {
		while (true) {
			outputObject << L"The dll is not yet loaded into the application. Do you want to load it? (Type y/n):\n";
			std::wstring lineContents;
			if (force) {
				outputObject << L"Force Y\n";
				lineContents = L"y";
			} else {
				GetLine(lineContents);
			}
			if (lineContents == L"y" || lineContents == L"Y") {
				break;
			}
			else if (lineContents == L"n" || lineContents == L"N") {
				outputObject << L"The DLL won't be loaded.\n";
				return true;
			}
		}
	}

	wchar_t dll_path[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, dll_path);
	wcscat_s(dll_path, MAX_PATH, L"\\");
	wcscat_s(dll_path, MAX_PATH, dll);
	outputObject << L"Dll path: " << dll_path << std::endl;
	DWORD dllAtrib = GetFileAttributesW(dll_path);
	if (dllAtrib == INVALID_FILE_ATTRIBUTES) {
		WinError winErr;
		outputObject << winErr.getMessage() << std::endl;
		return false;
	}
	if ((dllAtrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		outputObject << L"The found DLL is actually a directory. Terminating.\n";
		return false;
	}

	const auto size = (wcslen(dll_path) + 1) * sizeof(wchar_t);
	OBF_IMPORT(kernel32, VirtualAllocEx);
	
	OBF_VALUE(DWORD, newVal, MEM_RESERVE | MEM_COMMIT)
	
	cleanup.buf = (*VirtualAllocExPtr)(cleanup.proc, nullptr, size, newVal, PAGE_READWRITE);
	if (cleanup.buf == NULL) {
		outputObject << L"Failed to allocate memory.\n";
		return false;
	}
	outputObject << L"Allocated memory: " << cleanup.buf << std::endl;
	OBF_IMPORT(kernel32, WriteProcessMemory);
	
	if ((*WriteProcessMemoryPtr)(cleanup.proc, cleanup.buf, dll_path, size, nullptr)) {
		outputObject << L"Wrote memory successfully.\n";
	}
	else {
		outputObject << L"Failed to write memory.\n";
		return false;
	}
	
	OBF_IMPORT(kernel32, CreateRemoteThread);
	
	DWORD remoteLoadLibrary;
	#if defined( _WIN64 )
	remoteLoadLibrary = findImportedFunctionExtra(cleanup.proc, OBF_DATA("kernel32.dll"), OBF_DATA("LoadLibraryW"));
	if (!remoteLoadLibrary) {
		WinError winErr;
		outputObject << L"Failed to find load library w function in the process.\n";
		return false;
	}
	#else
	OBF_IMPORT(kernel32, LoadLibraryW);
	remoteLoadLibrary = (DWORD)(uintptr_t)&LoadLibraryW;
	#endif
	HANDLE newThread = (*CreateRemoteThreadPtr)(cleanup.proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(uintptr_t)remoteLoadLibrary, cleanup.buf, 0, nullptr);
	if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
		WinError winErr;
		outputObject << L"Failed to create remote thread: " << winErr.getMessage() << std::endl;
		return false;
	}
	cleanup.handlesToClose.push_back(newThread);
	
	outputObject << L"Waiting for injection to finish...\n";
	OBF_IMPORT(kernel32, WaitForSingleObject);
	DWORD waitResult = (*WaitForSingleObjectPtr)(newThread, INFINITE);
	if (waitResult == WAIT_OBJECT_0) {
		OBF_IMPORT(kernel32, GetExitCodeThread);
		DWORD exitCode = 0;
		if ((*GetExitCodeThreadPtr)(newThread, &exitCode) == 0) {
			WinError winErr;
			outputObject << L"Failed to get the exit code of the thread that was supposed to inject the DLL: "
				<< winErr.getMessage() << std::endl;
			return false;
		}
		if (exitCode != 0) {
			// on success, LoadLibrary returns a handle to the module, but that handle, as I noticed every time, is the base address of the module
			modBaseAddr = findModuleUsingEnumProcesses(procId, dll);
			if (modBaseAddr == exitCode) {
				outputObject << L"Injected successfully. You can launch this injector again to unload the DLL.\n";
				return true;
			} else if (modBaseAddr) {
				outputObject << "Noticed that the mod's DLL is present in the game, maybe it got injected correctly.\n";
			}
		}
		outputObject << L"Injection possibly failed. Exit code of the injection thread: 0x" << std::hex << exitCode << std::dec << std::endl;
		return false;
	} else if (waitResult == WAIT_FAILED) {
		WinError winErr;
		outputObject << L"Failed to wait for the injection to finish: " << winErr.getMessage() << std::endl;
		return false;
	} else {
		outputObject << L"Failed to wait for the injection to finish. Wait result: 0x" << std::hex << waitResult << std::dec << std::endl;
		return false;
	}
	
	// upon function exit cleanup will run its destructor and free up resources
}

// this was once a console app
// injectorMain was called main
// outputObject was instead std::wcout
// GetLine was a std::readline(std::wcin, str);
int injectorMain() {
	
	memcpy(exe, OBF_DATA(L"GuiltyGearXrd.exe"), sizeof exe);
	memcpy(dll, OBF_DATA(L"ggxrd_hitbox_overlay.dll"), sizeof dll);
	
	outputObject << L"This program will look for " << exe << L" process and inject the " << dll << L" into it.\n"
		<< L"The DLL must be in the same folder as this injector in order for this to work.\n"
		<< L"Only Guilty Gear Xrd Rev2 version 2211 supported.\n";
	
	std::wstring ignoreLine;
	bool success;
	DWORD procId;
	if (force) {
		outputObject << L"Waiting for Guilty Gear Xrd's window to open. Press "
			// I could not find a way to reliably non-blockingly peek into stdin, so no timeoutable "press any key" on the commandline
			// PeekConsoleInput is a garbage glitchy nightmare that reports key down events even when no keys are pressed
			// Windows commandline is blocking, we just have to live with that
			#ifndef ANY_KEY
			<< L"Ctrl+C"
			#else
			<< ANY_KEY
			#endif
			<< " to exit...\n";
		pressAnyKeyBegin();
		while (true) {
			if (isAnyKeyPressed()) {
				pressAnyKeyEnd();
				outputObject << L"Terminated by the user.\n";
				success = true;
				break;
			}
			procId = findOpenGgProcess();
			if (procId) {
				outputObject << "Found PID: " << procId << '\n';
				pressAnyKeyEnd();
				success = injectorTask(procId);
				break;
			}
			Sleep(333);
		}
	} else {
		outputObject << L"Press Enter to continue...\n";
		GetLine(ignoreLine);
		procId = findOpenGgProcess();
		if (!procId) {
			outputObject << L"Process with the name '" << exe << "' not found. Launch Guilty Gear and then try again.\n";
			success = false;
		} else {
			success = injectorTask(procId);
		}
	}
	
	if (!success || !force) {
		outputObject << L"Press Enter to exit...\n";
		GetLine(ignoreLine);
	}
	return 0;
}

// Finds if GuiltyGearXrd.exe is currently open and returns the ID of its process
DWORD findOpenGgProcess() {
	OBF_LOAD(user32);
	
    // this method was chosen because it's much faster than enumerating all windows or all processes and checking their names
    // also it was chosen because Xrd restarts itself upon launch, and the window appears only on the second, true start
    OBF_IMPORT(user32, FindWindowW);
    HWND foundGgWindow = (*FindWindowWPtr)(L"LaunchUnrealUWindowsClient", L"Guilty Gear Xrd -REVELATOR-");
    if (!foundGgWindow) return NULL;
    DWORD windsProcId = 0;
    OBF_IMPORT(user32, GetWindowThreadProcessId);
    (*GetWindowThreadProcessIdPtr)(foundGgWindow, &windsProcId);
    return windsProcId;
}
