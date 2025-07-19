#pragma once
#include "pch.h"
#include <vector>

struct PngResource {
	size_t width = 0;
	size_t height = 0;
	size_t stride = 0;
	struct PixelA {
		unsigned char blue;
		unsigned char green;
		unsigned char red;
		unsigned char alpha;
	};
	float uStart = 0.0F;
	float uEnd = 0.0F;
	float vStart = 0.0F;
	float vEnd = 0.0F;
	std::vector<PixelA> data;
	inline void resize(size_t newWidth, size_t newHeight) {
		width = newWidth;
		stride = width * sizeof(PixelA);
		height = newHeight;
		data.resize(width * height);
	}
	// replaces pixels without any kind of blend or scaling
	static void bitBlt(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride, DWORD sourceX, DWORD sourceY,
	        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height);
	// replaces pixels without any kind of blend or scaling
	inline void bitBlt(PngResource& destination, DWORD sourceX, DWORD sourceY,
	                   DWORD destinationX, DWORD destinationY, DWORD width, DWORD height) const {
		bitBlt(data.data(), stride, destination.data.data(), destination.stride,
			sourceX, sourceY, destinationX, destinationY, width, height);
	}
	// replaces pixels without any kind of blend or scaling
	inline void bitBlt(void* destination, int destinationStride, DWORD sourceX, DWORD sourceY,
	                   DWORD destinationX, DWORD destinationY, DWORD width, DWORD height) const {
		bitBlt(data.data(), stride, destination, destinationStride,
			sourceX, sourceY, destinationX, destinationY, width, height);
	}
	// Draws source image onto destination image, but:
	// 1) The source colors are 100% tinted to the specified color;
	// 2) Instead of blending colors, the destination color is simply set to the source color;
	// 3) The resulting alpha is set to max(src.alpha, dst.alpha);
	// 4) Does not support stretching or shrinking.
	static void drawTintWithMaxAlpha(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride, DWORD sourceX, DWORD sourceY,
	        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height, DWORD tint);
	inline void drawTintWithMaxAlpha(PngResource& destination, DWORD sourceX, DWORD sourceY,
	        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height, DWORD tint) const {
    	drawTintWithMaxAlpha(data.data(), stride,
    		destination.data.data(), destination.stride,
    		sourceX, sourceY,
    		destinationX, destinationY, width, height, tint);
    }
	// Draws source image onto destination image:
	// 1) Alpha of both source and destination is used to blend colors normally;
	// 2) Does not support stretching or shrinking.
	static void draw(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride, DWORD sourceX, DWORD sourceY,
		        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height);
	inline void draw(PngResource& destination, DWORD sourceX, DWORD sourceY,
		        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height) const {
		draw(data.data(), stride,
			destination.data.data(), destination.stride,
			sourceX, sourceY,
			destinationX, destinationY, width, height);
	}
	// copy source image to destination image using linear interpolation when stretching and unknown area-based approach when shrinking
	static void stretchRect(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride,
			DWORD sourceX, DWORD sourceY, DWORD sourceWidth, DWORD sourceHeight,
	        DWORD destinationX, DWORD destinationY, DWORD destinationWidth, DWORD destinationHeight);
	// copy source image to destination image using linear interpolation when stretching and unknown area-based approach when shrinking
	inline void stretchRect(PngResource& destination,
			DWORD sourceX, DWORD sourceY, DWORD sourceWidth, DWORD sourceHeight,
	        DWORD destinationX, DWORD destinationY, DWORD destinationWidth, DWORD destinationHeight) const {
		stretchRect(data.data(), stride,
			destination.data.data(), destination.stride,
			sourceX, sourceY, sourceWidth, sourceHeight,
			destinationX, destinationY, destinationWidth, destinationHeight);
	}
	static void drawRectOutline(void* destination, DWORD stride, DWORD x, DWORD y,
			DWORD width, DWORD height, DWORD color);
	inline void drawRectOutline(DWORD x, DWORD y, DWORD width, DWORD height, DWORD color) {
		drawRectOutline(data.data(), stride, x, y, width, height, color);
	}
	static void multiplyAlphaByPercent(void* destination, DWORD stride, DWORD x, DWORD y,
			DWORD width, DWORD height, int percent);
	inline void multiplyAlphaByPercent(DWORD x, DWORD y, DWORD width, DWORD height, int percent) {
		multiplyAlphaByPercent(data.data(), stride, x, y, width, height, percent);
	}
	static inline PixelA* getPixel(void* source, DWORD stride, DWORD x, DWORD y) {
		return (PixelA*)((char*)source + y * stride) + x;
	}
	static inline const PixelA* getPixel(const void* source, DWORD stride, DWORD x, DWORD y) {
		return (const PixelA*)((const char*)source + y * stride) + x;
	}
	inline PixelA* getPixel(DWORD x, DWORD y) {
		return getPixel(data.data(), stride, x, y);
	}
	inline const PixelA* getPixel(DWORD x, DWORD y) const {
		return getPixel(data.data(), stride, x, y);
	}
	static inline void incrementRow(PixelA*& ptr, DWORD stride) {
		ptr = (PixelA*)((char*)ptr + stride);
	}
	static inline void incrementRow(const PixelA*& ptr, DWORD stride) {
		ptr = (const PixelA*)((const char*)ptr + stride);
	}
	inline void incrementRow(PixelA*& ptr) {
		incrementRow(ptr, stride);
	}
	inline void incrementRow(const PixelA*& ptr) {
		incrementRow(ptr, stride);
	}
	static inline void decrementRow(PixelA*& ptr, DWORD stride) {
		ptr = (PixelA*)((char*)ptr - stride);
	}
	static inline void decrementRow(const PixelA*& ptr, DWORD stride) {
		ptr = (const PixelA*)((const char*)ptr - stride);
	}
	inline void decrementRow(PixelA*& ptr) {
		decrementRow(ptr, stride);
	}
	inline void decrementRow(const PixelA*& ptr) {
		decrementRow(ptr, stride);
	}
	// sample with linear interpolation
	static void sample(const void* source, DWORD stride, float x, float y, int xCapMin, int capWidth, int yCapMin, int capHeight, PixelA* outPixel);
	// sample with linear interpolation
	inline void sample(float x, float y, int xCapMin, int capWidth, int yCapMin, int capHeight, PixelA* outPixel) const {
		sample(data.data(), stride, x, y, xCapMin, capWidth, yCapMin, capHeight, outPixel);
	}
};

bool loadPngResource(HINSTANCE hInstance, WORD resourceSymbolId, PngResource& windowsImage);
