#include <Windows.h>
#include <iostream>

#define PATH_TO_CHECK_MODULE_IS_LOADED_EXE // please specify the path here

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
        fprintf(stderr, ": %s", message);
        LocalFree(message);
    }
}

int main()
{
    STARTUPINFOEXA startupInfo;
    SecureZeroMemory(&startupInfo, sizeof STARTUPINFOEXA);
    startupInfo.StartupInfo.cb = sizeof STARTUPINFOEXA;
    startupInfo.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    SECURITY_ATTRIBUTES securityAttribues;
    securityAttribues.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttribues.bInheritHandle = TRUE;
    securityAttribues.lpSecurityDescriptor = NULL;
    HANDLE readPipeIn;
    HANDLE writePipeIn;
    if (!CreatePipe(&readPipeIn, &writePipeIn, &securityAttribues, 0)) {
        printError("Failed to call CreatePipe", GetLastError());
        return 1;
    }
    if (!SetHandleInformation(writePipeIn, HANDLE_FLAG_INHERIT, 0)) {
        printError("Failed to call SetHandleInformation", GetLastError());
        return 1;
    }
    HANDLE readPipeOut;
    HANDLE writePipeOut;
    if (!CreatePipe(&readPipeOut, &writePipeOut, &securityAttribues, 0)) {
        printError("Failed to call CreatePipe", GetLastError());
        return 1;
    }
    if (!SetHandleInformation(readPipeOut, HANDLE_FLAG_INHERIT, 0)) {
        printError("Failed to call SetHandleInformation", GetLastError());
        return 1;
    }
    HANDLE readPipeErr;
    HANDLE writePipeErr;
    if (!CreatePipe(&readPipeErr, &writePipeErr, &securityAttribues, 0)) {
        printError("Failed to call CreatePipe", GetLastError());
        return 1;
    }
    if (!SetHandleInformation(readPipeErr, HANDLE_FLAG_INHERIT, 0)) {
        printError("Failed to call SetHandleInformation", GetLastError());
        return 1;
    }
    struct Cleanup {
        HANDLE handles[8] { NULL };
        ~Cleanup() {
            for (int i = 0; i < _countof(handles); ++i) {
                if (handles[i]) CloseHandle(handles[i]);
            }
        }
    } cleanup;
    cleanup.handles[0] = readPipeIn;
    cleanup.handles[1] = writePipeIn;
    cleanup.handles[2] = readPipeOut;
    cleanup.handles[3] = writePipeOut;
    cleanup.handles[4] = readPipeErr;
    cleanup.handles[5] = writePipeErr;
    startupInfo.StartupInfo.hStdInput = readPipeIn;
    startupInfo.StartupInfo.hStdOutput = writePipeOut;
    startupInfo.StartupInfo.hStdError = writePipeErr;
    PROCESS_INFORMATION processInformation;
    if (!CreateProcessA(
            PATH_TO_CHECK_MODULE_IS_LOADED_EXE,
            NULL, NULL, NULL, TRUE, NULL, NULL, NULL, (STARTUPINFOA*)&startupInfo, &processInformation)) {
        printError("Failed to call CreateProcessA", GetLastError());
        return 1;
    }
    cleanup.handles[0] = processInformation.hProcess;
    cleanup.handles[1] = processInformation.hThread;
    CloseHandle(readPipeIn);
    CloseHandle(writePipeOut);
    CloseHandle(writePipeErr);
    cleanup.handles[2] = writePipeIn;
    cleanup.handles[3] = readPipeOut;
    cleanup.handles[4] = readPipeErr;
    cleanup.handles[5] = NULL;
    
    DWORD bytesWritten = 0;
    const char* hitboxOverlay = "ggxrd_hitbox_overlay\n";
    Sleep(5000);
    WriteFile(writePipeIn, hitboxOverlay, strlen(hitboxOverlay), &bytesWritten, NULL);
    char response[1024];
    DWORD bytesRead = 0;
    char* responsePtr = response;
    int responseRemainingSize = _countof(response);
    while (true) {
    	char nextChar = '\0';
	    if (!ReadFile(readPipeOut, &nextChar, 1, &bytesRead, NULL)) {
	    	puts("ReadFile hitbox overlay failed.");
	    	return 0;
	    }
	    if (nextChar == '\n') {
	    	if (responseRemainingSize == 0) {
	    		puts("Ran out of space in response buffer.");
	    		return 0;
	    	}
	    	if (responsePtr > response && *(responsePtr - 1) == '\r') {
	    		*(responsePtr - 1) = '\0';
	    	} else {
	    		*responsePtr = '\0';
	    	}
	    	puts(response);
		    const char* displayPing = "GGXrdDisplayPing\n";
		    WriteFile(writePipeIn, displayPing, strlen(displayPing), &bytesWritten, NULL);
		    responsePtr = response;
		    responseRemainingSize = _countof(response);
		    while (true) {
		    	nextChar = '\0';
			    if (!ReadFile(readPipeOut, &nextChar, 1, &bytesRead, NULL)) {
			    	puts("ReadFile hitbox overlay failed.");
			    	return 0;
			    }
			    if (nextChar == '\n') {
			    	if (responseRemainingSize == 0) {
			    		puts("Ran out of space in response buffer.");
			    		return 0;
			    	}
			    	if (responsePtr > response && *(responsePtr - 1) == '\r') {
			    		*(responsePtr - 1) = '\0';
			    	} else {
			    		*responsePtr = '\0';
			    	}
			    	puts(response);
			    	return 0;
			    } else if (responseRemainingSize <= 1) {  // one char I always need to put the terminating null
		    		puts("Ran out of space in response buffer.");
		    		return 0;
			    } else {
			    	*responsePtr = nextChar;
			    	++responsePtr;
			    	--responseRemainingSize;
			    }
		    }
	    } else if (responseRemainingSize <= 1) {  // one char I always need to put the terminating null
    		puts("Ran out of space in response buffer.");
    		return 0;
	    } else {
	    	*responsePtr = nextChar;
	    	++responsePtr;
	    	--responseRemainingSize;
	    }
    }
    return 0;
}
