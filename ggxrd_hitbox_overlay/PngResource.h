#pragma once
#include "pch.h"
#include <vector>

struct PngResource {
	size_t width = 0;
	size_t height = 0;
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
    void bitBlt(PngResource& destination, unsigned int destinationX, unsigned int destinationY, unsigned int sourceX, unsigned int sourceY, unsigned int sourceWidth, unsigned int sourceHeight) const;
    void bitBlt(void* destination, int destinationStride, unsigned int destinationX, unsigned int destinationY, unsigned int sourceX, unsigned int sourceY, unsigned int sourceWidth, unsigned int sourceHeight) const;
};

bool loadPngResource(HINSTANCE hInstance, WORD resourceSymbolId, PngResource& windowsImage);
