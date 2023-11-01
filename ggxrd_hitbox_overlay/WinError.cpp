#include "pch.h"
#include "WinError.h"
#include <iostream>
#include <tchar.h>

// This is an almost exact copy-paste of the WinError.cpp file from ggxrd_hitbox_injector project

#ifndef tout
#ifdef UNICODE
#define tout std::wcout
#else
#define tout std::cout
#endif
#endif

WinError::WinError() {
    code = GetLastError();
}
void WinError::moveFrom(WinError& src) noexcept {
    message = src.message;
    code = src.code;
    src.message = NULL;
    src.code = 0;
}
void WinError::copyFrom(const WinError& src) {
    code = src.code;
    if (src.message) {
        size_t len = _tcslen(src.message);
        message = (LPTSTR)LocalAlloc(0, (len + 1) * sizeof(TCHAR));
        if (message) {
            memcpy(message, src.message, (len + 1) * sizeof(TCHAR));
        }
        else {
            WinError winErr;
            tout << TEXT("Error in LocalAlloc: ") << winErr.getMessage();
            return;
        }
    }
}
WinError::WinError(const WinError& src) {
    copyFrom(src);
}
WinError::WinError(WinError&& src) noexcept {
    moveFrom(src);
}
LPCTSTR WinError::getMessage() {
    if (!message) {
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)(&message),
            0, NULL);
    }

    
    return message;
}
void WinError::clear() {
    if (message) {
        LocalFree(message);
        message = NULL;
    }
}
WinError::~WinError() {
    clear();
}
WinError& WinError::operator=(const WinError& src) {
    clear();
    copyFrom(src);
    return *this;
}
WinError& WinError::operator=(WinError&& src) noexcept {
    clear();
    moveFrom(src);
    return *this;
}