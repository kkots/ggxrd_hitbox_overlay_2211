#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <tchar.h> // please Microsoft make it so tchar no longer exists there's enough headache without it
#include <string>
#include <vector>
#include "WinError.h"

// original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/

#ifndef tout
#ifdef UNICODE
#define tout std::wcout
#else
#define tout std::cout
#endif
#endif

#ifndef tin
#ifdef UNICODE
#define tin std::wcin
#else
#define tin std::cin
#endif
#endif

#ifndef tstring
#ifdef UNICODE
#define tstring std::wstring
#else
#define tstring std::string
#endif
#endif

// Finds module (a loaded dll or exe itself) in the given process by name.
// If module is not found, the returned module will have its modBaseAddr equal to 0.
// Parameters:
//  procId - process ID (PID)
//  name - the name of the module, including the .exe or .dll at the end
//  is32Bit - specify true if the target process is 32-bit
MODULEENTRY32 findModule(DWORD procId, LPCTSTR name, bool is32Bit) {
	HANDLE hSnapshot = NULL;
	MODULEENTRY32 mod32{ 0 };
	mod32.dwSize = sizeof(MODULEENTRY32);
	while (true) {
		// If you're a 64-bit process trying to get modules from a 32-bit process,
		// use TH32CS_SNAPMODULE32.
		// If you're a 64-bit process trying to get modules from a 64-bit process,
		// use TH32CS_SNAPMODULE.
		hSnapshot = CreateToolhelp32Snapshot(is32Bit ? (TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32) : TH32CS_SNAPMODULE, procId);
		if (hSnapshot == INVALID_HANDLE_VALUE || !hSnapshot) {
			WinError err;
			if (err.code == ERROR_BAD_LENGTH) {
				continue;
			}
			else {
				tout << TEXT("Error in CreateToolhelp32Snapshot: ") << err.getMessage() << std::endl;
				return MODULEENTRY32{ 0 };
			}
		}
		else {
			break;
		}
	}
	if (!Module32First(hSnapshot, &mod32)) {
		WinError winErr;
		tout << TEXT("Error in Module32First: ") << winErr.getMessage() << std::endl;
		CloseHandle(hSnapshot);
		return MODULEENTRY32{ 0 };
	}
	while (true) {
		if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, mod32.szModule, -1, name, -1) == CSTR_EQUAL) {
			CloseHandle(hSnapshot);
			return mod32;
		}
		BOOL resNext = Module32Next(hSnapshot, &mod32);
		if (!resNext) {
			WinError err;
			if (err.code != ERROR_NO_MORE_FILES) {
				tout << TEXT("Error in Module32Next: ") << err.getMessage() << std::endl;
				CloseHandle(hSnapshot);
				return MODULEENTRY32{ 0 };
			}
			break;
		}
	}
	CloseHandle(hSnapshot);
	return MODULEENTRY32{ 0 };
}

