#include "pch.h"
#include <iostream>
#include <string>
#include "ggxrd_hitbox_injector_common.h"
#include "InjectorConsoleVersion.h"  // I included this file so that when it changes, it triggers the Pre-Build event and updates versions in the .rc file

// original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/

// Do not ship the console version of the injector to users, because it is being detected as a virus by Windows Defender.
// This happens because Visual Studio's boilerplate code for any console program, even int main(){return 0;} is detected as a virus by a Machine Learning algorithm.

bool force = false;

void __cdecl GetLine(std::wstring& line) { std::getline(std::wcin, line); }

void OutputStringA(const char* text) { std::cout << text; }

void OutputStringW(const wchar_t* text) { std::wcout << text; }

// PeekConsoleInput is glitchy and returns key down events as soon as the program starts even though no keys were pressed. It doesn't work properly even if you wait 1 second after starting the program
void pressAnyKeyBegin() { }

void pressAnyKeyEnd() { }

bool isAnyKeyPressed() { return false; }

int wmain(int argc, wchar_t** argv) {
	for (int i = 0; i < argc; ++i) {
		if (_wcsicmp(argv[i], L"--help") == 0
				|| _wcsicmp(argv[i], L"-help") == 0
				|| _wcsicmp(argv[i], L"/?") == 0) {
			std::cout << "ggxrd_hitbox_injector for Guilty Gear Xrd Rev2 version 2211.\n"
				" Arguments:\n"
				" <None> - launch in interactive mode.\n"
				" -force - attempt to inject silently.\n";
			return 0;
		} else if (_wcsicmp(argv[i], L"-force") == 0) {
			force = true;
		}
	}
	
	return injectorMain();
}
