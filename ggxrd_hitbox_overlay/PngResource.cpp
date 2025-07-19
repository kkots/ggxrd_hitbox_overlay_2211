#include "pch.h"
#include "PngResource.h"
#include "WError.h"
#include "png.h"
#include <vector>
#include "logging.h"
#include <cmath>

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
	pngResource.resize(image.width, image.height);
	size_t stride = PNG_IMAGE_ROW_STRIDE(image);  // we can specify a larger stride if we want and libpng will align rows to that
	if (!png_image_finish_read(&image, nullptr, &pngResource.data.front(), stride, nullptr)) {
		WinError winErr;
		logwrap(fputs("png_image_finish_read failed.\n", logfile));
		return false;
	}
	return true;
}

void PngResource::bitBlt(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride,
			DWORD sourceX, DWORD sourceY,
	        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height) {
	PixelA* destinationRowPtr = getPixel(destination, destinationStride, destinationX, destinationY);
	const PixelA* sourceRowPtr = getPixel(source, sourceStride, sourceX, sourceY);
	const DWORD copySize = width * sizeof(PixelA);
	for (unsigned int heightCounter = height; heightCounter != 0; --heightCounter) {
		memcpy(destinationRowPtr, sourceRowPtr, copySize);
		incrementRow(sourceRowPtr, sourceStride);
		incrementRow(destinationRowPtr, destinationStride);
	}
}

// Draws source image onto destination image, but:
// 1) The source colors are 100% tinted to the specified color;
// 2) Instead of blending colors, the destination color is simply set to the source color;
// 3) The resulting alpha is set to max(src.alpha, dst.alpha);
// 4) Does not support stretching or shrinking.
void PngResource::drawTintWithMaxAlpha(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride, DWORD sourceX, DWORD sourceY,
	        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height, DWORD tint) {
	PixelA* destinationRowPtr = getPixel(destination, destinationStride, destinationX, destinationY);
	const PixelA* sourceRowPtr = getPixel(source, sourceStride, sourceX, sourceY);
	tint = tint & 0x00FFFFFF;
	// tint alpha is ignored, we use 100% tint
	for (unsigned int heightCounter = height; heightCounter != 0; --heightCounter) {
		PixelA* destinationPtr = destinationRowPtr;
		const PixelA* sourcePtr = sourceRowPtr;
		for (unsigned int widthCounter = width; widthCounter != 0; --widthCounter) {
			*(DWORD*)destinationPtr = tint | max(destinationPtr->alpha, sourcePtr->alpha) << 24;
			++sourcePtr;
			++destinationPtr;
		}
		incrementRow(sourceRowPtr, sourceStride);
		incrementRow(destinationRowPtr, destinationStride);
	}
}

// Draws source image onto destination image:
// 1) Alpha of both source and destination is used to blend colors normally;
// 2) Does not support stretching or shrinking.
void PngResource::draw(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride, DWORD sourceX, DWORD sourceY,
	        DWORD destinationX, DWORD destinationY, DWORD width, DWORD height) {
	PixelA* destinationRowPtr = getPixel(destination, destinationStride, destinationX, destinationY);
	const PixelA* sourceRowPtr = getPixel(source, sourceStride, sourceX, sourceY);
	for (unsigned int heightCounter = height; heightCounter != 0; --heightCounter) {
		PixelA* destinationPtr = destinationRowPtr;
		const PixelA* sourcePtr = sourceRowPtr;
		for (unsigned int widthCounter = width; widthCounter != 0; --widthCounter) {
			if (sourcePtr->alpha != 0) {
				if (destinationPtr->alpha == 0) {
					*destinationPtr = *sourcePtr;
				} else {
					int alphaTogether = 255 * 255 - (255 - sourcePtr->alpha) * (255 - destinationPtr->alpha);
					int mix;
					
					// How I arrived to this.
					// When blending onto a destination that has no alpha:
					// x' = x_dest * (1 - a) + x_src * a;
					// If we blend onto this some new source:
					// x'' = x' * (1 - a) + x_src * a;
					// Let's rename the old a to a_1, new a to a_2, and old x_src to x_src_1 and new x_src to x_src_2, and expand x':
					// x'' = (x_dest * (1 - a_1) + x_src_1 * a_1) * (1 - a_2) + x_src_2 * a_2;
					// If we expand things, we arrive at:
					// x'' = x_dest * (1 - a_1) * (1 - a_2) + x_src_1 * a_1 * (1 - a_2) + x_src_2 * a_2;
					// Remember the very first formula? If we want a transparent picture that is a blend of x_src_2 onto x_src_1,
					// preserving alpha, then, whatever it is, when applied to the x_dest from the very first formula, it
					// must give the same result as what we just got.
					// This means that, right off the bat, the alpha of the x_src_2 blended onto x_src_1 is 1 - (1 - a_1) * (1 - a_2),
					// and its color part * its alpha is the thing on the right, the two polynomial members.
					// So to get the color part we need to divide the thing on the right by the new alpha.
					// Worse Than You arrived at a different formula in CPU Pixel Blender in Graphics.cpp, and wikipedia provides
					// a different one as well, and to this day, I am still confused by their results. They just lerp (linearly interpolate),
					// and then lerp again
					
					#define blendColors(name) \
						mix = ( \
							destinationPtr->name * destinationPtr->alpha * (255 - sourcePtr->alpha) + sourcePtr->name * sourcePtr->alpha * 255 \
						) / alphaTogether; \
						if (mix > 0xFF) mix = 0xFF; \
						destinationPtr->name = mix;
					
					blendColors(red)
					blendColors(green)
					blendColors(blue)
					#undef blendColors
					
					destinationPtr->alpha = alphaTogether / 255;
				}
			}
			++sourcePtr;
			++destinationPtr;
		}
		incrementRow(sourceRowPtr, sourceStride);
		incrementRow(destinationRowPtr, destinationStride);
	}
}