int main()
{
	const auto* exe = TEXT("GuiltyGearXrd.exe");
	const auto* dll = TEXT("ggxrd_hitbox_overlay.dll");

	tout << TEXT("This program will look for ") << exe << TEXT(" process and inject the ") << dll << TEXT(" into it.\n")
		<< TEXT("The DLL must be in the same folder as this injector in order for this to work.\n")
		<< TEXT("Only Guilty Gear Xrd Rev2 version 2211 supported.\n")
		<< TEXT("Press Enter to continue...\n");
	tstring ignoreLine;
	std::getline(tin, ignoreLine);

	const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	bool foundProcess = false;

	LPVOID buf = NULL;
	std::vector<HANDLE> handlesToClose;
	HANDLE proc = INVALID_HANDLE_VALUE;
	Process32First(snap, &entry);
	do
	{
		if (proc && proc != INVALID_HANDLE_VALUE) CloseHandle(proc);
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
		if (proc == INVALID_HANDLE_VALUE || proc == NULL) continue;

		TCHAR path[MAX_PATH];
		if (GetModuleFileNameEx(proc, nullptr, path, MAX_PATH) == 0) continue;
		if (_tcscmp(path + _tcsclen(path) - _tcsclen(exe), exe) != 0) continue;

		foundProcess = true;
		tout << TEXT("Found the process by name: ") << exe << std::endl;

		MODULEENTRY32 mod32 = findModule(entry.th32ProcessID, dll, true);
		bool skipTheRest = false;
		if (mod32.modBaseAddr) {
			while (true) {
				tout << TEXT("The dll is already loaded into the application. Do you want to unload it? (Type y/n):\n");
				tstring lineContents;
				std::getline(tin, lineContents);
				if (lineContents == TEXT("y") || lineContents == TEXT("Y")) {
					HANDLE newThread = CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(FreeLibrary), mod32.modBaseAddr, 0, nullptr);
					if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
						WinError winErr;
						tout << TEXT("Failed to create remote thread: ") << winErr.getMessage() << std::endl;
						break;
					}
					handlesToClose.push_back(newThread);
					tout << TEXT("DLL unloaded.\n");
					break;
				}
				else if (lineContents == TEXT("n") || lineContents == TEXT("N")) {
					tout << TEXT("The DLL won't be loaded a second time. No action will be taken.\n");
					break;
				}
			}
			break;
		}
		else {
			while (true) {
				tout << TEXT("The dll is not yet loaded into the application. Do you want to load it? (Type y/n):\n");
				tstring lineContents;
				std::getline(tin, lineContents);
				if (lineContents == TEXT("y") || lineContents == TEXT("Y")) {
					break;
				}
				else if (lineContents == TEXT("n") || lineContents == TEXT("N")) {
					tout << TEXT("The DLL won't be loaded.\n");
					skipTheRest = true;
					break;
				}
			}
		}
		if (skipTheRest) break;

		TCHAR dll_path[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dll_path);
		_tcscat_s(dll_path, MAX_PATH, TEXT("\\"));
		_tcscat_s(dll_path, MAX_PATH, dll);
		tout << TEXT("Dll path: ") << dll_path << std::endl;
		DWORD dllAtrib = GetFileAttributes(dll_path);
		if (dllAtrib == INVALID_FILE_ATTRIBUTES) {
			WinError winErr;
			tout << winErr.getMessage() << std::endl;
			break;
		}
		if ((dllAtrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			tout << TEXT("The found DLL is actually a directory. Terminating.\n");
			break;
		}

		const auto size = (_tcsclen(dll_path) + 1) * sizeof(TCHAR);
		buf = VirtualAllocEx(proc, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (buf == NULL) {
			tout << TEXT("Failed to allocate memory.\n");
			break;
		}
		tout << TEXT("Allocated memory: ") << buf << std::endl;
		if (WriteProcessMemory(proc, buf, dll_path, size, nullptr)) {
			tout << TEXT("Wrote memory successfully.\n");
		}
		else {
			tout << TEXT("Failed to write memory.\n");
			break;
		}
		HANDLE newThread = CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(LoadLibrary), buf, 0, nullptr);
		if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
			WinError winErr;
			tout << TEXT("Failed to create remote thread: ") << winErr.getMessage() << std::endl;
			break;
		}
		handlesToClose.push_back(newThread);
		tout << TEXT("Injected successfully. You can launch this injector again to unload the DLL.\n");
		break;
	} while (Process32Next(snap, &entry));

	for (HANDLE h : handlesToClose) {
		CloseHandle(h);
	}
	CloseHandle(snap);
	if (!foundProcess) {
		tout << TEXT("Process with the name 'GuiltyGearXrd.exe' not found. Launch Guilty Gear and then try again.\n");
	}

	tout << TEXT("Press Enter to exit...\n");
	std::getline(tin, ignoreLine);
	if (proc && proc != INVALID_HANDLE_VALUE) {
		if (buf) {
			VirtualFreeEx(proc, buf, 0, MEM_RELEASE);
		}
		CloseHandle(proc);
	}
	return 0;
}
