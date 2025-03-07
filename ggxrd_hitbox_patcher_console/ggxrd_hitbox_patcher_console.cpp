#include "ggxrd_hitbox_patcher_common.h"
#include <iostream>
#include <string>
#include "PatcherConsoleVersion.h"  // this file is included so that if version changes, it causes recompilation and the Pre-Build event fires which launches a .ps1 script that updates VERSIONINFO

// Do not ship the console version of the patcher to Windows users, because it is being detected as a virus by Windows Defender.
// This happens because Visual Studio's boilerplate code for any console program, even int main(){return 0;} is detected as a virus by a Machine Learning algorithm.
// You may ship the Linux console version of the patcher to Windows and Linux users.

void OutputStringA(const char* text) {
	std::cout << text;
}

void OutputStringW(const wchar_t* text) {
	std::wcout << text;
}

#ifndef FOR_LINUX
void GetLine(std::wstring& line) {
	std::getline(std::wcin, line);
}
#else
void GetLine(std::string& line) {
	std::getline(std::cin, line);
}
#endif

int main() {
	std::cout << "Patcher version " << PATCHER_CONSOLE_VERSION << '\n';
	return patcherMain();
}