// copy source image to destination image using linear interpolation when stretching and unknown area-based approach when shrinking
void PngResource::stretchRect(const void* source, DWORD sourceStride, void* destination, DWORD destinationStride,
                              DWORD sourceX, DWORD sourceY, DWORD sourceWidth, DWORD sourceHeight,
                              DWORD destinationX, DWORD destinationY, DWORD destinationWidth, DWORD destinationHeight) {
	if (destinationWidth == sourceWidth && destinationHeight == sourceHeight) {
		bitBlt(source, sourceStride, destination, destinationStride, sourceX, sourceY,
			destinationX, destinationY, sourceWidth, sourceHeight);
		return;
	}
	PixelA* destinationRowPtr = getPixel(destination, destinationStride, destinationX, destinationY);
	PixelA* destinationPtr;
	float widthRatio = (float)sourceWidth / (float)destinationWidth;
	float heightRatio = (float)sourceHeight / (float)destinationHeight;
	
	if (destinationWidth <= sourceWidth && destinationHeight <= sourceHeight) {
		const PixelA* sourceRowPtr;
		const PixelA* sourceRowPtrStart;
		const PixelA* sourcePtr;
		float top, left, bottom, right;  // in src
		top = (float)sourceY;
		float leftStart = (float)sourceX;
		bottom = top + heightRatio;
		float rightStart = leftStart + widthRatio;
		float redSum, greenSum, blueSum, alphaSum, totalArea;
		DWORD srcRowStart, srcRowEnd, srcColumnStart, srcColumnEnd;
		int srcRowStartInt, srcRowEndInt, srcColumnStartInt, srcColumnEndInt;
		float areaTop, areaBottom, areaLeft, areaRight;
		float areaHeight, areaWidth;
		float limitedTop, limitedBottom, limitedLeft, limitedRight;
		float areaTopStart, areaBottomStart, areaLeftStart, areaRightStart;
		for (DWORD row = 0; row < destinationHeight; ++row) {
			left = leftStart;
			right = rightStart;
			areaTopStart = std::floorf(top);
			areaBottomStart = areaTopStart + 1.F;
			srcRowStartInt = (int)top;
			if (srcRowStartInt < 0) srcRowStartInt = 0;
			srcRowStart = (DWORD)srcRowStartInt;
			if (srcRowStart < sourceY) srcRowStart = sourceY;
			srcRowEndInt = (int)std::ceilf(bottom);
			if (srcRowEndInt < 0) srcRowEndInt = 0;
			srcRowEnd = (DWORD)srcRowEndInt;
			if (srcRowEnd > sourceHeight) srcRowEnd = sourceHeight;
			sourceRowPtrStart = getPixel(source, sourceStride, 0, srcRowStart);
			destinationPtr = destinationRowPtr;
			for (DWORD column = 0; column < destinationWidth; ++column) {
				redSum = 0.F;
				greenSum = 0.F;
				blueSum = 0.F;
				alphaSum = 0.F;
				totalArea = 0.F;
				
				areaTop = areaTopStart;
				areaBottom = areaBottomStart;
				
				areaLeftStart = std::floorf(left);
				srcColumnStartInt = (int)areaLeftStart;
				if (srcColumnStartInt < 0) srcColumnStartInt = 0;
				srcColumnStart = (DWORD)srcColumnStartInt;
				if (srcColumnStart < sourceX) srcColumnStart = sourceX;
				areaRightStart = areaLeftStart + 1.F;
				srcColumnEndInt = (int)std::ceilf(right);
				if (srcColumnEndInt < 0) srcColumnEndInt = 0;
				srcColumnEnd = (DWORD)srcColumnEndInt;
				if (srcColumnEnd > sourceWidth) srcColumnEnd = sourceWidth;
				sourceRowPtr = sourceRowPtrStart + srcColumnStart;
				
				for (DWORD srcRow = srcRowStart; srcRow < srcRowEnd; ++srcRow) {
					limitedTop = top > areaTop ? top : areaTop;
					limitedBottom = bottom < areaBottom ? bottom : areaBottom;
					areaHeight = limitedBottom - limitedTop;
					
					areaLeft = areaLeftStart;
					areaRight = areaRightStart;
					sourcePtr = sourceRowPtr;
					for (DWORD srcColumn = srcColumnStart; srcColumn < srcColumnEnd; ++srcColumn) {
						limitedLeft = left > areaLeft ? left : areaLeft;
						limitedRight = right < areaRight ? right : areaRight;
						areaWidth = limitedRight - limitedLeft;
						
						float area = areaHeight * areaWidth;
						redSum += (float)sourcePtr->red * area;
						greenSum += (float)sourcePtr->green * area;
						blueSum += (float)sourcePtr->blue * area;
						alphaSum += (float)sourcePtr->alpha * area;
						totalArea += area;
						
						areaLeft = areaRight;
						areaRight += 1.F;
						++sourcePtr;
					}
					
					areaTop = areaBottom;
					areaBottom += 1.F;
					incrementRow(sourceRowPtr, sourceStride);
				}
				
				if (totalArea == 0.F) {
					destinationPtr->red = 0;
					destinationPtr->green = 0;
					destinationPtr->blue = 0;
					destinationPtr->alpha = 0;
				} else {
					struct MyBlockScopeDefinedFunctions {
						static inline unsigned char cap(float val) {
							int valInt = (int)std::roundf(val);
							if (valInt < 0) valInt = 0;
							else if (valInt > 0xFF) valInt = 0xFF;
							return valInt;
						}
					};
					
					destinationPtr->red = MyBlockScopeDefinedFunctions::cap(redSum / totalArea);
					destinationPtr->green = MyBlockScopeDefinedFunctions::cap(greenSum / totalArea);
					destinationPtr->blue = MyBlockScopeDefinedFunctions::cap(blueSum / totalArea);
					destinationPtr->alpha = MyBlockScopeDefinedFunctions::cap(alphaSum / totalArea);
				}
				
				left = right;
				right += widthRatio;
				++destinationPtr;
			}
			top = bottom;
			bottom += heightRatio;
			incrementRow(destinationRowPtr, destinationStride);
		}
	} else {
		float sampleXStart = (float)sourceX;
		float sampleX;
		float sampleY = (float)sourceY;
		
		for (DWORD row = 0; row < destinationHeight; ++row) {
			destinationPtr = destinationRowPtr;
			sampleX = sampleXStart;
			for (DWORD column = 0; column < destinationWidth; ++column) {
				sample(source, sourceStride, sampleX, sampleY, sourceX, sourceWidth, sourceY, sourceHeight, destinationPtr);
				sampleX += widthRatio;
				++destinationPtr;
			}
			incrementRow(destinationRowPtr, destinationStride);
			sampleY += heightRatio;
		}
	}
}

