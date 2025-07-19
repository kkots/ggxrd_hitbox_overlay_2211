#pragma once
class ImGuiCorrecter
{
public:
	void adjustMousePosition(float screenWidth, float screenHeight,
			bool usePresentRect, int presentRectW, int presentRectH);
	bool checkWindowHasSize(const char* name, short* width, short* height, bool* windowExists);
private:
	int processedIds[100];
	int processedIdsCount = 0;
	int processedIdsIndex = 0;
	bool eventProcessed(int id) const;
	void addProcessedEvent(int id);
};

extern ImGuiCorrecter imGuiCorrecter;
