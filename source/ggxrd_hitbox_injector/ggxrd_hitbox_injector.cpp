#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>

// original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/
// Compile without UNICODE. Right click the project in Solution Explorer -> Properties -> Advanced -> Character Set -> Use Multi-Byte Character Set

int main()
{
	const auto *exe = "GuiltyGearXrd.exe";
	const auto *dll = "ggxrd_hitbox_overlay.dll";

	const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	Process32First(snap, &entry);
	do 
	{
		const auto proc = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
		if (proc == INVALID_HANDLE_VALUE || proc == NULL) continue;

		char path[MAX_PATH];
		if (GetModuleFileNameEx(proc, nullptr, path, MAX_PATH) == 0)
		{
			CloseHandle(proc);
			continue;
		}

		if (strcmp(path + strlen(path) - strlen(exe), exe) != 0)
		{
			CloseHandle(proc);
			continue;
		}

		std::cout << "Found the process by name\n";

		char dll_path[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dll_path);
		strcat_s(dll_path, MAX_PATH, "/");
		strcat_s(dll_path, MAX_PATH, dll);
		std::cout << "Dll path: " << dll_path << std::endl;

		const auto size = strlen(dll_path) + 1;
		auto *buf = VirtualAllocEx(proc, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (buf == NULL) {
			std::cout << "Failed to allocate memory\n";
			return 1;
		}
		std::cout << "Allocated memory: " << buf << std::endl;
		BOOL success = WriteProcessMemory(proc, buf, dll_path, size, nullptr);
		std::cout << "Wrote memory: " << success << std::endl;
		HANDLE newThread = CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(LoadLibrary), buf, 0, nullptr);
		if (newThread == INVALID_HANDLE_VALUE || newThread == 0) {
			std::cout << "Failed to create remote thread\n";
		}

		CloseHandle(proc);
		CloseHandle(snap);
		std::cout << "Injected successfully\n";
		break;
	}
	while (Process32Next(snap, &entry));

	return 0;
}