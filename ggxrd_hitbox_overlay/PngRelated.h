#pragma once
#include <string>

class PngRelated
{
public:
	bool writePngToPath(const std::wstring& path, unsigned int width, unsigned int height, void* buffer);
	void saveScreenshotData(unsigned int width, unsigned int height, void* buffer);
private:
	bool fileExists(const wchar_t* path) const;
	bool isDirectory(const wchar_t* path) const;
	int findRev(const wchar_t* path, wchar_t charToFind) const;
	std::wstring getScreenshotSavingPath();
	bool writePngToMemory(HGLOBAL* handleToGlobalAlloc, unsigned int width, unsigned int height, void* imageDataToWrite);
	void writePngPrepare(unsigned int width, unsigned int height, void* buffer, unsigned int* formatField, void** newBuffer);
	void writeScreenshotToClipboard(unsigned int width, unsigned int height, void* buffer);
};

extern PngRelated pngRelated;
