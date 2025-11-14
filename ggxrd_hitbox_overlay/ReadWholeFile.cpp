#include "pch.h"
#include "ReadWholeFile.h"
#include "WError.h"
#include <stdexcept>

bool readWholeFile(std::vector<BYTE>& data, HANDLE file, bool addNullTerminator, char (&errorbuf)[1024],
		void (*readWholeFileErrorHandler)(const char* errorStr),
		const wchar_t* readWholeFilePath
) {
	DWORD fileSizeHigh;
	DWORD fileSizeLow;
	fileSizeLow = GetFileSize(file, &fileSizeHigh);
	if (fileSizeLow == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
		WinError winErr;
		sprintf_s(errorbuf, "Failed to get file size: %ls", winErr.getMessage());
		if (readWholeFileErrorHandler) readWholeFileErrorHandler(errorbuf);
		return false;
	}
	if (fileSizeHigh || fileSizeLow >= 0x7FFFFFFE) {
		sprintf_s(errorbuf, "The file size is too big: %llu bytes", ((unsigned long long)fileSizeHigh << 32) | (unsigned long long)fileSizeLow);
		if (readWholeFileErrorHandler) readWholeFileErrorHandler(errorbuf);
		return false;
	}
	
	try {
		data.resize(fileSizeLow + addNullTerminator);
	} catch (std::length_error& err) {
		(err);
		sprintf_s(errorbuf, "The file size is too big: %llu bytes", ((unsigned long long)fileSizeHigh << 32) | (unsigned long long)fileSizeLow);
		return false;
	} catch (std::bad_alloc& err) {
		(err);
		sprintf_s(errorbuf, "The file does not fit into RAM: %llu bytes", ((unsigned long long)fileSizeHigh << 32) | (unsigned long long)fileSizeLow);
		return false;
	}
	if (data.empty()) return true;
	BYTE* ptr = data.data();
	size_t remaining = fileSizeLow;
	DWORD bytesRead = 0;
	while (remaining) {
		DWORD bytesToRead = min(remaining, 1024);
		if (!ReadFile(file, ptr, bytesToRead, &bytesRead, NULL)) {
			WinError winErr;
			if (!readWholeFilePath) {
				sprintf_s(errorbuf, "Failed to read file: %ls", winErr.getMessage());
			} else {
				sprintf_s(errorbuf, "Failed to read file '%ls': %ls", readWholeFilePath, winErr.getMessage());
			}
			if (readWholeFileErrorHandler) readWholeFileErrorHandler(errorbuf);
			return false;
		}
		if (bytesToRead != bytesRead) {
			// um... ok
			remaining -= bytesRead;
			if ((int)remaining > 0) {
				data.resize(data.size() - remaining);
			}
			break;
		}
		remaining -= bytesRead;
		ptr += bytesRead;
	}
	if (addNullTerminator) data.back() = '\0';
	return true;
}
