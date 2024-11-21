#pragma once
#include <string>

class PngRelated
{
public:
	// May overwrite data in -buffer-! Provide memory that you won't regret getting trashed
	bool writePngToPath(const std::wstring& path, unsigned int width, unsigned int height, void* buffer);
	// May overwrite data in -buffer-! Provide memory that you won't regret getting trashed
	void saveScreenshotData(unsigned int width, unsigned int height, void* buffer);
private:
	bool fileExists(const wchar_t* path) const;
	bool isDirectory(const wchar_t* path) const;
	int findRev(const wchar_t* path, wchar_t charToFind) const;
	std::wstring getScreenshotSavingPath();
	// imageDataToWrite - the source data. Destination is new memory allocated by GlobalAlloc
	// May overwrite data in -imageDataToWrite-! Provide memory that you won't regret getting trashed
	bool writePngToMemory(HGLOBAL* handleToGlobalAlloc, unsigned int width, unsigned int height, void* imageDataToWrite);
	void writePngPrepare(unsigned int width, unsigned int height, const void* buffer, unsigned int* formatField, void** newBuffer);
	void writeScreenshotToClipboard(unsigned int width, unsigned int height, void* buffer);
};

extern PngRelated pngRelated;
