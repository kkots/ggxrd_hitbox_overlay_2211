#include "pch.h"
#include "resource.h"
#include "ConsoleEmulator.h"
#include <shellapi.h>  // for CommandLineToArgvW
#include <vector>
#include <string>
#include <commctrl.h>
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
// 3) Add a script that sits in the Debug/Release folders, checks if each executable or .dll file is signed, and signs it if it isn't.
// 6) Transition patcher to window mode, will probably have to do something very similar.

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
bool force = false;
HFONT font = NULL;
HWND mainWindow = NULL;
HWND textEdit = NULL;
HANDLE eventToInjectorThread = NULL;
std::wstring* getLineLine = nullptr;
bool wantAnyKey = false;
bool anyKeyPressed = false;
int nCmdShowToUse = 0;

extern void __cdecl GetLine(std::wstring& line);
void OutputStringA(const char* text);
void OutputStringW(const wchar_t* text);
void pressAnyKeyBegin();
void pressAnyKeyEnd();
bool isAnyKeyPressed();
void onPressAnyKeyBegin();
void onPressAnyKeyEnd();
void onIsAnyKeyPressed(bool* theValue);
void OutputStringAImpl(const char* text);
void OutputStringWImpl(const wchar_t* text);
void onTimer();
LRESULT __stdcall TextEditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void onGetLine(WPARAM wParam);
int countChar(const char* text, int chr);
void onCommand(WPARAM wParam, LPARAM lParam, LRESULT* lResult);
void onKeyDown(WPARAM wParam, LRESULT* lResult, bool handleEventOnYourOwn);
void onSize(WPARAM wParam, LPARAM lParam, LRESULT* lResult);
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	
	int numArgs = 0;
	LPWSTR* contiguousBlockOfMemory = CommandLineToArgvW(lpCmdLine, &numArgs);
	if (contiguousBlockOfMemory) {
		int exitCode;
		if (!parseArgs(numArgs, contiguousBlockOfMemory, &exitCode)) {
			return exitCode;
		}
		LocalFree(contiguousBlockOfMemory);
	}
	
	LoadStringW(hInstance, windowAppTitleResourceId, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, windowClassNameResourceId, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}
	
	HACCEL hAccelTable = LoadAcceleratorsW(hInstance, windowAcceleratorId);
	
	MSG msg;
	
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		if (!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	
	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex;
	
	wcex.cbSize = sizeof(WNDCLASSEX);
	
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIconW(hInstance, windowIconId);
	wcex.hCursor        = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = windowMenuName;
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIconW(wcex.hInstance, windowIconId);
	
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance;
	
	mainWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	
	if (!mainWindow) {
		return FALSE;
	}
	
	RECT clientRect;
	GetClientRect(mainWindow, &clientRect);
	
	NONCLIENTMETRICSW nonClientMetrics { 0 };
	nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICSW);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &nonClientMetrics, NULL);
	font = CreateFontIndirectW(&nonClientMetrics.lfCaptionFont);
	
	textEdit = CreateWindowW(WC_EDITW, L"",
		WS_CHILD
			| WS_OVERLAPPED
			| WS_VISIBLE
			| ES_MULTILINE
			| WS_VSCROLL
			| ES_AUTOVSCROLL
			| ES_READONLY,  // change with EM_SETREADONLY
		5, 5, clientRect.bottom - 10, clientRect.right - 10, mainWindow, NULL, hInst, NULL);
	SendMessageW(textEdit, WM_SETFONT, (WPARAM)font, TRUE);
	SetWindowSubclass(textEdit, TextEditSubclass, 1, NULL);
	
	eventToInjectorThread = CreateEventW(NULL, FALSE, FALSE, NULL);
	
	if (force && forceAllowed) {
		// delay showing the window by a little bit
		nCmdShowToUse = nCmdShow;
		SetTimer(mainWindow, 1, 8 * 1000, NULL);
	} else {
		ShowWindow(mainWindow, nCmdShow);
		UpdateWindow(mainWindow);
	}
	
	SetFocus(textEdit);
	
	CloseHandle(CreateThread(0, 0, taskThreadProc, NULL, 0, NULL));
	
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;
	switch (message) {
		case WM_COMMAND:
			onCommand(wParam, lParam, &result);
			break;
		case WM_CHAR:
			onKeyDown(wParam, &result, true);
			return DefWindowProcW(hWnd, message, wParam, lParam);
		case WM_SIZE:
			onSize(wParam, lParam, &result);
			return DefWindowProcW(hWnd, message, wParam, lParam);
		case WM_GET_LINE:
			onGetLine(wParam);
			break;
		case WM_TASK_ENDED:
			DestroyWindow(hWnd);
			break;
		case WM_OUTPUT_STRING_A:
			OutputStringAImpl((const char*)wParam);
			break;
		case WM_OUTPUT_STRING_W:
			OutputStringWImpl((const wchar_t*)wParam);
			break;
		case WM_PRESS_ANY_KEY_BEGIN:
			onPressAnyKeyBegin();
			break;
		case WM_PRESS_ANY_KEY_END:
			onPressAnyKeyEnd();
			break;
		case WM_IS_ANY_KEY_PRESSED:
			onIsAnyKeyPressed((bool*)wParam);
			break;
		case WM_TIMER:
			onTimer();
			break;
		case WM_CLOSE:
			if (!getLineLine && !wantAnyKey) {
				int answer = MessageBoxW(hWnd, L"Process is currently busy."
					L" Interrupting it is not as good as waiting for it to finish normally."
					L" Do you want to close this program anyway?", szTitle, MB_OKCANCEL);
				if (answer != IDOK) {
					break;
				}
			}
			return DefWindowProcW(hWnd, message, wParam, lParam);
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	switch (message) {
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;
		
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

void onCommand(WPARAM wParam, LPARAM lParam, LRESULT* lResult) {
	int wmId = LOWORD(wParam);
	switch (wmId) {
		case IDM_ABOUT:
			DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), mainWindow, About);
			break;
		case IDM_EXIT:
			DestroyWindow(mainWindow);
			break;
		default:
			*lResult = DefWindowProcW(mainWindow, WM_COMMAND, wParam, lParam);
	}
}

