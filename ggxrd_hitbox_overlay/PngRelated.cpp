#include "pch.h"
#include "PngRelated.h"
#include "logging.h"
#include "Settings.h"
#include "png.h"
#include "WError.h"

PngRelated pngRelated;

bool PngRelated::fileExists(const wchar_t* path) const {
	DWORD dllAtrib = GetFileAttributesW(path);
	if (dllAtrib == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	if ((dllAtrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return false;
	}
	return true;
}

bool PngRelated::isDirectory(const wchar_t* path) const {
	DWORD dllAtrib = GetFileAttributesW(path);
	if (dllAtrib == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	if ((dllAtrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return true;
	}
	return false;
}

int PngRelated::findRev(const wchar_t* path, wchar_t charToFind) const {
	size_t pathLen = wcslen(path);
	const wchar_t* pathPtr = path + pathLen - 1;
	while (pathPtr >= path) {
		if (*pathPtr == charToFind) return pathPtr - path;
		--pathPtr;
	}
	return -1;
}

std::wstring PngRelated::getScreenshotSavingPath() {
	if (settings.ignoreScreenshotPathAndSaveToClipboard) return std::wstring{};
	std::wstring pathUncounted;
	{
		std::unique_lock<std::mutex> guard(settings.screenshotPathMutex);
		if (settings.screenshotPath.empty()) {
			return std::wstring{};
		}
		pathUncounted.resize(settings.screenshotPath.size());
		MultiByteToWideChar(CP_UTF8, 0, settings.screenshotPath.c_str(), -1, &pathUncounted.front(), pathUncounted.size());
	}
	pathUncounted.resize(wcslen(pathUncounted.c_str()));

	int backslashPos = findRev(pathUncounted.c_str(), L'\\');
	if (isDirectory(pathUncounted.c_str())) {
		if (backslashPos == -1 || backslashPos != pathUncounted.size() - 1) {
			pathUncounted += L'\\';
		}
		pathUncounted += L"screen.png";
	}

	std::wstring extension;
	int dotPos = findRev(pathUncounted.c_str(), L'.');
	if (dotPos != -1 && (backslashPos == -1 || dotPos > backslashPos)) {
		extension.insert(extension.begin(), pathUncounted.begin() + dotPos, pathUncounted.end());
		pathUncounted.resize(dotPos);
	}
	else {
		logwrap(fputs("Dot not found in pathUncounted\n", logfile));
	}

	std::wstring path = pathUncounted + extension;
	unsigned int pathCounter = 1;
	while (fileExists(path.c_str())) {
		path = pathUncounted + std::to_wstring(pathCounter) + extension;
		++pathCounter;
	}
	return path;
}

void PngRelated::writePngPrepare(unsigned int width, unsigned int height, const void* buffer, unsigned int* formatField, void** newBuffer) {
	if (!settings.dontUseScreenshotTransparency) {
		*formatField = PNG_FORMAT_BGRA;
		return;
	}
	*formatField = PNG_FORMAT_BGR;
	*newBuffer = malloc(width * height * 3);
	if (!*newBuffer) return;
	struct BGR { unsigned char b; unsigned char g; unsigned char r; };
	struct BGRA { BGR bgr; unsigned char a; };
	const BGRA* oldBufferPtr = (const BGRA*)buffer;
	BGR* newBufferPtr = (BGR*)*newBuffer;
	const unsigned int imageSize = width * height;
	for (unsigned int i = imageSize; i != 0; --i) {
		*newBufferPtr = oldBufferPtr->bgr;
		++oldBufferPtr;
		++newBufferPtr;
	}
}

// May overwrite data in -buffer-
bool PngRelated::writePngToPath(const std::wstring& path, unsigned int width, unsigned int height, void* buffer) {
	png_image image;
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;
	image.width = width;
	image.height = height;
	void* newBuffer = nullptr;
	writePngPrepare(width, height, buffer, &image.format, &newBuffer);
	void* finalBuffer = newBuffer ? newBuffer : buffer;
	FILE* file = nullptr;
	if (_wfopen_s(&file, path.c_str(), L"wb") || !file) {
		if (file) fclose(file);
		logwrap(fputs("Could not open file for screenshot writing\n", logfile));
		return false;
	}
	if (png_image_write_to_stdio(&image, file, 0, finalBuffer, 0, NULL) == 0) {
		logwrap(fputs("png_image_write_to_stdio failed\n", logfile));
		return false;
	}
	else {
		fflush(file);
	}
	fclose(file);
	if (newBuffer) free(newBuffer);
	return true;
}

// imageDataToWrite - the source data. Destination is new memory allocated by GlobalAlloc
// May overwrite data in -imageDataToWrite-
bool PngRelated::writePngToMemory(HGLOBAL* handleToGlobalAlloc, unsigned int width, unsigned int height, void* imageDataToWrite) {
	if (!handleToGlobalAlloc) return false;
	if (!imageDataToWrite) return false;

	png_image image;
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;
	image.width = width;
	image.height = height;
	void* newBuffer = nullptr;
	writePngPrepare(width, height, imageDataToWrite, &image.format, &newBuffer);
	void* finalBuffer = newBuffer ? newBuffer : imageDataToWrite;

	png_alloc_size_t size;

	if (!png_image_write_get_memory_size(image, size, 0, finalBuffer, 0, NULL)) {
		logwrap(fputs("png_image_write_get_memory_size failed\n", logfile));
		return false;
	}

	if (size > PNG_IMAGE_PNG_SIZE_MAX(image)) {
		logwrap(fputs("size returned by png_image_write_get_memory_size exceeds PNG_IMAGE_PNG_SIZE_MAX\n", logfile));
		return false;
	}

	*handleToGlobalAlloc = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!*handleToGlobalAlloc) {
		logwrap(fputs("Error in GlobalAlloc inside writePngToMemory\n", logfile));
		return false;
	}

	LPVOID globalWriteLock = GlobalLock(*handleToGlobalAlloc);
	int writeResult = png_image_write_to_memory(&image, globalWriteLock, &size, 0, finalBuffer, 0, NULL);
	GlobalUnlock(*handleToGlobalAlloc);
	if (newBuffer) free(newBuffer);

	if (!writeResult) {
		logwrap(fputs("png_image_write_to_memory failed\n", logfile));
		return false;
	}
	return true;
}

// May overwrite data in -buffer-
void PngRelated::saveScreenshotData(unsigned int width, unsigned int height, void* buffer) {
	bool screenshotPathEmpty = false;
	if (settings.ignoreScreenshotPathAndSaveToClipboard) {
		screenshotPathEmpty = true;
	} else {
		std::unique_lock<std::mutex> guard(settings.screenshotPathMutex);
		screenshotPathEmpty = settings.screenshotPath.empty();
	}
	if (screenshotPathEmpty) {
		logwrap(fputs("Need to write screenshot to clipboard\n", logfile));
		writeScreenshotToClipboard(width, height, buffer);
	}
	else {
		logwrap(fputs("Need to save screenshot to a path\n", logfile));
		std::wstring path = getScreenshotSavingPath();
		if (!path.empty()) {
			writePngToPath(path, width, height, buffer);
		}
	}
}

void PngRelated::writeScreenshotToClipboard(unsigned int width, unsigned int height, void* buffer) {
	if (!OpenClipboard(0)) {
		WinError winErr;
		logwrap(fprintf(logfile, "OpenClipboard failed: %ls\n", winErr.getMessage()));
		return;
	}
	class ClipboardCloser {
	public:
		~ClipboardCloser() {
			if (!closedAlready) {
				CloseClipboard();
			}
		}
		bool closedAlready = false;
	} clipboardCloser;
	if (!EmptyClipboard()) {
		WinError winErr;
		logwrap(fprintf(logfile, "EmptyClipboard failed: %ls\n", winErr.getMessage()));
		return;
	}
	BITMAPINFO bitmapInfo;
	memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 24;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFO) + width * height * 3);
	if (!hg) {
		WinError winErr;
		logwrap(fprintf(logfile, "GlobalAlloc failed: %ls\n", winErr.getMessage()));
		return;
	}
	LPVOID hgLock = GlobalLock(hg);
	if (!hgLock) {
		WinError winErr;
		logwrap(fprintf(logfile, "GlobalLock failed: %ls\n", winErr.getMessage()));
		return;
	}
	memcpy(hgLock, &bitmapInfo, sizeof(BITMAPINFO));
	unsigned char* hgLockPtr = (unsigned char*)hgLock + sizeof(BITMAPINFO);
	unsigned int oneLineWidth = width * 4;
	unsigned char* bufferPtr = (unsigned char*)buffer + width * height * 4 - oneLineWidth;
	if (!settings.dontUseScreenshotTransparency) {
		for (unsigned int i = 0; i < height; ++i) {
			unsigned char* bufferWidthPtr = bufferPtr;
			for (unsigned int j = 0; j < width; ++j) {
				const unsigned char alpha = *(bufferWidthPtr + 3);
				*hgLockPtr = (unsigned int)*(bufferWidthPtr + 1) * alpha / 0xFF;
				*(hgLockPtr + 1) = (unsigned int)*(bufferWidthPtr + 2) * alpha / 0xFF;
				*(hgLockPtr + 2) = (unsigned int)*bufferWidthPtr * alpha / 0xFF;
				hgLockPtr += 3;
				bufferWidthPtr += 4;
			}
			bufferPtr -= oneLineWidth;
		}
	} else {
		for (unsigned int i = 0; i < height; ++i) {
			unsigned char* bufferWidthPtr = bufferPtr;
			for (unsigned int j = 0; j < width; ++j) {
				*hgLockPtr = (unsigned int)*(bufferWidthPtr + 1);
				*(hgLockPtr + 1) = (unsigned int)*(bufferWidthPtr + 2);
				*(hgLockPtr + 2) = (unsigned int)*bufferWidthPtr;
				hgLockPtr += 3;
				bufferWidthPtr += 4;
			}
			bufferPtr -= oneLineWidth;
		}
	}
	GlobalUnlock(hg);
	if (!SetClipboardData(CF_DIB, hg)) {
		WinError winErr;
		logwrap(fprintf(logfile, "SetClipboardData on BMP failed: %ls\n", winErr.getMessage()));
		return;
	}

	HGLOBAL pngMemory = nullptr;
	if (!settings.dontUseScreenshotTransparency) {
		UINT pngClipboardFormat = RegisterClipboardFormatA("PNG");
		if (!writePngToMemory(&pngMemory, width, height, buffer)) {
			logwrap(fputs("writePngToMemory failed\n", logfile));
			return;
		}
		if (!SetClipboardData(pngClipboardFormat, pngMemory)) {
			WinError winErr;
			logwrap(fprintf(logfile, "SetClipboardData on PNG failed: %ls\n", winErr.getMessage()));
			return;
		}
	}

	clipboardCloser.closedAlready = true;
	CloseClipboard();
	GlobalFree(hg);
	if (pngMemory) GlobalFree(pngMemory);
	logwrap(fputs("Wrote screenshot to clipboard\n", logfile));
}
