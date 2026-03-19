#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <vector>

void printError(const char* whatFailed, int code) {
	LPSTR message = NULL;
	if (code) {
		FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				code,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)(&message),
				0, NULL);
	}
	fprintf(stderr, whatFailed);
	if (message) {
		fprintf(stderr, ": %s\n", message);
		LocalFree(message);
	}
}
	
HANDLE g_ggxrd = NULL;

HANDLE findGGXrd() {
	// FindWindowW is unimplemented on Proton 8.0
	
	if (g_ggxrd) return g_ggxrd;
	
	struct Closer {
		~Closer() {
			if (snapshot) CloseHandle(snapshot);
		}
		HANDLE snapshot;
	} closer;
	
	closer.snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!closer.snapshot) {
		printError("Failed to call CreateToolhelp32Snapshot", GetLastError());
		return NULL;
	}
	
	PROCESSENTRY32W process;
	process.dwSize = sizeof PROCESSENTRY32W;
	if (!Process32FirstW(closer.snapshot, &process)) {
		DWORD code = GetLastError();
		if (code != ERROR_NO_MORE_FILES) {
			printError("Failed to call Process32FirstW", code);
		}
		return NULL;
	}
	
	while (true) {
		if (_wcsicmp(process.szExeFile, L"GuiltyGearXrd.exe") == 0) {
			// https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getmodulebasenamew
			// GetModuleBaseNameW: The handle must have the PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access rights.
			g_ggxrd = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process.th32ProcessID);
			if (!g_ggxrd) {
				printError("Failed to open GuiltyGearXrd.exe process", GetLastError());
				return NULL;
			}
			return g_ggxrd;
		}
		if (!Process32NextW(closer.snapshot, &process)) {
			DWORD code = GetLastError();
			if (code != ERROR_NO_MORE_FILES) {
				printError("Failed to call Process32NextW", code);
			}
			return NULL;
		}
	}
}

enum processNextRequestResult {
	processNextRequest_notFound,
	processNextRequest_error,
	processNextRequest_emptySearchInput,
	processNextRequest_found
};

processNextRequestResult processNextRequest(std::vector<char>& moduleName) {
	if (!moduleName.empty() && moduleName.back() == '\r') {
		moduleName.resize(moduleName.size() - 1);
	}
	if (moduleName.empty()) return processNextRequest_emptySearchInput;
	
	HANDLE xrd = findGGXrd();
	if (!xrd) {
		fprintf(stderr, "GuiltyGearXrd.exe process not found.\n");
		return processNextRequest_error;
	}
	
	std::vector<HMODULE> allModulesHeap;
	HMODULE allModulesStack[200];
	HMODULE* allModulesPtr = allModulesStack;
	int allModulesSize = (int)_countof(allModulesStack);
	while (true) {
		DWORD bytesNeeded = 0;
		if (!EnumProcessModules(xrd, allModulesPtr, allModulesSize * sizeof HMODULE, &bytesNeeded)) {
			printError("Failed to call EnumProcessModules", GetLastError());
			return processNextRequest_error;
		}
		if (allModulesSize * sizeof (HMODULE) < bytesNeeded) {
			int modulesNeeded = bytesNeeded / sizeof (HMODULE);
			if (modulesNeeded > (int)_countof(allModulesStack)) {
				allModulesHeap.resize(modulesNeeded);
				allModulesPtr = allModulesHeap.data();
				allModulesSize = modulesNeeded;
			} else {
				allModulesPtr = allModulesStack;
				allModulesSize = (int)_countof(allModulesStack);
			}
			continue;
		}
		allModulesSize = bytesNeeded / sizeof (HMODULE);
		break;
	}
	
	const char* searchNamePtrStart = moduleName.data();
	size_t charCount = moduleName.size();
	
	{
		bool hasDot = false;
		for (size_t charIndex = 0; charIndex < charCount; ++charIndex) {
			char currentChar = searchNamePtrStart[charIndex];
			if (currentChar == '.') {
				hasDot = true;
				break;
			}
		}
		
		if (!hasDot) {
			moduleName.resize(moduleName.size() + 4);
			memcpy(moduleName.data() + charCount, ".dll", 4);
			searchNamePtrStart = moduleName.data();
			charCount = moduleName.size();
		}
	}
	
	for (int moduleIndex = 0; moduleIndex < allModulesSize; ++moduleIndex) {
		MODULEINFO moduleInfo;
		wchar_t moduleBaseName[1024];
		if (!GetModuleBaseNameW(xrd, allModulesPtr[moduleIndex], moduleBaseName, _countof(moduleBaseName))) {
			printError("Failed to call GetModuleInformation", GetLastError());
			return processNextRequest_error;
		}
		wchar_t* moduleNamePtr = moduleBaseName;
		const char* searchNamePtr = searchNamePtrStart;
		bool match = true;
		for (size_t charIndex = 0; charIndex < charCount; ++charIndex) {
			wchar_t moduleWChar = *moduleNamePtr;
			char searchChar = tolower(*searchNamePtr);
			if (moduleWChar == 0 || moduleWChar > 127) {
				match = false;
				break;
			}
			char moduleChar = tolower((char)moduleWChar);
			if (moduleChar != searchChar) {
				match = false;
				break;
			}
			++moduleNamePtr;
			++searchNamePtr;
		}
		if (!match) {
			continue;
		}
		if (*moduleNamePtr) {
			continue;
		}
		// complete case-insensitive match
		for (char c : moduleName) {
			putc(c, stdout);
		}
		puts(" found.");
		fflush(stdout);
		return processNextRequest_found;
	}
	for (char c : moduleName) {
		putc(c, stdout);
	}
	puts(" not found.");
	fflush(stdout);
	return processNextRequest_notFound;
}

