#pragma once
#include <vector>
#include "PngResource.h"
class TexturePacker
{
public:
	void addImage(PngResource& resource);
	PngResource getTexture();
private:
	struct Row {
		unsigned int height = 0;
		unsigned int remainingWidth = 0;
		std::vector<PngResource*> items;
	};
	PngResource getTexture(unsigned int size, std::vector<Row>* rows = nullptr);
	bool trySize(unsigned int size, std::vector<Row>& rows);
	std::vector<PngResource*> resources;
};

