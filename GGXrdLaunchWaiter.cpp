#include <Windows.h>
#include <string>
#include <TlHelp32.h>

enum xrdWindowOpenResult {
    xrdWindowOpenResult_windowFound,
    xrdWindowOpenResult_windowNotFound,
    xrdWindowOpenResult_error
};

void printError(const wchar_t* whatTriedToDo, DWORD code) {
    LPWSTR message = NULL;
    FormatMessageW(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM |
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    code,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    (LPWSTR)(&message),
	    0, NULL);
    if (message) {
        wprintf_s(L"%s: %s\n", whatTriedToDo, message);
        LocalFree(message);
    }
}

xrdWindowOpenResult xrdWindowOpen() {
    // FindWindowW is unimplemented on Proton 8.0
    
    static BOOL foundWindow = FALSE;
    
    struct Closer {
        ~Closer() {
            if (snapshot) CloseHandle(snapshot);
        }
        HANDLE snapshot;
        
        static BOOL CALLBACK enumProc(HWND window, LPARAM param) {
            DWORD processId;
            GetWindowThreadProcessId(window, &processId);
            
            if (processId == param) {
                foundWindow = TRUE;
                return FALSE;
            }
            
            return TRUE;
        }
    } closer;
    
    closer.snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!closer.snapshot) {
        DWORD code = GetLastError();
        if (code) {
            printError(L"Failed to call CreateToolhelp32Snapshot", code);
        } else {
            printf("Failed to call CreateToolhelp32Snapshot.\n");
        }
        return xrdWindowOpenResult_error;
    }
    
    PROCESSENTRY32W process;
    process.dwSize = sizeof PROCESSENTRY32W;
    if (!Process32FirstW(closer.snapshot, &process)) {
        DWORD code = GetLastError();
        if (code == ERROR_NO_MORE_FILES) {
            return xrdWindowOpenResult_windowNotFound;
        } else {
            if (code) {
                printError(L"Failed to call Process32FirstW", code);
            } else {
                printf("Failed to call Process32FirstW.\n");
            }
            return xrdWindowOpenResult_error;
        }
    }
    
    while (true) {
        if (_wcsicmp(process.szExeFile, L"GuiltyGearXrd.exe") == 0) {
            EnumWindows(Closer::enumProc, process.th32ProcessID);
            BOOL isFound = foundWindow;
            foundWindow = FALSE;
            if (isFound) {
                return xrdWindowOpenResult_windowFound;
            }
            return xrdWindowOpenResult_windowNotFound;
        }
        if (!Process32NextW(closer.snapshot, &process)) {
            DWORD code = GetLastError();
            if (code == ERROR_NO_MORE_FILES) {
                return xrdWindowOpenResult_windowNotFound;
            } else {
                if (code) {
                    printError(L"Failed to call Process32NextW", code);
                } else {
                    printf("Failed to call Process32NextW.\n");
                }
                return xrdWindowOpenResult_error;
            }
        }
    }
}

int wmain(int argc, wchar_t** argv) {
    
    #define DEFAULT_SECONDS 30
    #define HELPER1(x) #x
    #define HELPER2(x) HELPER1(x)
    #define DEFAULT_SECONDS_STR HELPER2(DEFAULT_SECONDS)
    #define SPECIFYING "specifying the maximum number of seconds to wait (default is " DEFAULT_SECONDS_STR ")"
    int seconds = DEFAULT_SECONDS;
    bool outputtedSomeShit = false;
    wchar_t** argPtr = argv;
    for (int argInd = 0; argInd < argc; ++argInd) {
        if (argInd > 1) {
            outputtedSomeShit = true;
            printf("Why is there a second argument? Only one is expected, " SPECIFYING ".\n");
            break;
        } else if (argInd == 1) {
            if (wcscmp(*argPtr, L"/?") == 0 || _wcsicmp(*argPtr, L"--help") == 0) {
                printf("Expects an optional argument, " SPECIFYING ". Return success if Xrd is open by the end, failure if not.\n");
                return 0;
            }
            int parsedValue = 0;
            wchar_t parsedChar = L'\0';
            int numParsed = swscanf_s(*argPtr, L"%d%c", &parsedValue, &parsedChar, 1);
            if (numParsed <= 0) {
                outputtedSomeShit = true;
                printf("Failed to parse the (round) number argument, " SPECIFYING ", because this is not a whole number.\n");
            } else if (numParsed == 2 && !(parsedChar == L'\0' || isspace(parsedChar))) {
                outputtedSomeShit = true;
                printf("Failed to parse the (round) number argument, " SPECIFYING ", because the number is followed by an extra character.\n");
            } else if (parsedValue < 0) {
                outputtedSomeShit = true;
                printf("Failed to parse the (round) number argument, " SPECIFYING ", because the number is negative.\n");
            } else {
                seconds = parsedValue;
            }
        }
        ++argPtr;
    }
	
	if (outputtedSomeShit) {
	    printf("Using value %d and continuing.\n", seconds);
	}
	
	int second;
	for (second = 0; second < seconds; ++second) {
	    xrdWindowOpenResult windowResult = xrdWindowOpen();
	    if (windowResult == xrdWindowOpenResult_windowFound) return 0;
	    if (windowResult == xrdWindowOpenResult_error) {
	        return 2;
	    }
	    Sleep(1000);
	}
	
    return 1;
}