void onKeyDown(WPARAM wParam, LRESULT* lResult, bool handleEventOnYourOwn) {
	if (wParam == L'\r' && getLineLine) {
		std::vector<wchar_t> buf;
		size_t length = GetWindowTextLengthW(textEdit);
		buf.resize(length + 1);
		wchar_t* start = buf.data();
		if (buf.size() > 0xFFFFFFFF) {
			buf.erase(buf.begin(), buf.begin() + (buf.size() - 0xFFFFFFFF));
		}
		GetWindowTextW(textEdit, start, buf.size() & 0xFFFFFFFF);
		length = wcslen(start);
		wchar_t* ptr = start + length - 1;
		while (ptr >= start && *ptr != L'\n') {
			--ptr;
		}
		if (ptr < start) {
			*getLineLine = start;
		} else {
			*getLineLine = ptr + 1;
		}
		getLineLine = nullptr;
		if (handleEventOnYourOwn) {
			SendMessageW(textEdit, EM_SETREADONLY, TRUE, 0);
			SetEvent(eventToInjectorThread);
		}
	}
	if (wantAnyKey
			&& wParam != 9) {  // TAB, as in Alt+TAB
		anyKeyPressed = true;
	}
}

void onSize(WPARAM wParam, LPARAM lParam, LRESULT* lResult) {
	int newWidth = (int)LOWORD(lParam);
	int newHeight = (int)HIWORD(lParam);
	if (textEdit) {
		MoveWindow(textEdit, 5, 5, newWidth - 10, newHeight - 10, TRUE);
	}
}

int countChar(const char* text, int chr) {
	int result = 0;
	
	const char* c;
	const char* cNext = strchr(text, chr);
	while (cNext) {
		++result;
		c = cNext + 1;
		cNext = strchr(c, chr);
	}
	
	return result;
}

int countChar(const wchar_t* text, wchar_t chr) {
	int result = 0;
	
	const wchar_t* c;
	const wchar_t* cNext = wcschr(text, chr);
	while (cNext) {
		++result;
		c = cNext + 1;
		cNext = wcschr(c, chr);
	}
	
	return result;
}

inline size_t OutputString_GetLen(const char* text) {
	size_t result = strlen(text);
	int newlinesCount = countChar(text, '\n');
	return result + newlinesCount;
}

inline size_t OutputString_GetLen(const wchar_t* text) {
	size_t result = wcslen(text);
	int newlinesCount = countChar(text, L'\n');
	return result + newlinesCount;
}

inline wchar_t* OutputString_Concat(wchar_t* dest, const char* src) {
	const char* c = src;
	while (*c != '\0') {
		if (*c == '\n') {
			*dest = L'\r';
			++dest;
		}
		*dest = *c;
		++c;
		++dest;
	}
	*dest = L'\0';
	return dest;
}

