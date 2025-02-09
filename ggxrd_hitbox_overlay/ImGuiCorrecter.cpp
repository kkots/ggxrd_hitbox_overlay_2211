#include "pch.h"
#include "ImGuiCorrecter.h"
#include "imgui.h"
#include "imgui_internal.h"  // don't want this file in UI.cpp, it already takes a millennia to compile
#include <cmath>

ImGuiCorrecter imGuiCorrecter;

void ImGuiCorrecter::interjectIntoImGui(float screenWidth, float screenHeight) {
	ImGuiIO& io = ImGui::GetIO();
	
	ImVec2 oldDisplaySize = io.DisplaySize;
	
	bool widthsDiffer = screenWidth != oldDisplaySize.x;
	bool heightsDiffer = screenHeight != oldDisplaySize.y;
	if (!widthsDiffer && !heightsDiffer) return;
	
	io.DisplaySize.x = screenWidth;
	io.DisplaySize.y = screenHeight;
	
	ImGuiContext* g = ImGui::GetCurrentContext();
	
	for (int i = 0; i < g->InputEventsQueue.Size; ++i) {
		ImGuiInputEvent* e = &g->InputEventsQueue[i];
		if (e->Type == ImGuiInputEventType_MousePos) {
			if (widthsDiffer) {
				e->MousePos.PosX = std::floorf(e->MousePos.PosX * screenWidth / oldDisplaySize.x);
			}
			if (heightsDiffer) {
				e->MousePos.PosY = std::floorf(e->MousePos.PosY * screenHeight / oldDisplaySize.y);
			}
		}
	}
}