void Cleanup() {
	if (g_ggxrd) CloseHandle(g_ggxrd);
	fflush(stdout);
	fflush(stderr);
}

int main(int argc, char** argv)
{
	if (argc > 0) {
		++argv;  // skip program path, not interested
		--argc;
	}
	std::vector<char> moduleName;
	
	if (argc > 0) {
		int totalModulesPassed = 0;
		processNextRequestResult lastResult = processNextRequest_notFound;
		// ok so we get module names in arguments instead
		bool thisIsFirst = true;
		while (argc > 0) {
			++totalModulesPassed;
			int len = strlen(*argv);
			if (thisIsFirst && (len == 6 || len == 2)) {
				if (_stricmp(*argv, "--help") == 0 || strcmp(*argv, "/?") == 0) {
					puts("Provide module names to look for (without path, for example: ggxrd_hitbox_overlay.dll. Specifying the \".dll\" at the end is optional)"
						" in either the command arguments or in stdin.\n"
						"If providing in stdin, separate with newlines. If providing on the commandline, separate with spaces, may optionally enclose each"
						" module name in double quotes (but may not enclose in double quotes in stdin).\n"
						"If passing modules on the commandline, you may not pass them in stdin, and vice versa.\n"
						"If passing modules on the commandline, if only one module name is passed, the program exits with exit code 0 if the module is found,"
						" otherwise exits with exit code 1.\n"
						"If more than one module name is passed on the commandline, the exit code is always 0, if no errors occurred.\n"
						"If GuiltyGearXrd.exe process is not found or an error occurs, the program does not wait for it to open and immediately"
						" prints an error into stderr (not stdout, unless you piped stderr into stdout) and exits with exit code 1.\n"
						"If passing module names in stdin, close your end of the pipe to terminate the program. It will read the last module name"
						" on there and report whether each is found into stdout, then exit with code 0 if no errors occurred.\n"
						"The program always reports into stdout whether each requested module is found in the following format:\n"
						"ggxrd_hitbox_overlay.dll found.<NEWLINE CHARACTER>\n"
						"or\n"
						"ggxrd_hitbox_overlay.dll not found.<NEWLINE CHARACTER>\n"
						"ggxrd_hitbox_overlay.dll is the requested module name."
						" It always adds the \".dll\" to the end of the module name, if it was omitted.");
					return 0;
				}
			}
			thisIsFirst = false;
			moduleName.resize(len);
			memcpy(moduleName.data(), *argv, len);
			lastResult = processNextRequest(moduleName);
			if (lastResult == processNextRequest_error) {
				Cleanup();
				return 1;
			}
			if (lastResult == processNextRequest_emptySearchInput) {
				--totalModulesPassed;  // doesn't count I guess
			}
			--argc;
			++argv;
		}
		if (totalModulesPassed == 1) {
			switch (lastResult) {
				case processNextRequest_notFound:
				case processNextRequest_emptySearchInput:
				default:
					return 1;
				case processNextRequest_found:
					return 0;
			}
		} else if (totalModulesPassed > 1) {
			return 0;
		}
	}
	
	while (true) {
		int nextChar = getc(stdin);
		if (ferror(stdin)) {
			perror("Failed to get next character.");
			Cleanup();
			return 1;
		}
		if (nextChar == '\n' || nextChar == EOF) {
			if (processNextRequest(moduleName) == processNextRequest_error) {
				Cleanup();
				return 1;
			}
			moduleName.clear();
			if (nextChar == EOF) {
				Cleanup();
				return 0;
			}
		} else {
			moduleName.push_back(nextChar);
		}
	}
}
