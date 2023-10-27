#pragma once
#include <d3d9.h>
#include <vector>
#include "Entity.h"

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device);
HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

class EndScene
{
public:
	bool onDllMain();
	void endSceneHook(IDirect3DDevice9* device);
	void setPresentFlag();
	bool consumePresentFlag();
	EndScene_t orig_EndScene;
	Present_t orig_Present;
private:
	// The EndScene function is actually being called twice: once by GuiltyGear and one more time by the Steam overlay.
	// However, Present is only called once each frame. So we use the Present function to determine if the next EndScene
	// call should draw the boxes.
	bool presentCalled = true;

	std::vector<Entity> drawnEntities;
	bool isEntityAlreadyDrawn(const Entity& ent) const;
};

extern EndScene endScene;
