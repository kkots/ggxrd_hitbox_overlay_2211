#pragma once
#include <iostream>
#include <string>

class InjectorCommonOut {
public:
	InjectorCommonOut& operator<<(char c);
	InjectorCommonOut& operator<<(wchar_t c);
	InjectorCommonOut& operator<<(const wchar_t* wstr);
	InjectorCommonOut& operator<<(const char* str);
	InjectorCommonOut& operator<<(std::wostream& (*func)(std::wostream& in));
	InjectorCommonOut& operator<<(std::ios_base& (*func)(std::ios_base& in));
	InjectorCommonOut& operator<<(int i);
	InjectorCommonOut& operator<<(DWORD i);
	InjectorCommonOut& operator<<(LPVOID i);
private:
	void printHexDword(DWORD i);
	bool isHex = false;
};
