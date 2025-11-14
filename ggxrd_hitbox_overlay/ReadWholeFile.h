#pragma once
#include "pch.h"
#include <vector>

bool readWholeFile(std::vector<BYTE>& data, HANDLE file, bool addNullTerminator, char (&errorbuf)[1024],
	void (*readWholeFileErrorHandler)(const char* errorStr) = nullptr,
	const wchar_t* readWholeFilePath = nullptr);