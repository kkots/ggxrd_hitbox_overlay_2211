#pragma once
#include "pch.h"

class WinError {
public:
	LPWSTR message = NULL;
	LPSTR messageA = NULL;
	DWORD code = 0;
	WinError();
	void moveFrom(WinError& src) noexcept;
	void copyFrom(const WinError& src);
	WinError(const WinError& src);
	WinError(WinError&& src) noexcept;
	LPCWSTR getMessage();
	LPCSTR getMessageA();
	void clear();
	~WinError();
	WinError& operator=(const WinError& src);
	WinError& operator=(WinError&& src) noexcept;
};
