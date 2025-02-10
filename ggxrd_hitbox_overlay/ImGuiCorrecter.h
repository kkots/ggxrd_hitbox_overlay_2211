#pragma once
class ImGuiCorrecter
{
public:
	void interjectIntoImGui(float screenWidth, float screenHeight,
			bool usePresentRect, int presentRectW, int presentRectH);
private:
	int processedIds[100];
	int processedIdsCount = 0;
	int processedIdsIndex = 0;
	bool eventProcessed(int id) const;
	void addProcessedEvent(int id);
};

extern ImGuiCorrecter imGuiCorrecter;
