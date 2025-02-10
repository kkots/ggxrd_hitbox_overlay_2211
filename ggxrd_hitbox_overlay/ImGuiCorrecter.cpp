#include "pch.h"
#include "ImGuiCorrecter.h"
#include "imgui.h"
#include "imgui_internal.h"  // don't want this file in UI.cpp, it already takes a millennia to compile
#include <cmath>

ImGuiCorrecter imGuiCorrecter;

void ImGuiCorrecter::interjectIntoImGui(float screenWidth, float screenHeight,
										bool usePresentRect, int presentRectW, int presentRectH) {
	ImGuiIO& io = ImGui::GetIO();
	ImGuiContext* g = ImGui::GetCurrentContext();
	
	ImVec2 oldDisplaySize = io.DisplaySize;
	bool widthsDiffer;
	bool heightsDiffer;
	
	if (usePresentRect) {
		io.DisplaySize.x = screenWidth;
		io.DisplaySize.y = screenHeight;
		
		float presentRectWF = (float)presentRectW;
		float presentRectHF = (float)presentRectH;
		
		widthsDiffer = screenWidth != presentRectWF;
		heightsDiffer = screenHeight != presentRectHF;
		if (!widthsDiffer && !heightsDiffer) return;
		
		for (int i = 0; i < g->InputEventsQueue.Size; ++i) {
			ImGuiInputEvent* e = &g->InputEventsQueue[i];
			if (e->Type == ImGuiInputEventType_MousePos && !eventProcessed(e->EventId)) {
				if (widthsDiffer) {
					e->MousePos.PosX = std::floorf(e->MousePos.PosX * screenWidth / presentRectWF);
				}
				if (heightsDiffer) {
					e->MousePos.PosY = std::floorf(e->MousePos.PosY * screenHeight / presentRectHF);
				}
				addProcessedEvent(e->EventId);
			}
		}
		
		return;
	}
	
	widthsDiffer = screenWidth != oldDisplaySize.x;
	heightsDiffer = screenHeight != oldDisplaySize.y;
	if (!widthsDiffer && !heightsDiffer) return;
	
	io.DisplaySize.x = screenWidth;
	io.DisplaySize.y = screenHeight;
	
	
	
	for (int i = 0; i < g->InputEventsQueue.Size; ++i) {
		ImGuiInputEvent* e = &g->InputEventsQueue[i];
		if (e->Type == ImGuiInputEventType_MousePos && !eventProcessed(e->EventId)) {
			if (widthsDiffer) {
				e->MousePos.PosX = std::floorf(e->MousePos.PosX * screenWidth / oldDisplaySize.x);
			}
			if (heightsDiffer) {
				e->MousePos.PosY = std::floorf(e->MousePos.PosY * screenHeight / oldDisplaySize.y);
			}
			addProcessedEvent(e->EventId);
		}
	}
}

bool ImGuiCorrecter::eventProcessed(int id) const {
	if (processedIdsCount == 0) return false;
	int index = processedIdsIndex;
	for (int i = 0; i < processedIdsCount; ++i) {
		if (index == 0) index = _countof(processedIds) - 1;
		else --index;
		
		if (processedIds[index] == id) return true;
	}
	return false;
}

void ImGuiCorrecter::addProcessedEvent(int id) {
	processedIds[processedIdsIndex] = id;
	if (processedIdsIndex == _countof(processedIds) - 1) processedIdsIndex = 0;
	else ++processedIdsIndex;
	
	if (processedIdsCount < _countof(processedIds)) {
		++processedIdsCount;
	}
}
