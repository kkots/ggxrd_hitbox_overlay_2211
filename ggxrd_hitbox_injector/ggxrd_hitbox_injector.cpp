#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include "WinError.h"

// original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/

// Finds module (a loaded dll or exe itself) in the given process by name.
// If module is not found, the returned module will have its modBaseAddr equal to 0.
// Parameters:
//  procId - process ID (PID)
//  name - the name of the module, including the .exe or .dll at the end
//  is32Bit - specify true if the target process is 32-bit
MODULEENTRY32W findModule(DWORD procId, const wchar_t* name, bool is32Bit) {
	HANDLE hSnapshot = NULL;
	MODULEENTRY32W mod32{ 0 };
	mod32.dwSize = sizeof(MODULEENTRY32W);
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
				std::wcout << L"Error in CreateToolhelp32Snapshot: " << err.getMessage() << std::endl;
				return MODULEENTRY32W{ 0 };
			}
		}
		else {
			break;
		}
	}
	if (!Module32FirstW(hSnapshot, &mod32)) {
		WinError winErr;
		std::wcout << L"Error in Module32First: " << winErr.getMessage() << std::endl;
		CloseHandle(hSnapshot);
		return MODULEENTRY32W{ 0 };
	}
	while (true) {
		if (CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, mod32.szModule, -1, name, -1) == CSTR_EQUAL) {
			CloseHandle(hSnapshot);
			return mod32;
		}
		BOOL resNext = Module32NextW(hSnapshot, &mod32);
		if (!resNext) {
			WinError err;
			if (err.code != ERROR_NO_MORE_FILES) {
				std::wcout << L"Error in Module32Next: " << err.getMessage() << std::endl;
				CloseHandle(hSnapshot);
				return MODULEENTRY32W{ 0 };
			}
			break;
		}
	}
	CloseHandle(hSnapshot);
	return MODULEENTRY32W{ 0 };
}

int main()
{
	const wchar_t* exe = L"GuiltyGearXrd.exe";
	const wchar_t* dll = L"ggxrd_hitbox_overlay.dll";

	std::wcout << L"This program will look for " << exe << L" process and inject the " << dll << L" into it.\n"
		<< L"The DLL must be in the same folder as this injector in order for this to work.\n"
		<< L"Only Guilty Gear Xrd Rev2 version 2211 supported.\n"
		<< L"Press Enter to continue...\n";
	std::wstring ignoreLine;
	std::getline(std::wcin, ignoreLine);

	const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(entry);

	bool foundProcess = false;

	LPVOID buf = NULL;
	std::vector<HANDLE> handlesToClose;
	HANDLE proc = INVALID_HANDLE_VALUE;
	Process32FirstW(snap, &entry);
	do
	{
		if (proc && proc != INVALID_HANDLE_VALUE) CloseHandle(proc);
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
		if (proc == INVALID_HANDLE_VALUE || proc == NULL) continue;

		wchar_t path[MAX_PATH];
		if (GetModuleFileNameExW(proc, nullptr, path, MAX_PATH) == 0) continue;
		if (wcscmp(path + wcslen(path) - wcslen(exe), exe) != 0) continue;

		foundProcess = true;
		std::wcout << L"Found the process by name: " << exe << std::endl;

		MODULEENTRY32W mod32 = findModule(entry.th32ProcessID, dll, true);
		bool skipTheRest = false;
		if (mod32.modBaseAddr) {
			while (true) {
				std::wcout << L"The dll is already loaded into the application. Do you want to unload it? (Type y/n):\n";
				std::wstring lineContents;
				std::getline(std::wcin, lineContents);
				if (lineContents == L"y" || lineContents == L"Y") {
					HANDLE newThread = CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(FreeLibrary), mod32.modBaseAddr, 0, nullptr);
					if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
						WinError winErr;
						std::wcout << L"Failed to create remote thread: " << winErr.getMessage() << std::endl;
						break;
					}
					handlesToClose.push_back(newThread);
					std::wcout << L"DLL unloaded.\n";
					break;
				}
				else if (lineContents == L"n" || lineContents == L"N") {
					std::wcout << L"The DLL won't be loaded a second time. No action will be taken.\n";
					break;
				}
			}
			break;
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
		if (skipTheRest) break;

		wchar_t dll_path[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, dll_path);
		wcscat_s(dll_path, MAX_PATH, L"\\");
		wcscat_s(dll_path, MAX_PATH, dll);
		std::wcout << L"Dll path: " << dll_path << std::endl;
		DWORD dllAtrib = GetFileAttributesW(dll_path);
		if (dllAtrib == INVALID_FILE_ATTRIBUTES) {
			WinError winErr;
			std::wcout << winErr.getMessage() << std::endl;
			break;
		}
		if ((dllAtrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			std::wcout << L"The found DLL is actually a directory. Terminating.\n";
			break;
		}

		const auto size = (wcslen(dll_path) + 1) * sizeof(wchar_t);
		buf = VirtualAllocEx(proc, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (buf == NULL) {
			std::wcout << L"Failed to allocate memory.\n";
			break;
		}
		std::wcout << L"Allocated memory: " << buf << std::endl;
		if (WriteProcessMemory(proc, buf, dll_path, size, nullptr)) {
			std::wcout << L"Wrote memory successfully.\n";
		}
		else {
			std::wcout << L"Failed to write memory.\n";
			break;
		}
		HANDLE newThread = CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(LoadLibraryW), buf, 0, nullptr);
		if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
			WinError winErr;
			std::wcout << L"Failed to create remote thread: " << winErr.getMessage() << std::endl;
			break;
		}
		handlesToClose.push_back(newThread);
		std::wcout << L"Injected successfully. You can launch this injector again to unload the DLL.\n";
		break;
	} while (Process32NextW(snap, &entry));

	for (HANDLE h : handlesToClose) {
		CloseHandle(h);
	}
	CloseHandle(snap);
	if (!foundProcess) {
		std::wcout << L"Process with the name 'GuiltyGearXrd.exe' not found. Launch Guilty Gear and then try again.\n";
	}

	std::wcout << L"Press Enter to exit...\n";
	std::getline(std::wcin, ignoreLine);
	if (proc && proc != INVALID_HANDLE_VALUE) {
		if (buf) {
			VirtualFreeEx(proc, buf, 0, MEM_RELEASE);
		}
		CloseHandle(proc);
	}
	return 0;
}