void PngResource::drawRectOutline(void* destination, DWORD stride, DWORD x, DWORD y,
                                  DWORD width, DWORD height, DWORD color) {
	if (!width || !height) return;
	PixelA* ptr = getPixel(destination, stride, x, y);
	if (width == 1 && height == 1) {
		*(DWORD*)ptr = color;
		return;
	} else if (width == 1) {
		for (DWORD row = 0; row < height; ++row) {
			*(DWORD*)ptr = color;
			incrementRow(ptr, stride);
		}
		return;
	}
	for (DWORD column = 0; column < width; ++column) {
		*(DWORD*)ptr = color;
		++ptr;
	}
	if (height == 1) return;
	--ptr;
	incrementRow(ptr, stride);
	
	
	for (DWORD row = 1; row < height; ++row) {
		*(DWORD*)ptr = color;
		incrementRow(ptr, stride);
	}
	decrementRow(ptr, stride);
	--ptr;
	
	for (DWORD column = 1; column < width; ++column) {
		*(DWORD*)ptr = color;
		--ptr;
	}
	++ptr;
	decrementRow(ptr, stride);
	
	// need to take height - 1 out of the loop condition to prevent it from being reevaluated each iteration,
	// because in a test app where I did the same it recalculated height - 1 each time even on Release configuration
	const DWORD end = height - 1;
	for (DWORD row = 1; row < end; ++row) {
		*(DWORD*)ptr = color;
		decrementRow(ptr, stride);
	}
	
}

