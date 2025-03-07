#include "pch.h"
#include "InjectorCommonOut.h"

extern void OutputStringA(const char* str);  // maybe subclassing? I think all operator overloads get lost in subclasses. We don't have to go all C++35 futuristic featureset
extern void OutputStringW(const wchar_t* str);

#ifndef FOR_LINUX
#define sprintf_s_withoutSize(buf, fmt, ...) sprintf_s(buf, fmt, __VA_ARGS__)
#define sprintf_s_withSize(buf, size, fmt, ...) sprintf_s(buf, size, fmt, __VA_ARGS__)
#else
#define sprintf_s_withoutSize(buf, fmt, ...) sprintf(buf, fmt, __VA_ARGS__)
#define sprintf_s_withSize(buf, size, fmt, ...) sprintf(buf, fmt, __VA_ARGS__)
#endif

InjectorCommonOut& InjectorCommonOut::operator<<(char c) {
	char str[2] { '\0' };
	str[0] = c;
	OutputStringA(str);
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(wchar_t c) {
	wchar_t wstr[2] { L'\0' };
	wstr[0] = c;
	OutputStringW(wstr);
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(const wchar_t* wstr) {
	OutputStringW(wstr);
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(const char* str) {
	OutputStringA(str);
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(std::wostream& (*func)(std::wostream& in)) {
	if (func == std::endl<wchar_t, std::char_traits<wchar_t>>) {
		OutputStringW(L"\n");
	}
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(std::ios_base& (*func)(std::ios_base& in)) {
	if (func == std::hex) {
		isHex = true;
	} else if (func == std::dec) {
		isHex = false;
	}
	return *this;
}
void InjectorCommonOut::printHexDword(DWORD i) {
	char sprintfbuf[9];
	sprintf_s_withSize(sprintfbuf, sizeof sprintfbuf, "%x", i);
	OutputStringA(sprintfbuf);
}
InjectorCommonOut& InjectorCommonOut::operator<<(int i) {
	if (isHex) {
		printHexDword((DWORD)i);
	} else {
		char buf[14];
		sprintf_s_withoutSize(buf, "%i", i);
		OutputStringA(buf);
	}
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(DWORD i) {
	if (isHex) {
		printHexDword(i);
	} else {
		char buf[14];
		sprintf_s_withoutSize(buf, "%u", i);
		OutputStringA(buf);
	}
	return *this;
}
InjectorCommonOut& InjectorCommonOut::operator<<(LPVOID i) {
	char buf[17];
	sprintf_s_withoutSize(buf, "%p", i);
	OutputStringA(buf);
	return *this;
}
