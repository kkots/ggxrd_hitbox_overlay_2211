#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <tchar.h> // please Microsoft make it so tchar no longer exists there's enough headache without it

// original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/
// Compile without UNICODE. Right click the project in Solution Explorer -> Properties -> Advanced -> Character Set -> Use Multi-Byte Character Set

#ifdef  UNICODE
#define tout std::wcout
#else
#define tout std::ccout
#endif

int main()
{
	const auto *exe = TEXT("GuiltyGearXrd.exe");
	const auto *dll = TEXT("ggxrd_hitbox_overlay.dll");

	const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	bool foundProcess = false;

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

		TCHAR dll_path[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dll_path);
		_tcscat_s(dll_path, MAX_PATH, TEXT("/"));
		_tcscat_s(dll_path, MAX_PATH, dll);
		tout << TEXT("Dll path: ") << dll_path << std::endl;

		const auto size = (_tcsclen(dll_path) + 1) * sizeof(TCHAR);
		auto *buf = VirtualAllocEx(proc, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (buf == NULL) {
			std::cout << "Failed to allocate memory\n";
			break;
		}
		std::cout << "Allocated memory: " << buf << std::endl;
		if (WriteProcessMemory(proc, buf, dll_path, size, nullptr)) {
			std::cout << "Wrote memory successfully\n";
		} else {
			std::cout << "Failed to write memory\n";
			break;
		}
		HANDLE newThread = CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(LoadLibrary), buf, 0, nullptr);
		if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
			std::cout << "Failed to create remote thread\n";
			break;
		}

		CloseHandle(proc);
		CloseHandle(snap);
		std::cout << "Injected successfully\n";
		break;
	}
	while (Process32Next(snap, &entry));
	
	if (proc && proc != INVALID_HANDLE_VALUE) {
		CloseHandle(proc);
	}
	if (!foundProcess) {
		std::cout << "Process with the name 'GuiltyGearXrd.exe' not found. Launch Guilty Gear and then try again.\n";
	}
	std::cout << "Press Enter to exit...\n";
	int ignoreVal = getc(stdin);
	return 0;
}