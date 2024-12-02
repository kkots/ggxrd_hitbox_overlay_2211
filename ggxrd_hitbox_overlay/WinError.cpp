#include "pch.h"
#include "WinError.h"
#include <iostream>

// This is an almost exact copy-paste of the WinError.cpp file from ggxrd_hitbox_injector project

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
		size_t len = wcslen(src.message);
		message = (LPWSTR)LocalAlloc(0, (len + 1) * sizeof(wchar_t));
		if (message) {
			memcpy(message, src.message, (len + 1) * sizeof(wchar_t));
		}
		else {
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
LPCWSTR WinError::getMessage() {
	if (!message) {
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)(&message),
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