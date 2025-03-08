#pragma once
#include "pch.h"

// This was created because Windows Defender thinks every console Hello World.exe from Visual Studio is one of
// 1) Trojan:Win32/Wacatac.B!ml
// 2) Trojan:Win32/Phonzy.B!ml
// , even if that console program is just int main(){return 0;}.

// How this thing works:
// It will create window using the title, class name, icon, menu name and accelerator that you define in your own .cpp
// file using the extern variables below.
// That window will create a thread that executes the taskThreadProc function.
// You must define what that function does. You should launch some program that
// does something and, optionally, may print things into stdout or ask for a line of input
// from stdin.
// In order for that to work, the window will house an Edit control that outputs and lets
// the user input text.
// The program must use a special object instead of std::cout, std::wcout and std::cin and std::wcin.
// That object is InjectorCommonOut. It defines a very limited set of << operator overloads, but you can extend
// that if need be.
// The text output commands get sent to the window and it outputs text into the Edit control.
// From stdin you can only request one whole line of input. You can't request individual characters, peek, ungetc or other.
// To do that the program calls GetLine(std::wstring& line);
// This whole console emulation thing is for Windows only, there's no Linux, so everything should always be UNICODE.
// Anyway, when the program returns, the thread exits and the window closes.
// Do not forget to do a PostMessageW(mainWindow, WM_TASK_ENDED, 0, 0) at the end of taskThreadProc.
// It signals the window to close itself.

#define WM_GET_LINE WM_USER + 1
#define WM_TASK_ENDED WM_USER + 2
#define WM_OUTPUT_STRING_A WM_USER + 3
#define WM_OUTPUT_STRING_W WM_USER + 4
#define WM_PRESS_ANY_KEY_BEGIN WM_USER + 5
#define WM_PRESS_ANY_KEY_END WM_USER + 6
#define WM_IS_ANY_KEY_PRESSED WM_USER + 7

// You will need these to implement needed functions:
extern HWND mainWindow;
extern bool force;  // set this to true in parseArgs. It will make the window not show up for the first 8 seconds.

// Implement these functions and variables:
extern unsigned long __stdcall taskThreadProc(LPVOID unused);  // must post WM_TASK_ENDED to mainWindow: PostMessageW(mainWindow, WM_TASK_ENDED, 0, 0);
extern bool parseArgs(int argc, LPWSTR* argv, int* exitCode);

// Set default values for these, like for constants, so that by the time wWinMain launches, they're already initialized
extern bool forceAllowed;  // this means that if force is true, the window will acknowledge it and won't show up for the first 8 seconds
extern UINT windowAppTitleResourceId;
extern UINT windowClassNameResourceId;
extern LPCWSTR windowIconId;
extern LPCWSTR windowMenuName;
extern LPCWSTR windowAcceleratorId;
