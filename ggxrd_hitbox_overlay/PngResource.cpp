#include "pch.h"
#include "PngResource.h"
#include "WinError.h"
#include "png.h"
#include <vector>
#include "logging.h"

bool loadPngResource(HINSTANCE hInstance, WORD resourceSymbolId, PngResource& pngResource) {

	HRSRC resourceInfoHandle = FindResourceW(hInstance, MAKEINTRESOURCEW(resourceSymbolId), L"PNG");
	if (!resourceInfoHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "FindResource failed: %ls\n", winErr.getMessage()));
		return false;
	}
	HGLOBAL resourceHandle = LoadResource(hInstance, resourceInfoHandle);
	if (!resourceHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "LoadResource failed: %ls\n", winErr.getMessage()));
		return false;
	}
	LPVOID pngBtnData = LockResource(resourceHandle);
	if (!pngBtnData) {
		WinError winErr;
		logwrap(fprintf(logfile, "LockResource failed: %ls\n", winErr.getMessage()));
		return false;
	}
	DWORD size = SizeofResource(hInstance, resourceInfoHandle);
	if (!size) {
		WinError winErr;
		logwrap(fprintf(logfile, "SizeofResource failed: %ls\n", winErr.getMessage()));
		return false;
	}
	png_image image;
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_memory(&image, pngBtnData, size)) {
		WinError winErr;
		logwrap(fputs("png_image_begin_read_from_memory failed.\n", logfile));
		return false;
	}
	image.format = PNG_FORMAT_BGRA;
	pngResource.width = image.width;
	pngResource.height = image.height;
	size_t stride = PNG_IMAGE_ROW_STRIDE(image);  // we can specify a larger stride if we want and libpng will align rows to that
	pngResource.data.resize(image.width * image.height);
	if (!png_image_finish_read(&image, nullptr, &pngResource.data.front(), stride, nullptr)) {
		WinError winErr;
		logwrap(fputs("png_image_finish_read failed.\n", logfile));
		return false;
	}
	return true;
}

void PngResource::bitBlt(PngResource& destination, unsigned int destinationX, unsigned int destinationY, unsigned int sourceX, unsigned int sourceY, unsigned int sourceWidth, unsigned int sourceHeight) const {
	PixelA* destinationRowPtr = &destination.data.front() + destinationY * destination.width;
	const PixelA* sourceRowPtr = &data.front() + sourceY * width;
	for (unsigned int heightCounter = sourceHeight; heightCounter != 0; --heightCounter) {
		memcpy(destinationRowPtr + destinationX, sourceRowPtr + sourceX, sourceWidth * sizeof(PixelA));
		sourceRowPtr += width;
		destinationRowPtr += destination.width;
	}
}

void PngResource::bitBlt(void* destination, int destinationStride, unsigned int destinationX, unsigned int destinationY, unsigned int sourceX, unsigned int sourceY, unsigned int sourceWidth, unsigned int sourceHeight) const {
	PixelA* destinationRowPtr = (PixelA*)((char*)destination + destinationY * destinationStride);
	const PixelA* sourceRowPtr = &data.front() + sourceY * width;
	for (unsigned int heightCounter = sourceHeight; heightCounter != 0; --heightCounter) {
		memcpy(destinationRowPtr + destinationX, sourceRowPtr + sourceX, sourceWidth * sizeof(PixelA));
		sourceRowPtr += width;
		destinationRowPtr = (PixelA*)((char*)destinationRowPtr + destinationStride);
	}
}