inline wchar_t* OutputString_Concat(wchar_t* dest, const wchar_t* src) {
	const wchar_t* srcNext = wcschr(src, L'\n');
	while (srcNext) {
		if (src != srcNext) {
			memcpy(dest, src, (BYTE*)srcNext - (BYTE*)src);
			dest += srcNext - src;
		}
		*dest = L'\r';
		++dest;
		*dest = L'\n';
		++dest;
		src = srcNext + 1;
		srcNext = wcschr(src, L'\n');
	}
	if (*src != L'\0') {
		const wchar_t* endPos = wcschr(src, L'\0');
		memcpy(dest, src, (BYTE*)endPos - (BYTE*)src);
		dest += endPos - src;
	}
	*dest = L'\0';
	return dest;
}

void OutputStringA(const char* text) {
	PostMessageW(mainWindow, WM_OUTPUT_STRING_A, (WPARAM)text, 0);
	WaitForSingleObject(eventToInjectorThread, INFINITE);
}

void OutputStringW(const wchar_t* text) {
	PostMessageW(mainWindow, WM_OUTPUT_STRING_W, (WPARAM)text, 0);
	WaitForSingleObject(eventToInjectorThread, INFINITE);
}

void OutputStringAImpl(const char* text) {
	std::vector<wchar_t> buf;
	int length = GetWindowTextLengthW(textEdit);
	size_t textLength = OutputString_GetLen(text);
	buf.resize(length
		+ textLength
		+ 1);  // null character
	GetWindowTextW(textEdit, buf.data(), length + 1);
	buf[length] = L'\0';
	size_t newLength = wcslen(buf.data());
	newLength = OutputString_Concat(buf.data() + newLength, text) - buf.data();
	SetWindowTextW(textEdit, buf.data());
	SendMessageW(textEdit, EM_SETSEL, newLength, newLength);
	SendMessageW(textEdit, EM_SCROLLCARET, 0, 0);
	SetEvent(eventToInjectorThread);
}

void OutputStringWImpl(const wchar_t* text) {
	std::vector<wchar_t> buf;
	int length = GetWindowTextLengthW(textEdit);
	size_t textLength = OutputString_GetLen(text);
	buf.resize(length
		+ textLength
		+ 1);  // null character
	GetWindowTextW(textEdit, buf.data(), length + 1);
	buf[length] = L'\0';
	size_t newLength = wcslen(buf.data());
	newLength = OutputString_Concat(buf.data() + newLength, text) - buf.data();
	SetWindowTextW(textEdit, buf.data());
	SendMessageW(textEdit, EM_SETSEL, newLength, newLength);
	SendMessageW(textEdit, EM_SCROLLCARET, 0, 0);
	SetEvent(eventToInjectorThread);
}

void __cdecl GetLine(std::wstring& line) {
	line.clear();
	PostMessageW(mainWindow, WM_GET_LINE, (WPARAM)&line, 0);
	WaitForSingleObject(eventToInjectorThread, INFINITE);
}

LRESULT __stdcall TextEditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	bool wasNotNull = getLineLine != nullptr;
	if (hWnd == textEdit && uIdSubclass == 1 && uMsg == WM_CHAR) {
		LRESULT unused;
		onKeyDown(wParam, &unused, false);
	}
	LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	if (getLineLine == nullptr && wasNotNull) {
		SendMessageW(textEdit, EM_SETREADONLY, TRUE, 0);
		SetEvent(eventToInjectorThread);
	}
	return result;
}

void onGetLine(WPARAM wParam) {
	getLineLine = (std::wstring*)wParam;
	SendMessageW(textEdit, EM_SETREADONLY, FALSE, 0);
	SetFocus(textEdit);
}

void pressAnyKeyBegin() {
	PostMessageW(mainWindow, WM_PRESS_ANY_KEY_BEGIN, 0, 0);
}

void pressAnyKeyEnd() {
	PostMessageW(mainWindow, WM_PRESS_ANY_KEY_END, 0, 0);
}

bool isAnyKeyPressed() {
	bool theValue = false;
	PostMessageW(mainWindow, WM_IS_ANY_KEY_PRESSED, (WPARAM)&theValue, 0);
	WaitForSingleObject(eventToInjectorThread, INFINITE);
	return theValue;
}

void onPressAnyKeyBegin() {
	wantAnyKey = true;
	SendMessageW(textEdit, EM_SETREADONLY, FALSE, 0);
}

void onPressAnyKeyEnd() {
	wantAnyKey = false;
	anyKeyPressed = false;
	SendMessageW(textEdit, EM_SETREADONLY, TRUE, 0);
}

void onIsAnyKeyPressed(bool* theValue) {
	*theValue = anyKeyPressed;
	SetEvent(eventToInjectorThread);
}

void onTimer() {
	KillTimer(mainWindow, 1);
	ShowWindow(mainWindow, nCmdShowToUse);
	UpdateWindow(mainWindow);
}
