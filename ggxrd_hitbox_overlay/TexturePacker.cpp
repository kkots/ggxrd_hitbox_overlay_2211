#include "pch.h"
#include "TexturePacker.h"
#include <algorithm>

void TexturePacker::addImage(PngResource& resource) {
	resources.push_back(&resource);
}

PngResource TexturePacker::getTexture() {
	std::sort(resources.begin(), resources.end(), [](PngResource* a, PngResource* b) {
		return a->height > b->height;
	});
	unsigned int size = 1;
	std::vector<Row> rows;
	while (!trySize(size, rows)) {
		size *= 2;
		rows.clear();
	}
	return getTexture(size, &rows);
}

PngResource TexturePacker::getTexture(unsigned int size, std::vector<Row>* rows) {
	std::vector<Row> rowsInner;
	if (!rows) {
		if (!trySize(size, rowsInner)) return PngResource{};
		rows = &rowsInner;
	}
	PngResource result;
	result.data.resize(size * size);
	result.width = size;
	result.height = size;
	unsigned int currentY = 0;
	float sizeFloat = (float)size;
	for (auto rowIt = rows->begin(); rowIt != rows->end(); ++rowIt) {
		unsigned int currentX = 0;
		for (auto itemIt = rowIt->items.begin(); itemIt != rowIt->items.end(); ++itemIt) {
			
			PngResource& resource = **itemIt;
			resource.uStart = (float)currentX / sizeFloat;
			resource.vStart = (float)currentY / sizeFloat;
			resource.uEnd = ((float)currentX + (float)resource.width) / sizeFloat;
			resource.vEnd = ((float)currentY + (float)resource.height) / sizeFloat;
			resource.bitBlt(result, currentX, currentY, 0, 0, resource.width, resource.height);
			currentX += resource.width + 1;
		}
		currentY += rowIt->height + 1;
	}
	return result;
}

bool TexturePacker::trySize(unsigned int size, std::vector<Row>& rows) {
	unsigned int remainingHeight = size;
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		PngResource& resource = **it;
		if (resource.width > size || resource.height > size) return false;
		if (!rows.empty()) {
			Row& row = rows.back();
			unsigned int resourceWidthWithPadding = resource.width + 1;
			if (row.remainingWidth >= resourceWidthWithPadding) {
				row.items.push_back(&resource);
				row.remainingWidth -= resourceWidthWithPadding;
				if (resource.height > row.height) {
					const unsigned int diff = resource.height - row.height;
					if (remainingHeight >= diff) {
						remainingHeight -= diff;
						row.height = resource.height;
					}
					else {
						return false;
					}
				}
				continue;
			}
		}
		unsigned int resourceHeightWithPadding = resource.height;
		if (!rows.empty()) ++resourceHeightWithPadding;
		if (remainingHeight >= resourceHeightWithPadding) {
			remainingHeight -= resourceHeightWithPadding;
			rows.push_back(Row{});
			Row& row = rows.back();
			row.height = resource.height;
			row.remainingWidth = size - resource.width;
			row.items.push_back(&resource);
		}
		else {
			return false;
		}
	}
	return true;
}