void PngResource::multiplyAlphaByPercent(void* destination, DWORD stride, DWORD x, DWORD y,
			DWORD width, DWORD height, int percent) {
	PixelA* rowPtr = getPixel(destination, stride, x, y);
	for (DWORD row = 0; row < height; ++row) {
		PixelA* ptr = rowPtr;
		for (DWORD column = 0; column < width; ++column) {
			int alpha = ptr->alpha;
			alpha *= percent / 100;
			if (alpha < 0) alpha = 0;
			if (alpha > 0xFF) alpha = 0xFF;
			ptr->alpha = alpha;
			++ptr;
		}
		incrementRow(rowPtr, stride);
	}
}

// sample with linear interpolation
void PngResource::sample(const void* source, DWORD stride, float x, float y, int xCapMin, int capWidth, int yCapMin, int capHeight, PixelA* outPixel) {
	int xLeft = (int)x;
	int xRight = xLeft + 1;
	int yTop = (int)y;
	int yBottom = yTop + 1;
	
	int xCapEnd = xCapMin + capWidth;
	int yCapEnd = yCapMin + capHeight;
	
	float xFraction = x - (float)xLeft;
	float yFraction = y - (float)yTop;
	
	if (yTop < yCapMin) {
		yTop = yCapMin;
		yFraction = 0.F;
	}
	if (xLeft < xCapMin) {
		xLeft = xCapMin;
		xFraction = 0.F;
	}
	const PixelA* topLeft = getPixel(source, stride, xLeft, yTop);
	
	if (xRight >= xCapEnd) {
		xRight = xCapEnd - 1;
		xFraction = 1.F;
	}
	const PixelA* topRight = topLeft - xLeft + xRight;
	
	float xFractionInv = 1.F - xFraction;
	
	#define lerp(start, end, fraction) (float)start * fraction##Inv + (float)end * fraction
	#define lerpRGBA \
		lerpPart(red) \
		lerpPart(green) \
		lerpPart(blue) \
		lerpPart(alpha)
		
	#define lerpPart(colorPart) float top_##colorPart = lerp(topLeft->colorPart, topRight->colorPart, xFraction);
	lerpRGBA
	#undef lerpPart
	
	if (yBottom >= yCapEnd) {
		yBottom = yCapEnd - 1;
		yFraction = 1.F;
	}
	const PixelA* bottomLeft = getPixel(source, stride, xLeft, yBottom);
	
	const PixelA* bottomRight = getPixel(source, stride, xRight, yBottom);
	
	#define lerpPart(colorPart) float bottom_##colorPart = lerp(bottomLeft->colorPart, bottomRight->colorPart, xFraction);
	lerpRGBA
	#undef lerpPart
	
	float yFractionInv = 1.F - yFraction;
	
	struct MyBlockScopeDefinedFunctions {
		static inline unsigned char cap(float val) {
			int valInt = (int)std::roundf(val);
			if (valInt < 0) valInt = 0;
			else if (valInt > 0xFF) valInt = 0xFF;
			return valInt;
		}
	};
	
	#define lerpPart(colorPart) outPixel->colorPart = MyBlockScopeDefinedFunctions::cap(lerp(top_##colorPart, bottom_##colorPart, yFraction));
	lerpRGBA
	#undef lerpPart
	#undef lerpRGBA
	#undef lerp
}
