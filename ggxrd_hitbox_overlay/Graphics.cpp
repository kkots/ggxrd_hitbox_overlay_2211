#include "pch.h"
#include "Graphics.h"
#include "Detouring.h"
#include "memoryFunctions.h"
#include "Hitbox.h"
#include "Camera.h"
#include "BoundingRect.h"
#include "logging.h"
#include "pi.h"
#include "colors.h"
#include "PngRelated.h"
#include "Settings.h"
#include "UI.h"
#include "resource.h"
#include "WinError.h"
#include <d3dcompiler.h>
#include "resource.h"
#include <D3DX9Shader.h>
#ifdef PERFORMANCE_MEASUREMENT
#include <chrono>
#endif
#include <algorithm>
#include "Direct3DVTable.h"
#include "Game.h"
#include "InputsIcon.h"

Graphics graphics;

// THIS PERFORMANCE MEASUREMENT IS NOT THREAD SAFE
#ifdef PERFORMANCE_MEASUREMENT
static std::chrono::time_point<std::chrono::system_clock> performanceMeasurementStart;
#define PERFORMANCE_MEASUREMENT_DECLARE(name) \
	static unsigned long long performanceMeasurement_##name##_sum = 0; \
	static unsigned long long performanceMeasurement_##name##_count = 0; \
	static unsigned long long performanceMeasurement_##name##_average = 0;
#define PERFORMANCE_MEASUREMENT_ON_EXIT(name) \
	PerformanceMeasurementEnder performanceMeasurementEnder_##name(performanceMeasurement_##name##_sum, \
		performanceMeasurement_##name##_count, \
		performanceMeasurement_##name##_average, \
		#name);
#define PERFORMANCE_MEASUREMENT_START performanceMeasurementStart = std::chrono::system_clock::now();
#define PERFORMANCE_MEASUREMENT_END(name) \
	{ \
		PERFORMANCE_MEASUREMENT_ON_EXIT(name) \
	}
		
PERFORMANCE_MEASUREMENT_DECLARE(takeScreenshotBegin)
PERFORMANCE_MEASUREMENT_DECLARE(screenshotDrawAll1)
PERFORMANCE_MEASUREMENT_DECLARE(screenshotDrawAll2)
PERFORMANCE_MEASUREMENT_DECLARE(takeScreenshotEnd_getFramebufferData)
PERFORMANCE_MEASUREMENT_DECLARE(takeScreenshotEnd_pixelBlender)
PERFORMANCE_MEASUREMENT_DECLARE(takeScreenshotEnd_writeScreenshot)

struct PerformanceMeasurementEnder {
	PerformanceMeasurementEnder(unsigned long long& sum,
		unsigned long long& count,
		unsigned long long& average,
		const char* name) : sum(sum), count(count), average(average), name(name) { }
	~PerformanceMeasurementEnder() {
		unsigned long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - performanceMeasurementStart).count();
		sum += duration;
		++count;
		average = sum / count;
		logwrap(fprintf(logfile, "%s took %llu nanoseconds; average: %llu; count: %llu\n", name, duration, average, count));
	}
	unsigned long long& sum;
	unsigned long long& count;
	unsigned long long& average;
	const char* name;
};
#else
#define PERFORMANCE_MEASUREMENT_ON_EXIT(name) 
#define PERFORMANCE_MEASUREMENT_START 
#define PERFORMANCE_MEASUREMENT_END(name) 
#endif

static D3DXMATRIX identity;

bool Graphics::onDllMain(HMODULE hInstance) {
	bool error = false;
	uintptr_t UpdateD3DDeviceFromViewportsCallPlace = sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 79 40 00 74 05 e8 ?? ?? ?? ?? c2 04 00",
		{ 6 },
		&error, "UpdateD3DDeviceFromViewportsCallPlace");
	
	if (!UpdateD3DDeviceFromViewportsCallPlace) return false;
	orig_UpdateD3DDeviceFromViewports = (UpdateD3DDeviceFromViewports_t)followRelativeCall(UpdateD3DDeviceFromViewportsCallPlace);
	if (orig_UpdateD3DDeviceFromViewports) {
		void(HookHelp::*UpdateD3DDeviceFromViewportsHookPtr)() = &HookHelp::UpdateD3DDeviceFromViewportsHook;
		if (!detouring.attach(
			&(PVOID&)(orig_UpdateD3DDeviceFromViewports),
			(PVOID&)UpdateD3DDeviceFromViewportsHookPtr,
			"UpdateD3DDeviceFromViewports")) return false;
		
		uintptr_t FSuspendRenderingThreadCallPlace = sigscanForward((uintptr_t)orig_UpdateD3DDeviceFromViewports,
			"6a 01 8d 8c 24 80 00 00 00 e8");
		if (FSuspendRenderingThreadCallPlace) {
			orig_FSuspendRenderingThread = (FSuspendRenderingThread_t)followRelativeCall(FSuspendRenderingThreadCallPlace + 9);
		}
		uintptr_t functionEnd = sigscanForward((uintptr_t)orig_UpdateD3DDeviceFromViewports,
			"81 c4 ?? ?? ?? ?? c3", 0x1200);
		uintptr_t leaEcxEspPlusSmthPlace = 0;
		if (functionEnd) {
			leaEcxEspPlusSmthPlace = (uintptr_t)scrollUpToBytes((char*)functionEnd, "\x8d\x4c\x24", 3);
		}
		uintptr_t destructorCallPlace = 0;
		if (leaEcxEspPlusSmthPlace) {
			destructorCallPlace = sigscanForward(leaEcxEspPlusSmthPlace, "e8");
		}
		if (destructorCallPlace) {
			orig_FSuspendRenderingThreadDestructor = (FSuspendRenderingThreadDestructor_t)followRelativeCall(destructorCallPlace);
		}
	}
	if (orig_FSuspendRenderingThread) {
		void(HookHelp::*FSuspendRenderingThreadHookPtr)(unsigned int InSuspendThreadFlags) = &HookHelp::FSuspendRenderingThreadHook;
		if (!detouring.attach(
			&(PVOID&)(orig_FSuspendRenderingThread),
			(PVOID&)FSuspendRenderingThreadHookPtr,
			"FSuspendRenderingThread")) return false;
	} else return false;
	
	if (orig_FSuspendRenderingThreadDestructor) {
		void(HookHelp::*FSuspendRenderingThreadDestructorHookPtr)() = &HookHelp::FSuspendRenderingThreadDestructorHook;
		if (!detouring.attach(
			&(PVOID&)(orig_FSuspendRenderingThreadDestructor),
			(PVOID&)FSuspendRenderingThreadDestructorHookPtr,
			"~FSuspendRenderingThread")) return false;
	} else return false;
	
	shutdownFinishedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!shutdownFinishedEvent || shutdownFinishedEvent == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "Failed to create event: %ls\n", winErr.getMessage()));
		return false;
	}
	
	this->hInstance = hInstance;
	
	D3DXMatrixIdentity(&identity);
	
	renderStateValueHandlers[RenderStateType(D3DRS_STENCILENABLE)] = new RenderStateHandler(D3DRS_STENCILENABLE)(this);
	renderStateValueHandlers[RenderStateType(D3DRS_ALPHABLENDENABLE)] = new RenderStateHandler(D3DRS_ALPHABLENDENABLE)(this);
	renderStateValueHandlers[RenderStateType(PIXEL_SHADER)] = new RenderStateHandler(PIXEL_SHADER)(this);
	renderStateValueHandlers[RenderStateType(TRANSFORM_MATRICES)] = new RenderStateHandler(TRANSFORM_MATRICES)(this);
	renderStateValueHandlers[RenderStateType(D3DRS_SRCBLEND)] = new RenderStateHandler(D3DRS_SRCBLEND)(this);
	renderStateValueHandlers[RenderStateType(D3DRS_DESTBLEND)] = new RenderStateHandler(D3DRS_DESTBLEND)(this);
	renderStateValueHandlers[RenderStateType(D3DRS_SRCBLENDALPHA)] = new RenderStateHandler(D3DRS_SRCBLENDALPHA)(this);
	renderStateValueHandlers[RenderStateType(D3DRS_DESTBLENDALPHA)] = new RenderStateHandler(D3DRS_DESTBLENDALPHA)(this);
	renderStateValueHandlers[RenderStateType(VERTEX)] = new RenderStateHandler(VERTEX)(this);
	renderStateValueHandlers[RenderStateType(TEXTURE)] = new RenderStateHandler(TEXTURE)(this);
	
	RenderStateValueStack* stack = requiredRenderState[SCREENSHOT_STAGE_NONE];
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, FALSE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NONE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, NONE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(VERTEX)] = RenderStateValue(VERTEX, NONTEXTURE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, NONE);
	
	stack[RENDER_STATE_DRAWING_ARRAYBOXES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, TRUE);
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	
	stack[RENDER_STATE_DRAWING_BOXES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	
	stack[RENDER_STATE_DRAWING_OUTLINES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, FALSE);
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, CUSTOM_PIXEL_SHADER);
	// in OBS dodging mode, when pixel shader was putting 0 alpha in its output, outlines would be invisible, unless we stop using the pixel shader.
	// Changing alpha to 1 solved that problem. That problem only occured in OBS dodging, and everywhere else - on the screenshots or when not dodging - the outlines would be fine
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, FOR_PIXEL_SHADER);
	
	stack[RENDER_STATE_DRAWING_POINTS] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, FALSE);
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 2D);
	
	stack[RENDER_STATE_DRAWING_TEXTURES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_TEXTURES][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	stack[RENDER_STATE_DRAWING_TEXTURES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 2D);
	stack[RENDER_STATE_DRAWING_TEXTURES][RenderStateType(VERTEX)] = RenderStateValue(VERTEX, TEXTURE);
	stack[RENDER_STATE_DRAWING_TEXTURES][RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, ICONS);
	
	stack = requiredRenderState[SCREENSHOT_STAGE_BASE_COLOR];
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, TRUE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, NONE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	
	stack[RENDER_STATE_DRAWING_ARRAYBOXES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	
	stack[RENDER_STATE_DRAWING_BOXES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	
	stack = requiredRenderState[SCREENSHOT_STAGE_FINAL];
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, FALSE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	stack[RENDER_STATE_DRAWING_NOTHING][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, NONE);
	
	stack[RENDER_STATE_DRAWING_ARRAYBOXES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, TRUE);
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	// 1-(1-a)*(1-b) = a+b(1-a)
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_ARRAYBOXES][RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
	
	stack[RENDER_STATE_DRAWING_BOXES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_BOXES][RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
	
	stack[RENDER_STATE_DRAWING_OUTLINES] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 3D);
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_OUTLINES][RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	// we're using a CPU pixel blender and drawing onto a completely transparent black image so there's no need for a pixel shader
	
	stack[RENDER_STATE_DRAWING_POINTS] = stack[RENDER_STATE_DRAWING_NOTHING];
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, 2D);
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	stack[RENDER_STATE_DRAWING_POINTS][RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	
	// textures won't be drawn on screenshot
	
	orig_present = (Present_t)direct3DVTable.deviceVtable[17];
	orig_beginScene = (BeginScene_t)direct3DVTable.deviceVtable[41];
	responseToImInDanger = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!checkAndHookEndSceneAndPresent(true)) error = true;
	
	return !error;
}

bool Graphics::checkCanHookEndSceneAndPresent() {
	HMODULE obsDll = GetModuleHandleA("graphics-hook32.dll");
	if (!obsDll || obsDll == INVALID_HANDLE_VALUE) return false;
	uintptr_t start, end;
	if (!getModuleBoundsHandle(obsDll, &start, &end)) return false;
	const BYTE* ptr = (const BYTE*)orig_present;
	if (*ptr != 0xe9) return false;
	uintptr_t destination = followRelativeCallNoLogs((uintptr_t)ptr);
	if (!(destination >= start && destination < end)) return false;
	return true;
}

bool Graphics::checkAndHookEndSceneAndPresent(bool transactionActive) {
	if (!transactionActive && canDrawOnThisFrame()) return true;
	if (!checkCanHookEndSceneAndPresent()) return true;
	if (!transactionActive) {
		if (!detouring.beginTransaction(false)) return false;
	}
	bool error = false;
	
	if (!detouring.attach(
		&(PVOID&)(orig_present),
		presentHook,
		"Present")) error = true;
	
	if (!error && !detouring.attach(
		&(PVOID&)(orig_beginScene),
		endSceneHookStatic,
		"EndScene")) error = true;
	
	if (!transactionActive) {
		if (error) {
			detouring.cancelTransaction();
		} else {
			detouring.endTransaction();
			endSceneAndPresentHooked = true;
		}
	} else {
		endSceneAndPresentHooked = true;
	}
	return !error;
}

HRESULT __stdcall Graphics::presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	graphics.graphicsThreadId = GetCurrentThreadId();
	graphics.mayRunEndSceneHook = true;
	return graphics.orig_present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT __stdcall Graphics::endSceneHookStatic(IDirect3DDevice9* device) {
	graphics.endSceneHook(device);
	return graphics.orig_beginScene(device);
}

void Graphics::endSceneHook(IDirect3DDevice9* device) {
	graphicsThreadId = GetCurrentThreadId();
	if (mayRunEndSceneHook) {
		mayRunEndSceneHook = false;
		
		if (drawingPostponed() && !shutdown && !runningOwnBeginScene) {
			runningOwnBeginScene = true;
			device->BeginScene();
			runningOwnBeginScene = false;
			if (!pauseMenuOpen) {
				executeBoxesRenderingCommand(device);
			}
			if (uiTexture) {
				ui.pauseMenuOpen = pauseMenuOpen;
				ui.onEndScene(device, uiDrawData.data(), uiTexture);
			}
			device->EndScene();
		}
		
	}
}
// This function is called from the main thread.
// It 'initializes the D3D device for the current viewport state.'
void Graphics::HookHelp::UpdateD3DDeviceFromViewportsHook() {
	graphics.suspenderThreadId = GetCurrentThreadId();
	// This function will call the constructor of class FSuspendRenderingThread, which we hooked.
	// That constructor stops the rendering thread, so that this function could manipulate graphics
	// resources safely.
	// We must release our own resources only after the suspension occurs.
	// We must recreate our own resources either on the graphics thread or before the resuming occurs.
	graphics.orig_UpdateD3DDeviceFromViewports((char*)this);
	graphics.suspenderThreadId = NULL;
	return;
}

void Graphics::resetHook() {
	stencil.surface = NULL;
	stencil.direct3DSuccess = false;
	offscreenSurface = NULL;
	offscreenSurfaceWidth = 0;
	offscreenSurfaceHeight = 0;
	vertexBuffer = NULL;
	framesTexture = nullptr;
	outlinesRTSamplingTexture = nullptr;
	pixelShader = nullptr;
	altRenderTarget = nullptr;
}

void Graphics::dllDetachPiece() {
	resetHook();
	ui.onDllDetachGraphics();
	receiveDanger();
}

void Graphics::onDllDetach() {
	logwrap(fputs("Graphics::onDllDetach called\n", logfile));
	// this tells various callers to stop trying to use the resources as they're about to be freed
	shutdown = true;
	if (!graphicsThreadId) {
		dllDetachPiece();
		return;
	}
	HANDLE graphicsThreadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, graphicsThreadId);
	if (!graphicsThreadHandle) {
		WinError winErr;
		// The graphicsThreadId we have may be from a time before this thread got terminated:
		// When Xrd is closed with the mod still running, it kills all threads except the main one
		// first, then unloads all DLLs.
		// If the graphics thread got terminated, it will fail to open which is a possible reason
		// why we may be here.
		// The logic thread won't be terminated on normal Xrd exit though, because it is the main
		// thread and the window thread (as in, it pumps messages for windows).
		logwrap(fprintf(logfile, "Graphics failed to open graphics thread handle: %ls\n", winErr.getMessage()));
		dllDetachPiece();
		return;
	}
	if (GetProcessIdOfThread(graphicsThreadHandle) != GetCurrentProcessId()) {
		CloseHandle(graphicsThreadHandle);
		logwrap(fprintf(logfile, "Graphics freeing resources on DLL thread, because thread is no longer alive"));
		dllDetachPiece();
		return;
	}
	DWORD exitCode;
	bool stillActive = GetExitCodeThread(graphicsThreadHandle, &exitCode) && exitCode == STILL_ACTIVE;
	CloseHandle(graphicsThreadHandle);
	
	// free the resource - and stop using it
	
	if (!stillActive) {
		logwrap(fprintf(logfile, "Graphics freeing resources on DLL thread, because thread is no longer alive (2)"));
		dllDetachPiece();
		return;
	}
	
	logwrap(fputs("Graphics calling WaitForSingleObject\n", logfile));
	DWORD result = WaitForSingleObject(shutdownFinishedEvent, 300);
	if (result != WAIT_OBJECT_0) {
		logwrap(fprintf(logfile, "Graphics freeing resources on DLL thread, because WaitForSingleObject did not return success"));
		// We were hoping to free resources on the graphics thread, but if that's not possible, we free them on this thread
		dllDetachPiece();
		return;
	}
	logwrap(fprintf(logfile, "Graphics freed resources successfully\n"));
}

void Graphics::onEndSceneStart(IDirect3DDevice9* device) {
	if (shutdown) return;
	this->device = device;
	stencil.onEndSceneStart();
	graphics.receiveDanger();
	checkAndHookEndSceneAndPresent(false);
}

void Graphics::onShutdown() {
	resetHook();
	ui.onDllDetachGraphics();
	if (endSceneAndPresentHooked) {
		const char* hooksToUndetour[] {
			"EndScene",
			"Present"
		};
		detouring.detachOnlyTheseHooks(hooksToUndetour, _countof(hooksToUndetour));
	}
	SetEvent(shutdownFinishedEvent);
	receiveDanger();
}

bool Graphics::prepareBox(const DrawBoxCallParams& params, BoundingRect* const boundingRect, bool ignoreFill, bool ignoreOutline) {
	if (params.left == params.right
		|| params.top == params.bottom) return false;

	int left;
	int right;
	int top;
	int bottom;
	if (params.left > params.right) {
		left = params.right;
		right = params.left;
	} else {
		left = params.left;
		right = params.right;
	}
	if (params.top > params.bottom) {
		top = params.bottom;
		bottom = params.top;
	} else {
		top = params.top;
		bottom = params.bottom;
	}

	D3DXVECTOR3 v1{ (float)left, (float)top, 0.F };
	D3DXVECTOR3 v2{ (float)left, (float)bottom, 0.F };
	D3DXVECTOR3 v3{ (float)right, (float)top, 0.F };
	D3DXVECTOR3 v4{ (float)right, (float)bottom, 0.F };

	logOnce(fprintf(logfile, "Drawing box v1 { x: %f, z: %f }, v2 { x: %f, z: %f }, v3 { x: %f, z: %f }, v4 { x: %f, z: %f }\n",
			v1.x, v1.z, v2.x, v2.z, v3.x, v3.z, v4.x, v4.z));

	bool drewRect = false;

	if ((params.fillColor & 0xFF000000) != 0 && !ignoreFill) {
		drewRect = true;
		if (boundingRect) {
			D3DXVECTOR3 sp1;
			D3DXVECTOR3 sp2;
			D3DXVECTOR3 sp3;
			D3DXVECTOR3 sp4;
			bool projectionFailed = !camera.worldToScreen(device, v1, &sp1);
			projectionFailed = projectionFailed || !camera.worldToScreen(device, v2, &sp2);
			projectionFailed = projectionFailed || !camera.worldToScreen(device, v3, &sp3);
			projectionFailed = projectionFailed || !camera.worldToScreen(device, v4, &sp4);
			if (projectionFailed) {
				boundingRect->addX(0.F);
				boundingRect->addX(viewportW);
				boundingRect->addY(0.F);
				boundingRect->addY(viewportH);
			} else {
				boundingRect->addX(sp1.x);
				boundingRect->addX(sp2.x);
				boundingRect->addX(sp3.x);
				boundingRect->addX(sp4.x);
	
				boundingRect->addY(sp1.y);
				boundingRect->addY(sp2.y);
				boundingRect->addY(sp3.y);
				boundingRect->addY(sp4.y);
			}
		}

		logOnce(fprintf(logfile,
			"Box. Red: %u; Green: %u; Blue: %u; Alpha: %u;\n",
			(params.fillColor >> 16) & 0xff, (params.fillColor >> 8) & 0xff, params.fillColor & 0xff, (params.fillColor >> 24) & 0xff));

		D3DCOLOR fillColor;
		if (screenshotStage != SCREENSHOT_STAGE_NONE) {
			unsigned int newAlphaValue = ((unsigned int)(params.fillColor) >> 24) * 2;
			if (newAlphaValue > 255) newAlphaValue = 255;
			fillColor = replaceAlpha(newAlphaValue, params.fillColor);
		} else {
			fillColor = params.fillColor;
		}
		
		if (lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_BOX) {
			const bool drew = drawIfOutOfSpace(6);
			if (!drew) {
				*vertexIt = *(vertexIt - 1);
				++vertexIt;
				const Vertex firstVertex{ v1.x, v1.y, 0.F, fillColor };
				*vertexIt = firstVertex;
				++vertexIt;
				*vertexIt = firstVertex;
				++vertexIt;
				consumeVertexBufferSpace(6);
			} else {
				consumeVertexBufferSpace(4);
				*vertexIt = Vertex{ v1.x, v1.y, 0.F, fillColor };
				++vertexIt;
			}
		} else {
			drawIfOutOfSpace(4);
			consumeVertexBufferSpace(4);
			*vertexIt = Vertex{ v1.x, v1.y, 0.F, fillColor };
			++vertexIt;
		}
		*vertexIt = Vertex{ v2.x, v2.y, 0.F, fillColor };
		++vertexIt;
		*vertexIt = Vertex{ v3.x, v3.y, 0.F, fillColor };
		++vertexIt;
		*vertexIt = Vertex{ v4.x, v4.y, 0.F, fillColor };
		++vertexIt;
		++preparedBoxesCount;
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_BOX;
	}

	if (params.thickness && !ignoreOutline && screenshotStage != SCREENSHOT_STAGE_BASE_COLOR) {
		DrawOutlineCallParams drawOutlineCallParams;
		drawOutlineCallParams.reserveSize(4);

		drawOutlineCallParams.addPathElem(v1.x, v1.y, left, top, 1, 1);
		drawOutlineCallParams.addPathElem(v2.x, v2.y, left, bottom, 1, -1);
		drawOutlineCallParams.addPathElem(v4.x, v4.y, right, bottom, -1, -1);
		drawOutlineCallParams.addPathElem(v3.x, v3.y, right, top, -1, 1);
		drawOutlineCallParams.outlineColor = params.outlineColor;
		drawOutlineCallParams.thickness = params.thickness;
		drawOutlineCallParams.hatched = params.hatched;
		if (params.hatched) {
			drawOutlineCallParams.hatches.originX = params.originX;
			drawOutlineCallParams.hatches.originY = params.originY;
			drawOutlineCallParams.hatches.points.emplace_back();
			HatchesCallParams::HatchPoints& pointStarts = drawOutlineCallParams.hatches.points.back();
			pointStarts.start = drawOutlineCallParams.getStartPosition();
			pointStarts.count = 4;
		}
		outlines.push_back(drawOutlineCallParams);
	}
	return drewRect;
}

void Graphics::prepareTextureBox(const TextureBoxParams& box) {
	if (lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_TEXTUREBOX) {
		const bool drew = textureDrawIfOutOfSpace(6);
		if (!drew) {
			*textureVertexIt = *(textureVertexIt - 1);
			++textureVertexIt;
			const TextureVertex firstVertex{ box.xStart, box.yStart, 0.F, box.uStart, box.vStart, box.color };
			*textureVertexIt = firstVertex;
			++textureVertexIt;
			*textureVertexIt = firstVertex;
			++textureVertexIt;
			consumeTextureVertexBufferSpace(6);
		} else {
			consumeTextureVertexBufferSpace(4);
			*textureVertexIt = TextureVertex{ box.xStart, box.yStart, 0.F, box.uStart, box.vStart, box.color };
			++textureVertexIt;
		}
	} else {
		textureDrawIfOutOfSpace(4);
		consumeTextureVertexBufferSpace(4);
		*textureVertexIt = TextureVertex{ box.xStart, box.yStart, 0.F, box.uStart, box.vStart, box.color };
		++textureVertexIt;
	}
	*textureVertexIt = TextureVertex{ box.xEnd, box.yStart, 0.F, box.uEnd, box.vStart, box.color };
	++textureVertexIt;
	*textureVertexIt = TextureVertex{ box.xStart, box.yEnd, 0.F, box.uStart, box.vEnd, box.color };
	++textureVertexIt;
	*textureVertexIt = TextureVertex{ box.xEnd, box.yEnd, 0.F, box.uEnd, box.vEnd, box.color };
	++textureVertexIt;
	++preparedTextureBoxesCount;
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_TEXTUREBOX;
}

void Graphics::sendAllPreparedVertices() {
	if (vertexBufferSent) return;
	vertexBufferSent = true;
	Vertex* buffer = nullptr;
	if (FAILED(vertexBuffer->Lock(0, 0, (void**)&buffer, D3DLOCK_DISCARD))) return;
	size_t offset = sizeof Vertex * vertexBufferLength;
	if (preparingTextureVertexBuffer) {
		size_t textureOffset = sizeof TextureVertex * textureVertexBufferLength;
		if (textureOffset > offset) offset = textureOffset;
	}
	memcpy(buffer, vertexArena.data(), offset);
	if (FAILED(vertexBuffer->Unlock())) return;
}

bool Graphics::drawAllArrayboxes() {
	if (preparedArrayboxes.empty()) return true;
	advanceRenderState(RENDER_STATE_DRAWING_ARRAYBOXES);
	sendAllPreparedVertices();
	for (auto it = preparedArrayboxes.begin(); it != preparedArrayboxes.end(); ++it) {
		if (it->boxesPreparedSoFar) {

			if (needClearStencil && lastArrayboxId != it->id) {
				if (screenshotStage != SCREENSHOT_STAGE_BASE_COLOR) {
					stencil.clearRegion(device, lastBoundingRect);
				}
				needClearStencil = false;
			}

			stencil.initialize(device);
			device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, 2 + (it->boxesPreparedSoFar - 1) * 6);
			vertexBufferPosition += 4 + (it->boxesPreparedSoFar - 1) * 6;
			if (vertexBufferPosition > vertexBufferLength) {
				logwrap(fprintf(logfile, "drawAllArrayboxes made vertex buffer position go out of bounds: %u, %u. Bounding rect: %f, %f, %f, %f\n",
					vertexBufferPosition, vertexBufferLength, (double)it->boundingRect.left, (double)it->boundingRect.top, (double)it->boundingRect.bottom,
					(double)it->boundingRect.right));
			}
			preparedBoxesCount -= it->boxesPreparedSoFar;
			it->boxesPreparedSoFar = 0;
			
			needClearStencil = true;
			lastArrayboxId = it->id;
			lastBoundingRect = it->boundingRect;

			if (!it->isComplete) {
				preparedArrayboxes.erase(preparedArrayboxes.begin(), it);
				return false;
			}

		} else if (!it->isComplete) {
			preparedArrayboxes.erase(preparedArrayboxes.begin(), it);
			return false;
		}
	}
	preparedArrayboxes.clear();
	return true;
}

void Graphics::drawAllBoxes() {
	if (!preparedBoxesCount) return;
	if (screenshotStage == SCREENSHOT_STAGE_BASE_COLOR) {
		stencil.initialize(device);
	}
	sendAllPreparedVertices();
	advanceRenderState(RENDER_STATE_DRAWING_BOXES);
	device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, 2 + (preparedBoxesCount - 1) * 6);
	vertexBufferPosition += 4 + (preparedBoxesCount - 1) * 6;
	preparedBoxesCount = 0;
}

void Graphics::drawAllTextureBoxes() {
	if (!preparedTextureBoxesCount) return;
	sendAllPreparedVertices();
	switchToRenderingTextureVertices();
	advanceRenderState(RENDER_STATE_DRAWING_TEXTURES);
	device->DrawPrimitive(D3DPT_TRIANGLESTRIP, textureVertexBufferPosition, 2 + (preparedTextureBoxesCount - 1) * 6);
	textureVertexBufferPosition += 4 + (preparedTextureBoxesCount - 1) * 6;
	preparedTextureBoxesCount = 0;
}

void Graphics::drawOutlinesSection(bool preserveLastTwoVertices) {
	if (outlinesSectionOutlineCount) {
		if (!loggedDrawingOperationsOnce) {
			logwrap(fprintf(logfile, "drawOutlinesSection: preserveLastTwoVertices: %u; vertexBufferPosition: %u, outlinesSectionTotalLineCount: %u, outlinesSectionOutlineCount: %u\n",
				(unsigned int)preserveLastTwoVertices, vertexBufferPosition, outlinesSectionTotalLineCount, outlinesSectionOutlineCount));
		}
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, 2 * outlinesSectionTotalLineCount + 4 * (outlinesSectionOutlineCount - 1));
		vertexBufferPosition += (preserveLastTwoVertices ? 0 : 2) + 2 * outlinesSectionTotalLineCount + 4 * (outlinesSectionOutlineCount - 1);
		outlinesSectionOutlineCount = 0;
		outlinesSectionTotalLineCount = 0;
	}
	if (outlinesSectionHatchCount) {
		device->DrawPrimitive(D3DPT_LINELIST, vertexBufferPosition, outlinesSectionHatchCount);
		vertexBufferPosition += 2 * outlinesSectionHatchCount;
		outlinesSectionHatchCount = 0;
	}
}

bool Graphics::drawAllOutlines() {
	if (preparedOutlines.empty()) return true;
	sendAllPreparedVertices();
	advanceRenderState(RENDER_STATE_DRAWING_OUTLINES);
	
	if (!loggedDrawingOperationsOnce) {
		logwrap(fprintf(logfile, "drawAllOutlines: vertexBufferPosition: %u\n", vertexBufferPosition));
	}
	
	for (auto it = preparedOutlines.begin(); it != preparedOutlines.end(); ++it) {
		if (!loggedDrawingOperationsOnce) {
			logwrap(fprintf(logfile, "drawAllOutlines: outline index: %u; it->isOnePixelThick: %u; it->linesSoFar: %u; it->isComplete: %u\n",
				it - preparedOutlines.begin(), (unsigned int)it->isOnePixelThick, it->linesSoFar, (unsigned int)it->isComplete));
		}
		if (it->isOnePixelThick) {
			if (it->linesSoFar) {
				drawOutlinesSection(false);
				device->DrawPrimitive(D3DPT_LINESTRIP, vertexBufferPosition, it->linesSoFar);
				if (it->isComplete) {
					vertexBufferPosition += 1 + it->linesSoFar;
					it->linesSoFar = 0;
					if (it->hatched) {
						outlinesSectionHatchCount += it->hatchesCount;
						it->hatchesCount = 0;
						drawOutlinesSection(false);
						if (!it->hatchesComplete) {
							preparedOutlines.erase(preparedOutlines.begin(), it);
							return false;
						}
					}
				} else {
					// we'll duplicate the last one vertex into the new buffer
					vertexBufferPosition += it->linesSoFar;
					it->linesSoFar = 0;
					if (!loggedDrawingOperationsOnce) {
						logwrap(fprintf(logfile, "drawAllOutlines: erasing %u lines\n", it - preparedOutlines.begin()));
					}
					preparedOutlines.erase(preparedOutlines.begin(), it);
					return false;
				}
			} else if (!it->isComplete) {
				drawOutlinesSection(false);
				if (!loggedDrawingOperationsOnce) {
					logwrap(fprintf(logfile, "drawAllOutlines: erasing %u lines\n", it - preparedOutlines.begin()));
				}
				preparedOutlines.erase(preparedOutlines.begin(), it);
				return false;
			} else if (it->hatched) {
				outlinesSectionHatchCount += it->hatchesCount;
				it->hatchesCount = 0;
				drawOutlinesSection(false);
				if (!it->hatchesComplete) {
					preparedOutlines.erase(preparedOutlines.begin(), it);
					return false;
				}
			}
		} else {
			if (it->linesSoFar) {
				if (outlinesSectionHatchCount) {
					drawOutlinesSection(false);
				}
				if (outlinesSectionOutlineCount == 0 && !loggedDrawingOperationsOnce) {
					logwrap(fprintf(logfile, "drawAllOutlines: starting new outlines section\n"));
				}
				++outlinesSectionOutlineCount;
				outlinesSectionTotalLineCount += it->linesSoFar;
				
				it->linesSoFar = 0;
				if (!it->isComplete) {
					drawOutlinesSection(true);
					if (!loggedDrawingOperationsOnce) {
						logwrap(fprintf(logfile, "drawAllOutlines: erasing %u lines\n", it - preparedOutlines.begin()));
					}
					preparedOutlines.erase(preparedOutlines.begin(), it);
					return false;
				} else if (it->hatched) {
					outlinesSectionHatchCount += it->hatchesCount;
					it->hatchesCount = 0;
					drawOutlinesSection(false);
					if (!it->hatchesComplete) {
						preparedOutlines.erase(preparedOutlines.begin(), it);
						return false;
					}
				}
			} else if (!it->isComplete) {
				drawOutlinesSection(false);
				if (it->hasPadding) {
					it->hasPadding = false;
					vertexBufferPosition += 2;
				}
				if (!loggedDrawingOperationsOnce) {
					logwrap(fprintf(logfile, "drawAllOutlines: erasing %u lines\n", it - preparedOutlines.begin()));
				}
				preparedOutlines.erase(preparedOutlines.begin(), it);
				return false;
			} else if (it->hatched) {
				outlinesSectionHatchCount += it->hatchesCount;
				it->hatchesCount = 0;
				drawOutlinesSection(false);
				if (!it->hatchesComplete) {
					preparedOutlines.erase(preparedOutlines.begin(), it);
					return false;
				}
			}
		}
	}
	drawOutlinesSection(false);

	preparedOutlines.clear();
	return true;
}

void Graphics::drawAllPoints() {
	if (!numberOfPointsPrepared) return;
	sendAllPreparedVertices();
	advanceRenderState(RENDER_STATE_DRAWING_POINTS);

	for (unsigned int i = numberOfPointsPrepared; i != 0; --i) {
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, 8);
		vertexBufferPosition += 10;
		device->DrawPrimitive(D3DPT_LINELIST, vertexBufferPosition, 2);
		vertexBufferPosition += 4;
	}

	numberOfPointsPrepared = 0;

}

void Graphics::drawAllPrepared() {
	if (!loggedDrawingOperationsOnce) {
		logwrap(fprintf(logfile, "Arrayboxes count: %u\n"
			"boxes count (including those in arrayboxes): %u\n"
			"outlines count: %u\n"
			"points count: %u\n"
			"texture boxes count: %u",
			preparedArrayboxes.size(),
			preparedBoxesCount,
			preparedOutlines.size(),
			numberOfPointsPrepared,
			preparedTextureBoxesCount));
	}
	switch (1) {
	case 1:
		if (!drawAllArrayboxes()) break;
		drawAllBoxes();
		if (!drawAllOutlines()) break;
		drawAllPoints();
		drawAllTextureBoxes();
	}
	
	bool willMessWithTextureVertexBufferSeparately = preparingTextureVertexBuffer && textureVertexBufferPosition != 0;
	
	if (vertexBufferPosition != 0) {
		if (vertexBufferPosition > vertexBufferLength) {
			logwrap(fprintf(logfile, "vertexBufferPosition > vertexBufferLength: %u, %u\n", vertexBufferPosition, vertexBufferLength));
			// this is an error
		}
		if (vertexBufferPosition != vertexBufferLength) {
			memmove(vertexArena.data(), vertexArena.data() + vertexBufferPosition, (vertexBufferLength - vertexBufferPosition) * sizeof Vertex);
		}
		
		int startingTextureVertexBufferLength;
		int newStartingTextureVertexBufferLength;
		int freedTextureVertices;
		if (!willMessWithTextureVertexBufferSeparately) {
			startingTextureVertexBufferLength = calculateStartingTextureVertexBufferLength();
		}
		
		vertexBufferLength -= vertexBufferPosition;
		
		if (!willMessWithTextureVertexBufferSeparately) {
			newStartingTextureVertexBufferLength = calculateStartingTextureVertexBufferLength();
			freedTextureVertices = startingTextureVertexBufferLength - newStartingTextureVertexBufferLength;
			if (freedTextureVertices) {
				textureVertexBufferRemainingSize += freedTextureVertices;
				textureVertexBufferLength -= freedTextureVertices;
				textureVertexIt = (TextureVertex*)vertexArena.data() + textureVertexBufferLength;
				if (renderingTextureVertices) {
					textureVertexBufferPosition -= freedTextureVertices;
				}
			}
		}
		vertexBufferPosition = 0;
		vertexIt = vertexArena.data() + vertexBufferLength;
		vertexBufferRemainingSize = vertexBufferSize - vertexBufferLength;
		if (!loggedDrawingOperationsOnce) {
			logwrap(fprintf(logfile, "vertexBufferNewline: resetting vertex buffer: vertexBufferLength: %u,"
				" vertexBufferPosition: 0, vertex iterator: %u, vertex buffer remaining size: %u\n",
				vertexBufferLength, it - vertexArena.begin(), vertexBufferRemainingSize));
		}
	}
	if (willMessWithTextureVertexBufferSeparately) {
		if (textureVertexBufferPosition > textureVertexBufferLength) {
			logwrap(fprintf(logfile, "textureVertexBufferPosition > textureVertexBufferLength: %u, %u\n", textureVertexBufferPosition, textureVertexBufferLength));
			// this is an error
		}
		// texture vertex buffer can't get stuck in the middle of drawing a primitive, it's always complete boxes
		// if you started drawing texture vertices, there can be no leftovers from non-texture vertices
		textureVertexBufferLength = 0;
		textureVertexBufferPosition = 0;
		textureVertexIt = (TextureVertex*)vertexArena.data();
		textureVertexBufferRemainingSize = textureVertexBufferSize;
	}
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	vertexBufferSent = false;
}

void Graphics::drawAll() {
	
	if (!canDrawOnThisFrame()) return;
	
	initializeVertexBuffers();
	resetVertexBuffer();
	preparedArrayboxIdCounter = 0;
	needClearStencil = false;

	drawingWhat = RENDER_STATE_DRAWING_NOTHING;
	CComPtr<IDirect3DStateBlock9> oldState = nullptr;
	if (screenshotStage == SCREENSHOT_STAGE_NONE) {
		currentTransformSet = CURRENT_TRANSFORM_DEFAULT;
		renderStateValues[RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, NONE);
		device->CreateStateBlock(D3DSBT_ALL, &oldState);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		renderStateValues[RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		renderStateValues[RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		renderStateValues[RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_LIGHTING, FALSE);
		
		renderStateValues[RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, FALSE);
		renderStateValues[RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NONE);
		renderStateValues[RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		renderStateValues[RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
		
		device->SetTexture(0, nullptr);
		renderStateValues[RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, NONE);
		device->SetVertexShader(nullptr);
		device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
		renderStateValues[RenderStateType(VERTEX)] = RenderStateValue(VERTEX, NONTEXTURE);
		device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
		
	}
	
	if (!onlyDrawPoints && !dontShowBoxes) {
		for (const ComplicatedHurtbox& params : drawDataUse.hurtboxes) {
			prepareComplicatedHurtbox(params);
		}
		
		static std::vector<bool> algoArena;  // needed for Johnny hitboxes, they have a duplicate:
		                                     // one is a projectile with clashOnly set, the other is Johnny himself
		                                     // clashOnly we recently started displaying more thin, they're after Johnny's own hitboxes,
		                                     // so that makes Johnny's own hitboxes end up displaying thin, which is wrong. We're fixing that
		algoArena.assign(drawDataUse.hitboxes.size(), false);
		
		for (int i = 0; i < (int)drawDataUse.hitboxes.size(); ++i) {
			DrawHitboxArrayCallParams& params = drawDataUse.hitboxes[i];
			bool found = algoArena[i];
			if (!found) {
				for (int j = i + 1; j < (int)drawDataUse.hitboxes.size(); ++j) {
					DrawHitboxArrayCallParams& paramsOther = drawDataUse.hitboxes[j];
					if (params == paramsOther) {
						if (paramsOther.thickness >= params.thickness) {
							found = true;
						} else {
							algoArena[j] = true;
						}
						break;
					}
				}
			}
			if (!found) {
				prepareArraybox(params, false);
			}
		}
		for (const DrawBoxCallParams& params : drawDataUse.pushboxes) {
			prepareBox(params);
		}
		for (const DrawBoxCallParams& params : drawDataUse.throwBoxes) {
			prepareBox(params);
		}
		for (const DrawBoxCallParams& params : drawDataUse.interactionBoxes) {
			prepareBox(params);
		}
		if (screenshotStage != SCREENSHOT_STAGE_BASE_COLOR) {
			for (DrawOutlineCallParams& params : outlines) {
				prepareOutline(params);
			}
		}
	}
	
	if (screenshotStage != SCREENSHOT_STAGE_BASE_COLOR
			&& (!noNeedToDrawPoints || drawDataUse.needTakeScreenshot)
			&& !dontShowBoxes) {
		for (const DrawPointCallParams& params : drawDataUse.points) {
			preparePoint(params);
		}
	}
	if ((onlyDrawPoints || drawingPostponed())
			&& screenshotStage == SCREENSHOT_STAGE_NONE
			&& (drawDataUse.inputsSize[0] || drawDataUse.inputsSize[1])) {
		prepareDrawInputs();
	}
	drawAllPrepared();
	
	outlines.clear();

	if (screenshotStage == SCREENSHOT_STAGE_NONE) {
		stencil.onEndSceneEnd(device);
		oldState->Apply();
		bringBackOldTransform(device);
		
	}

	loggedDrawingOperationsOnce = true;

}

void Graphics::bringBackOldTransform(IDirect3DDevice9* device) {
	if (currentTransformSet != CURRENT_TRANSFORM_DEFAULT) {
	    device->SetTransform(D3DTS_WORLD, &prevWorld);
	    device->SetTransform(D3DTS_VIEW, &prevView);
	    device->SetTransform(D3DTS_PROJECTION, &prevProjection);
	    currentTransformSet = CURRENT_TRANSFORM_DEFAULT;
	}
}

void Graphics::prepareArraybox(const DrawHitboxArrayCallParams& params, bool isComplicatedHurtbox,
								BoundingRect* boundingRect, std::vector<DrawOutlineCallParams>* outlinesOverride) {
	if (!params.hitboxCount) return;
	logOnce(fputs("drawHitboxArray called with parameters:\n", logfile));
	logOnce(fprintf(logfile, "hitboxData: %p\n", params.hitboxData));
	logOnce(fprintf(logfile, "hitboxCount: %d\n", params.hitboxCount));
	logOnce(fputs("fillColor: ", logfile));
	logColor(params.fillColor);
	logOnce(fputs("\noutlineColor: ", logfile));
	logColor(params.outlineColor);
	logOnce(fprintf(logfile, "\nposX: %d\nposY: %d\nflip: %hhd\nscaleX: %d\nscaleY: %d\nangle: %d\nhitboxOffsetX: %d\nhitboxOffsetY: %d\n",
		params.params.posX, params.params.posY, params.params.flip, params.params.scaleX, params.params.scaleY,
		params.params.angle, params.params.hitboxOffsetX, params.params.hitboxOffsetY));
	bool drawOutlines = screenshotStage != SCREENSHOT_STAGE_BASE_COLOR;
	if (drawOutlines) {
		rectCombinerInputBoxes.reserve(params.hitboxCount);
	}
	BoundingRect localBoundingRect;
	if (!boundingRect) {
		boundingRect = &localBoundingRect;
	}
	if (!isComplicatedHurtbox) {
		preparedArrayboxes.emplace_back();
		preparedArrayboxes.back().id = preparedArrayboxIdCounter++;
	}

	int cos = -2000;
	int sin = -2000;
	if (params.params.angle) {
		float angleRads = -(float)params.params.angle / 1000.F / 180.F * PI;
		cos = (int)(::cos(angleRads) * 1000.F);
		sin = (int)(::sin(angleRads) * 1000.F);
	}
	
	DrawBoxCallParams drawBoxCall;
	drawBoxCall.fillColor = params.fillColor;

	for (int i = 0; i < params.hitboxCount; ++i) {
		logOnce(fprintf(logfile, "drawing box %d\n", params.hitboxCount - i));
		
		RECT bounds = params.getWorldBounds(i, cos, sin);
		
		if (drawOutlines) {
			rectCombinerInputBoxes.emplace_back(bounds.left, bounds.right, bounds.top, bounds.bottom);
		}

		drawBoxCall.left = bounds.left;
		drawBoxCall.right = bounds.right;
		drawBoxCall.top = bounds.top;
		drawBoxCall.bottom = bounds.bottom;

		if (prepareBox(drawBoxCall, boundingRect, false, true)) {
			++preparedArrayboxes.back().boxesPreparedSoFar;
		}
	}
	if (!isComplicatedHurtbox) {
		PreparedArraybox& preparedArraybox = preparedArrayboxes.back();
		preparedArraybox.isComplete = true;
		preparedArraybox.boundingRect = *boundingRect;
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_ARRAYBOX;
	}
	
	if (drawOutlines) {
		RectCombiner::getOutlines(rectCombinerInputBoxes, rectCombinerOutlines);
		rectCombinerInputBoxes.clear();
		size_t totalCount = rectCombinerOutlines.size();
		size_t counter = 0;
		std::vector<HatchesCallParams::HatchPoints>* pointStarts = nullptr;
		unsigned long long pointStartsMem[sizeof *pointStarts / 8 + ((sizeof *pointStarts % 8) != 0 ? 1 : 0)];
		if (params.hatched) {
			pointStarts = new (pointStartsMem) std::vector<HatchesCallParams::HatchPoints>();
			pointStarts->reserve(totalCount);
		}
		std::vector<DrawOutlineCallParams>* outlinesDest = outlinesOverride ? outlinesOverride : &outlines;
		outlinesDest->reserve(outlinesDest->size() + totalCount);
		for (const std::vector<RectCombiner::PathElement>& outline : rectCombinerOutlines) {
			outlinesDest->emplace_back();
			DrawOutlineCallParams& drawOutlineCallParams = outlinesDest->back();
			drawOutlineCallParams.outlineColor = params.outlineColor;
			drawOutlineCallParams.thickness = params.thickness;
			++counter;
			bool hatched = params.hatched && counter == totalCount;
			drawOutlineCallParams.hatched = hatched;
			drawOutlineCallParams.reserveSize(outline.size());
			for (const RectCombiner::PathElement& path : outline) {
				drawOutlineCallParams.addPathElem(path.x, path.y, path.xDir(), path.yDir());
			}
			if (params.hatched) {
				pointStarts->emplace_back();
				HatchesCallParams::HatchPoints& pointStart = pointStarts->back();
				pointStart.start = drawOutlineCallParams.getStartPosition();
				pointStart.count = outline.size();
				if (hatched) {
					drawOutlineCallParams.hatches.originX = params.originX;
					drawOutlineCallParams.hatches.originY = params.originY;
					drawOutlineCallParams.hatches.points = std::move(*pointStarts);
				}
			}
		}
	}
}

Graphics::Vertex::Vertex(float x, float y, float z, D3DCOLOR color)
	: x(x), y(y), z(z), color(color) { }

Graphics::TextureVertex::TextureVertex(float x, float y, float z, float u, float v, D3DCOLOR color)
	: x(x), y(y), z(z), u(u), v(v), color(color) { }

void Graphics::prepareOutline(DrawOutlineCallParams& params) {
	if (params.empty()) return;
	logOnce(fprintf(logfile, "Called drawOutlines with an outline with %d elements\n", params.count()));
	
	PreparedOutline* preparedOutlinePtr = nullptr;
	
	if (params.thickness == 1) {
		
		preparedOutlines.emplace_back();
		preparedOutlines.back().isOnePixelThick = true;

		Vertex firstVertex;

		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			drawIfOutOfSpace(1);
			if (outlineIndex != 0) {
				++preparedOutlines.back().linesSoFar;
			}

			logOnce(fprintf(logfile, "x: %d; y: %d;\n", elem.x, elem.y));
			*vertexIt = Vertex{ (float)elem.x, (float)elem.y, 0.F, params.outlineColor };
			if (outlineIndex == 0) firstVertex = *vertexIt;
			++vertexIt;
			consumeVertexBufferSpace(1);
		}
		drawIfOutOfSpace(1);
		PreparedOutline& preparedOutline = preparedOutlines.back();
		preparedOutlinePtr = &preparedOutline;
		++preparedOutline.linesSoFar;
		preparedOutline.isComplete = true;
		*vertexIt = firstVertex;
		++vertexIt;
		consumeVertexBufferSpace(1);
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_THINLINE;

	} else {
		
		std::vector<D3DXVECTOR3> extraPoints(params.count(), D3DXVECTOR3{ 0.F, 0.F, 0.F });
		
		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			PathElement& elem = params.getPathElem(outlineIndex);
			extraPoints[outlineIndex] = D3DXVECTOR3{
				(float)elem.x + params.thickness * elem.inX,
				(float)elem.y + params.thickness * elem.inY,
				0.F
			};
		}

		preparedOutlines.emplace_back();

		Vertex firstVertex;
		Vertex secondVertex;

		bool padTheFirst = (lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_THICKLINE);
		
		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			logOnce(fprintf(logfile, "x: %d; y: %d;\n", elem.x, elem.y));
			if (padTheFirst && !drawIfOutOfSpace(4)) {
				firstVertex = Vertex{ (float)elem.x, (float)elem.y, 0.F, params.outlineColor };
				*vertexIt = *(vertexIt - 1);
				++vertexIt;
				*vertexIt = firstVertex;
				++vertexIt;
				*vertexIt = firstVertex;
				++vertexIt;
				consumeVertexBufferSpace(4);
				preparedOutlines.back().hasPadding = true;
			} else {
				drawIfOutOfSpace(2);
				if (outlineIndex == 0) {
					firstVertex = Vertex{ (float)elem.x, (float)elem.y, 0.F, params.outlineColor };
					*vertexIt = firstVertex;
					++vertexIt;
				} else {
					++preparedOutlines.back().linesSoFar;
					*vertexIt = Vertex{ (float)elem.x, (float)elem.y, 0.F, params.outlineColor };
					++vertexIt;
				}
				consumeVertexBufferSpace(2);
			}
			padTheFirst = false;
			
			const D3DXVECTOR3& extraPoint = extraPoints[outlineIndex];

			if (outlineIndex == 0) {
				secondVertex = Vertex{ extraPoint.x, extraPoint.y, 0.F, params.outlineColor };
				*vertexIt = secondVertex;
				++vertexIt;
			} else {
				*vertexIt = Vertex{ extraPoint.x, extraPoint.y, 0.F, params.outlineColor };
				++vertexIt;
			}
		}
		drawIfOutOfSpace(2);
		PreparedOutline& preparedOutline = preparedOutlines.back();
		preparedOutlinePtr = &preparedOutline;
		++preparedOutline.linesSoFar;
		preparedOutline.isComplete = true;
		*vertexIt = firstVertex;
		++vertexIt;
		*vertexIt = secondVertex;
		++vertexIt;
		consumeVertexBufferSpace(2);
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_THICKLINE;
		
	}
		
	if (params.hatched) {
		preparedOutlinePtr->hatched = true;
		
		struct HatchPoint {
			int n;
			int x;
			int y;
		};
		static std::vector<HatchPoint> hatchArena;
		hatchArena.clear();
		
		int originX = params.hatches.originX + hatchesDist / 2;
		int originY = params.hatches.originY;
		
		for (const HatchesCallParams::HatchPoints& pointStart : params.hatches.points) {
			if (!pointStart.count) break;
			
			const PathElement* pathElementStart = &DrawOutlineCallParams::getPathElemStatic(pointStart.start, 0);
			int distToLine = pathElementStart->x - originX + (pathElementStart->y - originY);
			int n = distToLine / hatchesDist;
			int dist = distToLine % hatchesDist;
			if (distToLine > 0) {
				dist = hatchesDist - dist;
			} else if (distToLine < 0) {
				--n;
				dist = hatchesDist + dist;
			}
			
			const PathElement* pathElementEnd = pathElementStart;
			
			for (int i = 0; i < pointStart.count; ++i) {
				pathElementStart = pathElementEnd;
				if (i == pointStart.count - 1) {
					pathElementEnd = &DrawOutlineCallParams::getPathElemStatic(pointStart.start, 0);
				} else {
					pathElementEnd = &DrawOutlineCallParams::getPathElemStatic(pointStart.start, i + 1);
				}
				
				DWORD lineHasX;
				DWORD lineHasY;
				int distEnd;
				if (pathElementEnd->x == pathElementStart->x) {
					lineHasX = 0;
					lineHasY = -1;
					if (pathElementEnd->y == pathElementStart->y) {
						continue;
					}
					distEnd = dist + pathElementEnd->y - pathElementStart->y;
				} else {
					lineHasX = -1;
					lineHasY = 0;
					distEnd = dist + pathElementEnd->x - pathElementStart->x;
				}
				
				int off = -dist;
				if (distEnd > 0) {
					if (dist < 0) {
						hatchArena.emplace_back();
						HatchPoint& hatchPoint = hatchArena.back();
						hatchPoint.n = n;
						hatchPoint.x = pathElementStart->x + (lineHasX & off);
						hatchPoint.y = pathElementStart->y + (lineHasY & off);
					}
					while (distEnd >= hatchesDist) {
						++n;
						distEnd -= hatchesDist;
						off += hatchesDist;
						if (distEnd != 0 || pathElementEnd->inX != pathElementEnd->inY) {
							hatchArena.emplace_back();
							HatchPoint& hatchPoint = hatchArena.back();
							hatchPoint.n = n;
							hatchPoint.x = pathElementStart->x + (lineHasX & off);
							hatchPoint.y = pathElementStart->y + (lineHasY & off);
						}
					}
				} else {
					if (dist > 0) {
						if (distEnd != 0 || pathElementEnd->inX != pathElementEnd->inY) {
							hatchArena.emplace_back();
							HatchPoint& hatchPoint = hatchArena.back();
							hatchPoint.n = n;
							hatchPoint.x = pathElementStart->x + (lineHasX & off);
							hatchPoint.y = pathElementStart->y + (lineHasY & off);
						}
					}
					while (distEnd <= -hatchesDist) {
						--n;
						distEnd += hatchesDist;
						off -= hatchesDist;
						if (distEnd != 0 || pathElementEnd->inX != pathElementEnd->inY) {
							hatchArena.emplace_back();
							HatchPoint& hatchPoint = hatchArena.back();
							hatchPoint.n = n;
							hatchPoint.x = pathElementStart->x + (lineHasX & off);
							hatchPoint.y = pathElementStart->y + (lineHasY & off);
						}
					}
				}
				dist = distEnd;
			}
		}
		
		std::sort(hatchArena.begin(), hatchArena.end(), [](const HatchPoint& a, const HatchPoint& b) {
			return a.n < b.n || a.n == b.n && a.x < b.x;
		});
		
		int lastN = 0;
		D3DXVECTOR3 pnt1;
		bool hasPnt1 = false;
		for (const HatchPoint& hatchPoint : hatchArena) {
			
			if (hatchPoint.n != lastN) {
				lastN = hatchPoint.n;
				hasPnt1 = false;
			}
			
			if (!hasPnt1) {
				pnt1 = { (float)hatchPoint.x, (float)hatchPoint.y, 0.F };
				hasPnt1 = true;
				continue;
			}
			
			D3DXVECTOR3 pnt2 = D3DXVECTOR3{
				(float)hatchPoint.x,
				(float)hatchPoint.y,
				0.F
			};
			hasPnt1 = false;
			
			drawIfOutOfSpace(2);
			++preparedOutlinePtr->hatchesCount;
			*vertexIt = Vertex{ pnt1.x, pnt1.y, 0.F, params.outlineColor };
			++vertexIt;
			*vertexIt = Vertex{ pnt2.x, pnt2.y, 0.F, params.outlineColor };
			++vertexIt;
			consumeVertexBufferSpace(2);
			lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_HATCH;
		}
		preparedOutlinePtr->hatchesComplete = true;
	}
}

bool Graphics::worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out) {
	return camera.worldToScreen(device, vec, out);
}

void Graphics::preparePoint(const DrawPointCallParams& params) {
	D3DXVECTOR3 p{ (float)params.posX, (float)params.posY, 0.F };
	logOnce(fprintf(logfile, "drawPoint called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));
	
	D3DXVECTOR3 sp;
	if (!worldToScreen(p, &sp)) return;

	/*  54321012345 (+)
	*  +-----------+
	*  |           | 5 (+)
	*  |           | 4
	*  |           | 3
	*  | 2        4| 2
	*  |           | 1
	*  |     x     | 0
	*  | 1        3| 1
	*  |           | 2
	*  |           | 3
	*  |           | 4
	*  |           | 5 (-)
	   +-----------+*/

	drawIfOutOfSpace(14);
	consumeVertexBufferSpace(14);

	const D3DCOLOR fillColor = params.fillColor;

	*vertexIt = Vertex{ sp.x - 4, sp.y - 1, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 4, sp.y + 2, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 5, sp.y - 1, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 5, sp.y + 2, 0.F, fillColor };
	++vertexIt;

	/*  54321012345 (+)
	*  +-----------+
	*  |    2  4   | 5 (+)
	*  |           | 4
	*  |           | 3
	*  |           | 2
	*  |           | 1
	*  |     x     | 0
	*  |           | 1
	*  |           | 2
	*  |           | 3
	*  |    1  3   | 4
	*  |           | 5 (-)
	   +-----------+*/

	// PADDING
	*vertexIt = Vertex{ sp.x + 5, sp.y + 2, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 1, sp.y - 4, 0.F, fillColor };
	++vertexIt;

	*vertexIt = Vertex{ sp.x - 1, sp.y - 4, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 1, sp.y + 5, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 2, sp.y - 4, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 2, sp.y + 5, 0.F, fillColor };
	++vertexIt;

	/*  54321012345 (+)
	*  +-----------+
	*  |           | 5 (+)
	*  |     4     | 4
	*  |           | 3
	*  |           | 2
	*  |           | 1
	*  |  1  x   2 | 0
	*  |           | 1
	*  |           | 2
	*  |     3     | 3
	*  |           | 4
	*  |           | 5 (-)
	   +-----------+*/

	const D3DCOLOR outlineColor = params.outlineColor;

	*vertexIt = Vertex{ sp.x - 3, sp.y, 0.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 4, sp.y, 0.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x, sp.y - 3, 0.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x, sp.y + 4, 0.F, outlineColor };
	++vertexIt;
	++numberOfPointsPrepared;
}

IDirect3DSurface9* Graphics::getOffscreenSurface(D3DSURFACE_DESC* renderTargetDescPtr) {
	D3DSURFACE_DESC renderTargetDesc;
	if (!renderTargetDescPtr) {
		CComPtr<IDirect3DSurface9> renderTarget;
		if (FAILED(device->GetRenderTarget(0, &renderTarget))) {
			logwrap(fputs("GetRenderTarget failed\n", logfile));
			return nullptr;
		}
		SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
		if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
			logwrap(fputs("GetDesc failed\n", logfile));
			return nullptr;
		}
		renderTargetDescPtr = &renderTargetDesc;
	}
	if (offscreenSurfaceWidth != renderTargetDescPtr->Width || offscreenSurfaceHeight != renderTargetDescPtr->Height) {
		offscreenSurface = nullptr;
		if (FAILED(device->CreateOffscreenPlainSurface(renderTargetDescPtr->Width,
			renderTargetDescPtr->Height,
			renderTargetDescPtr->Format,
			D3DPOOL_SYSTEMMEM,
			&offscreenSurface,
			NULL))) {
			logwrap(fputs("CreateOffscreenPlainSurface failed\n", logfile));
			return nullptr;
		}
		offscreenSurfaceWidth = renderTargetDescPtr->Width;
		offscreenSurfaceHeight = renderTargetDescPtr->Height;
	}
	return offscreenSurface;
}

bool Graphics::takeScreenshotBegin(IDirect3DDevice9* device) {
	logwrap(fputs("takeScreenshotBegin called\n", logfile));
	
	PERFORMANCE_MEASUREMENT_START
	PERFORMANCE_MEASUREMENT_ON_EXIT(takeScreenshotBegin)
	
	CComPtr<IDirect3DSurface9> oldRenderTarget;
	if (FAILED(device->GetRenderTarget(0, &oldRenderTarget))) {
		logwrap(fputs("GetRenderTarget failed\n", logfile));
		return false;
	}
	
	D3DSURFACE_DESC renderTargetDesc;
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(oldRenderTarget->GetDesc(&renderTargetDesc))) {
		logwrap(fputs("GetDesc failed\n", logfile));
		return false;
	}
	
	if (!altRenderTarget) {
		if (FAILED(device->CreateRenderTarget(renderTargetDesc.Width,
				renderTargetDesc.Height,
				renderTargetDesc.Format,
				renderTargetDesc.MultiSampleType,
				renderTargetDesc.MultiSampleQuality,
				FALSE,
				&altRenderTarget,
				NULL))) {
				logwrap(fputs("CreateRenderTarget failed\n", logfile));
			return false;
		}
	}
	
	altRenderTargetLifeRemaining = 41;

	if (FAILED(device->SetRenderTarget(0, altRenderTarget))) {
		logwrap(fputs("SetRenderTarget failed\n", logfile));
		return false;
	}
	
	struct SetOldRenderTargetOnFail {
	public:
		~SetOldRenderTargetOnFail() {
			if (success) return;
			device->SetRenderTarget(0, oldRenderTarget);
			oldRenderTarget = nullptr;
		}
		IDirect3DDevice9* device = nullptr;
		CComPtr<IDirect3DSurface9>& oldRenderTarget;
		bool success = false;
	} setOldRenderTargetOnFail { device, oldRenderTarget };
	
	if (FAILED(device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.F, 0))) {
		logwrap(fputs("Clear failed\n", logfile));
		return false;
	}
	
	setOldRenderTargetOnFail.success = true;
	gamesRenderTarget = oldRenderTarget;
	return true;
}

void Graphics::takeScreenshotDebug(IDirect3DDevice9* device, const wchar_t* filename) {
	std::vector<unsigned char> gameImage;
	unsigned int width = 0;
	unsigned int height = 0;
	if (!getFramebufferData(device, gameImage, nullptr, nullptr, &width, &height)) return;
	pngRelated.writePngToPath(filename, width, height, gameImage.data());

}

void Graphics::takeScreenshotEnd(IDirect3DDevice9* device) {
	
	PERFORMANCE_MEASUREMENT_START
	
	CComPtr<IDirect3DSurface9> renderTarget;
	if (FAILED(device->GetRenderTarget(0, &renderTarget))) {
		logwrap(fputs("GetRenderTarget failed\n", logfile));
		return;
	}
	D3DSURFACE_DESC renderTargetDesc;
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
		logwrap(fputs("GetDesc failed\n", logfile));
		return;
	}
	std::vector<unsigned char> boxesImage;
	if (!getFramebufferData(device, boxesImage, renderTarget, &renderTargetDesc)) return;


	std::vector<unsigned char> gameImage;
	if (!getFramebufferData(device, gameImage, gamesRenderTarget, &renderTargetDesc)) return;
	
	PERFORMANCE_MEASUREMENT_END(takeScreenshotEnd_getFramebufferData)
	
	PERFORMANCE_MEASUREMENT_START
	
	if (settings.useSimplePixelBlender) {
		cpuPixelBlenderSimple(gameImage.data(), boxesImage.data(), renderTargetDesc.Width, renderTargetDesc.Height);
	} else {
		cpuPixelBlenderComplex(gameImage.data(), boxesImage.data(), renderTargetDesc.Width, renderTargetDesc.Height);
	}
	
	PERFORMANCE_MEASUREMENT_END(takeScreenshotEnd_pixelBlender)
	
	PERFORMANCE_MEASUREMENT_START
	
	pngRelated.saveScreenshotData(renderTargetDesc.Width, renderTargetDesc.Height, gameImage.data());
	
	PERFORMANCE_MEASUREMENT_END(takeScreenshotEnd_writeScreenshot)
}

void Graphics::takeScreenshotSimple(IDirect3DDevice9* device) {
	CComPtr<IDirect3DSurface9> renderTarget;
	if (FAILED(device->GetRenderTarget(0, &renderTarget))) {
		logwrap(fputs("GetRenderTarget failed\n", logfile));
		return;
	}
	D3DSURFACE_DESC renderTargetDesc;
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
		logwrap(fputs("GetDesc failed\n", logfile));
		return;
	}
	static std::vector<unsigned char> gameImage;
	gameImage.clear();
	if (!getFramebufferData(device, gameImage, renderTarget, &renderTargetDesc)) return;

	if (!settings.dontUseScreenshotTransparency) {
		union Pixel {
			struct { unsigned char r, g, b, a; };
			int value;
		};
		Pixel* gameImagePtr = (Pixel*)gameImage.data();
		const size_t offLimit = renderTargetDesc.Width * renderTargetDesc.Height;
		for (size_t off = 0; off < offLimit; ++off)
		{
			Pixel& d = *gameImagePtr;

			d.a ^= 255;
			unsigned int maxColor = d.r;
			if (d.g > maxColor) maxColor = d.g;
			if (d.b > maxColor) maxColor = d.b;
			if (maxColor > d.a) d.a = maxColor;
			++gameImagePtr;
		}
	}

	pngRelated.saveScreenshotData(renderTargetDesc.Width, renderTargetDesc.Height, gameImage.data());

}

bool Graphics::getFramebufferData(IDirect3DDevice9* device,
		std::vector<unsigned char>& buffer,
		IDirect3DSurface9* renderTarget,
		D3DSURFACE_DESC* renderTargetDescPtr,
		unsigned int* widthPtr,
		unsigned int* heightPtr) {
	CComPtr<IDirect3DSurface9> renderTargetComPtr;
	if (!renderTarget) {
		if (FAILED(device->GetRenderTarget(0, &renderTargetComPtr))) {
			logwrap(fputs("GetRenderTarget failed\n", logfile));
			return false;
		}
		renderTarget = renderTargetComPtr;
	}
	D3DSURFACE_DESC renderTargetDesc;
	if (!renderTargetDescPtr) {
		SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
		if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
			logwrap(fputs("GetDesc failed\n", logfile));
			return false;
		}
		renderTargetDescPtr = &renderTargetDesc;
	}
	if (widthPtr) *widthPtr = renderTargetDescPtr->Width;
	if (heightPtr) *heightPtr = renderTargetDescPtr->Height;

	IDirect3DSurface9* offscreenSurface = getOffscreenSurface(renderTargetDescPtr);
	if (!offscreenSurface) return false;
	if (FAILED(device->GetRenderTargetData(renderTarget, offscreenSurface))) {
		logwrap(fprintf(logfile, "GetRenderTargetData failed. renderTarget is: %p. offscreenSurface is %p. gamesRenderTarget is %p\n", renderTarget, offscreenSurface, gamesRenderTarget.p));
		return false;
	}

	D3DLOCKED_RECT lockedRect;
	RECT rect;
	rect.left = 0;
	rect.right = renderTargetDescPtr->Width;
	rect.top = 0;
	rect.bottom = renderTargetDescPtr->Height;
	if (FAILED(offscreenSurface->LockRect(&lockedRect, &rect, D3DLOCK_READONLY))) {
		logwrap(fputs("LockRect failed\n", logfile));
		return false;
	}

	unsigned int imageSize = renderTargetDescPtr->Width * renderTargetDescPtr->Height;

	buffer.resize(imageSize * 4);
	memcpy(buffer.data(), lockedRect.pBits, imageSize * 4);

	offscreenSurface->UnlockRect();

	return true;
}

void Graphics::setTransformMatrices3DProjection(IDirect3DDevice9* device) {
	
	if (currentTransformSet == CURRENT_TRANSFORM_3D_PROJECTION) return;
	
	rememberTransforms(device);
	
	float m = camera.valuesUse.coordCoefficient / 1000.F;
	D3DXMATRIX world {
		m,   0.F, 0.F, 0.F,
		0.F, 0.F, m,   0.F,
		0.F, m,   0.F, 0.F,
		0.F, 0.F, 0.F, 1.F
	};
	device->SetTransform(D3DTS_WORLD, &world);
	
	// D3D axes:
	// Z points into the screen
	// X points right
	// Y points up
	
	// UE3 axes:
	// Z points up
	// Y points away from the fighting arena, to the right from P1's character's side
	// X points to the right of arena
	
	// ArcSys axes:
	// Z points away from and out of the screen
	// Y points up
	// X points right
	
	// yaw goes from positive direction of x to positive direction of y in UE3 coordinate space
	// pitch goes from positive direction of x to positive direction of z
	// roll goes from positive direction of z to positive direction of y
	
	m = PI / 32768.F;
	D3DXMATRIX view;
	D3DXMATRIX mat1;
	// The reason we apply translation first is because we need to move everything to where the camera is before rotating.
	// We rotate everything in the opposite direction to undo the camera's rotation.
	// The camera itself, if we please view it as a model for a second here, was rotated roll, then pitch, then yaw, then translated.
	// So to do the reverse of that, we have to untranslate, unyaw, unpitch and then unroll.
	D3DXMatrixTranslation(&mat1, -camera.valuesUse.pos.x, -camera.valuesUse.pos.y, -camera.valuesUse.pos.z);
	D3DXMATRIX mat2;
	// The reason the angle is not reversed here is because the function's rotation is defined as:
	// "Angles are measured clockwise when looking along the rotation axis toward the origin."
	// Clearly, this function rotates from positive direction of x to positive direction of y which is exactly what we need
	D3DXMatrixRotationZ(&mat2, (float)-camera.valuesUse.yaw * m);
	D3DXMatrixMultiply(&view, &mat1, &mat2);
	// The reason the angle is reversed here is because the function's rotation is defined as:
	// "Angles are measured clockwise when looking along the rotation axis toward the origin."
	// Clearly, this function rotates from positive direction of z to positive direction of x which is the opposite of what we need
	D3DXMatrixRotationY(&mat2, (float)camera.valuesUse.pitch * m);
	mat1 = view;
	D3DXMatrixMultiply(&view, &mat1, &mat2);
	// The reason the angle is reversed here is because the function's rotation is defined as:
	// "Angles are measured clockwise when looking along the rotation axis toward the origin."
	// Clearly, this function rotates from positive direction of y to positive direction of z which is the opposite of what we need
	D3DXMatrixRotationX(&mat2, (float)camera.valuesUse.roll * m);
	mat1 = view;
	D3DXMatrixMultiply(&view, &mat1, &mat2);
	device->SetTransform(D3DTS_VIEW, &view);
	
	float t = 1.F / tanf(camera.valuesUse.fov / 360.F * PI);  // this is from Altimor's formula
	
	// A thing of note is that D3D automatically divides x and y by z at the end.
	// Tests show if w does not equal z, nothing gets drawn on the screen. So w must equal z.
	// X goes from -1 (left side) to 1 (right side).
	// Y goes from 1 (top side) to -1 (bottom side).
	// Z goes into the screen.
	D3DXMATRIX projection;
	projection = {
		1.F / viewportW, 1.F / viewportH,            1.F, 1.F,  // UE3 uses left-hand coordinates, and D3D also uses left-hand
		t,               0.F,                        0.F, 0.F,
		0.F,             t * viewportW / viewportH,  0.F, 0.F,
		0.F,             0.F,                        0.F, 0.F
	};  // the "1.F / vw, 1.F / vh" at the top left are from Altimor's formula, because multiplying and dividing them by Z will give us that half a pixel
	device->SetTransform(D3DTS_PROJECTION, &projection);
	
	currentTransformSet = CURRENT_TRANSFORM_3D_PROJECTION;
	
}

void Graphics::setTransformMatricesPlain2D(IDirect3DDevice9* device) {
	
	if (currentTransformSet == CURRENT_TRANSFORM_2D_PROJECTION) return;
	
	D3DXMATRIX projection =
    {
        2.F / viewportW,  0.F,              0.F, 0.F,
        0.F,              -2.F / viewportH, 0.F, 0.F,
        0.F,              0.F,              1.F, 0.F,
        -1.F,             1.F,              0.F, 1.F
    };
	
    rememberTransforms(device);
    device->SetTransform(D3DTS_WORLD, &identity);
    device->SetTransform(D3DTS_VIEW, &identity);
    device->SetTransform(D3DTS_PROJECTION, &projection);
    
    currentTransformSet = CURRENT_TRANSFORM_2D_PROJECTION;
}

void Graphics::rememberTransforms(IDirect3DDevice9* device) {
	if (currentTransformSet != CURRENT_TRANSFORM_DEFAULT) return;
	device->GetTransform(D3DTS_WORLD, &prevWorld);
	device->GetTransform(D3DTS_VIEW, &prevView);
	device->GetTransform(D3DTS_PROJECTION, &prevProjection);
}

void Graphics::takeScreenshotMain(IDirect3DDevice9* device, bool useSimpleVerion) {
	if (useSimpleVerion) {
		takeScreenshotSimple(device);
		return;
	}
	if (imInDangerReceived && settings.dodgeObsRecording && !endSceneAndPresentHooked) return;  // let's not trigger the hook attempt more than once per frame
	CComPtr<IDirect3DStateBlock9> oldState = nullptr;
	device->CreateStateBlock(D3DSBT_ALL, &oldState);
	if (!takeScreenshotBegin(device)) return;
	
	// thanks to WorseThanYou for coming up with this box blending sequence
	
	// Because render target starts with color 0 (black) it would
	// make the blending result darker if we blend colors into it
	// with transparency.
	// So we need this step which is aimed at getting the base color into the render target.
	device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	renderStateValues[RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

	device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
	renderStateValues[RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, TRUE);
	device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
	device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);

	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	renderStateValues[RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	renderStateValues[RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
	renderStateValues[RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
	device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	renderStateValues[RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	device->SetRenderState(D3DRS_LIGHTING, FALSE);
	
	device->SetVertexShader(nullptr);
	device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
	renderStateValues[RenderStateType(VERTEX)] = RenderStateValue(VERTEX, NONTEXTURE);
	device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
	device->SetPixelShader(nullptr);
	renderStateValues[RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	device->SetTexture(0, nullptr);
	renderStateValues[RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, NONE);
	
	renderStateValues[RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, NONE);
	currentTransformSet = CURRENT_TRANSFORM_DEFAULT;
	
	screenshotStage = SCREENSHOT_STAGE_BASE_COLOR;
	
	PERFORMANCE_MEASUREMENT_START
	
	drawAll();
	
	PERFORMANCE_MEASUREMENT_END(screenshotDrawAll1)
	
	logOnce(fputs("drawAll() (for screenshot) call successful\n", logfile));
	
	// This step blends the colors with alpha and just does everything normally
	screenshotStage = SCREENSHOT_STAGE_FINAL;
	stencil.clear(device);
	
	PERFORMANCE_MEASUREMENT_START
	
	drawAll();
	
	PERFORMANCE_MEASUREMENT_END(screenshotDrawAll2)
	
	takeScreenshotEnd(device);
	screenshotStage = SCREENSHOT_STAGE_NONE;
	device->SetRenderTarget(0, gamesRenderTarget);
	gamesRenderTarget = nullptr;
	stencil.onEndSceneEnd(device);
	oldState->Apply();
	bringBackOldTransform(device);
	
    device->SetTransform(D3DTS_WORLD, &prevWorld);
    device->SetTransform(D3DTS_VIEW, &prevView);
    device->SetTransform(D3DTS_PROJECTION, &prevProjection);
}

void Graphics::advanceRenderState(RenderStateDrawingWhat newState) {
	RenderStateValueStack& to = requiredRenderState[screenshotStage][newState];
	for (int i = 0; i < RENDER_STATE_TYPE_LAST; ++i) {
		RenderStateValue newVal = to[i];
		if (renderStateValues[i] != newVal) {
			renderStateValueHandlers[i]->handleChange(newVal);
			renderStateValues[i] = newVal;
		}
	}
	drawingWhat = newState;
}

bool Graphics::initializeVertexBuffers() {
	if (failedToCreateVertexBuffers) return false;
	if (vertexBuffer) return true;
	if (FAILED(device->CreateVertexBuffer(sizeof(Vertex) * vertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vertexBuffer, NULL))) {
		logwrap(fputs("CreateVertexBuffer failed\n", logfile));
		failedToCreateVertexBuffers = true;
		return false;
	}
	vertexArena.resize(vertexBufferSize);

	return true;
}

void Graphics::resetVertexBuffer() {
	vertexBufferRemainingSize = vertexBufferSize;
	vertexBufferLength = 0;
	vertexIt = vertexArena.data();
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	vertexBufferPosition = 0;
	preparingTextureVertexBuffer = false;
	renderingTextureVertices = false;
}

void DrawData::clearBoxes() {
	hurtboxes.clear();
	hitboxes.clear();
	pushboxes.clear();
	interactionBoxes.clear();
	points.clear();
	throwBoxes.clear();
	needTakeScreenshot = false;
}

void DrawData::clearInputs() {
	for (int i = 0; i < 2; ++i) {
		inputs[i].clear();
		inputsSize[i] = 0;
	}
}

bool Graphics::drawIfOutOfSpace(unsigned int verticesCountRequired) {
	if (vertexBufferRemainingSize < verticesCountRequired) {
		drawAllPrepared();
		return true;
	}
	return false;
}

bool Graphics::textureDrawIfOutOfSpace(unsigned int verticesCountRequired) {
	if (textureVertexBufferRemainingSize < verticesCountRequired) {
		drawAllPrepared();
		return true;
	}
	return false;
}

void Graphics::prepareComplicatedHurtbox(const ComplicatedHurtbox& pairOfBoxesOrOneBox) {
	if (pairOfBoxesOrOneBox.hasTwo) {
		preparedArrayboxes.emplace_back();
		preparedArrayboxes.back().id = preparedArrayboxIdCounter++;
		BoundingRect boundingRect;
		prepareArraybox(pairOfBoxesOrOneBox.param1, true, &boundingRect, &outlinesOverrideArena);
		prepareArraybox(pairOfBoxesOrOneBox.param2, true, &boundingRect);
		PreparedArraybox& preparedArraybox = preparedArrayboxes.back();
		preparedArraybox.isComplete = true;
		preparedArraybox.boundingRect = boundingRect;
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_COMPLICATED_HURTBOX;
		outlines.insert(outlines.end(), outlinesOverrideArena.begin(), outlinesOverrideArena.end());
		outlinesOverrideArena.clear();
	} else {
		prepareArraybox(pairOfBoxesOrOneBox.param1, false);
	}
}

void DrawData::copyTo(DrawData* destination) {
	destination->hurtboxes = hurtboxes;
	destination->hitboxes = hitboxes;
	destination->pushboxes = pushboxes;
	destination->interactionBoxes = interactionBoxes;
	destination->points = points;
	destination->throwBoxes = throwBoxes;
	for (int i = 0; i < 2; ++i) {
		destination->inputs[i] = inputs[i];
		destination->inputsSize[i] = inputsSize[i];
	}
	destination->needTakeScreenshot = needTakeScreenshot;
}

void Graphics::HookHelp::FSuspendRenderingThreadHook(unsigned int InSuspendThreadFlags) {
	graphics.orig_FSuspendRenderingThread((char*)this, InSuspendThreadFlags);
	if (graphics.suspenderThreadId == GetCurrentThreadId()) {
		graphics.resetHook();
		ui.handleResetBefore();
	}
}

void Graphics::HookHelp::FSuspendRenderingThreadDestructorHook() {
	if (graphics.suspenderThreadId == GetCurrentThreadId() && !graphics.shutdown) {
		// We must recreate our resources before FSuspendRenderingThread resumes the graphics thread, so that
		// there's no race condition between us and the graphics thread
		ui.handleResetAfter();
	}
	graphics.orig_FSuspendRenderingThreadDestructor((char*)this);
}

IDirect3DTexture9* Graphics::getFramesTexture(IDirect3DDevice9* device) {
	if (failedToCreateFramesTexture) return nullptr;
	if (framesTexture) return framesTexture;
	CComPtr<IDirect3DTexture9> systemTexture;
	const PngResource& packedFramesTexture = ui.getPackedFramesTexture();
	if (FAILED(device->CreateTexture(packedFramesTexture.width, packedFramesTexture.height, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &systemTexture, NULL))) {
		logwrap(fputs("CreateTexture failed\n", logfile));
		failedToCreateFramesTexture = true;
		return nullptr;
	}
	if (FAILED(device->CreateTexture(packedFramesTexture.width, packedFramesTexture.height, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &framesTexture, NULL))) {
		logwrap(fputs("CreateTexture (2) failed\n", logfile));
		failedToCreateFramesTexture = true;
		return nullptr;
	}
	D3DLOCKED_RECT lockedRect;
	if (FAILED(systemTexture->LockRect(0, &lockedRect, NULL, NULL))) {
		logwrap(fputs("texture->LockRect failed\n", logfile));
		failedToCreateFramesTexture = true;
		return nullptr;
	}
	packedFramesTexture.bitBlt(lockedRect.pBits, lockedRect.Pitch, 0, 0, 0, 0, packedFramesTexture.width, packedFramesTexture.height);
	if (FAILED(systemTexture->UnlockRect(0))) {
		logwrap(fputs("texture->UnlockRect failed\n", logfile));
		failedToCreateFramesTexture = true;
		return nullptr;
	}
	if (FAILED(device->UpdateTexture(systemTexture, framesTexture))) {
		logwrap(fputs("UpdateTexture failed\n", logfile));
		failedToCreateFramesTexture = true;
		return nullptr;
	}
	logwrap(fprintf(logfile, "Initialized packed frames texture successfully. Width: %u; Height: %u.\n", packedFramesTexture.width, packedFramesTexture.height));
	return framesTexture;
}

IDirect3DTexture9* Graphics::getOutlinesRTSamplingTexture(IDirect3DDevice9* device) {
	if (failedToCreateOutlinesRTSamplingTexture) return nullptr;
	if (outlinesRTSamplingTexture) return outlinesRTSamplingTexture;
	
	D3DSURFACE_DESC renderTargetDesc;
	CComPtr<IDirect3DSurface9> renderTarget;
	if (FAILED(device->GetRenderTarget(0, &renderTarget))) {
		failedToCreateOutlinesRTSamplingTexture = true;
		logwrap(fputs("GetRenderTarget failed\n", logfile));
		return nullptr;
	}
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
		failedToCreateOutlinesRTSamplingTexture = true;
		logwrap(fputs("GetDesc failed\n", logfile));
		return nullptr;
	}
	
	if (FAILED(device->CreateTexture(renderTargetDesc.Width, renderTargetDesc.Height, 1, D3DUSAGE_RENDERTARGET, renderTargetDesc.Format, D3DPOOL_DEFAULT, &outlinesRTSamplingTexture, NULL))) {
		logwrap(fputs("CreateTexture (3) failed\n", logfile));
		failedToCreateOutlinesRTSamplingTexture = true;
		return nullptr;
	}
	logwrap(fprintf(logfile, "Initialized packed frames texture successfully. Width: %u; Height: %u.\n", renderTargetDesc.Width, renderTargetDesc.Height));
	return outlinesRTSamplingTexture;
}

void Graphics::compilePixelShader() {
	
	if (failedToCompilePixelShader || pixelShaderCode) return;
	
	HRSRC resourceInfoHandle = FindResourceW(hInstance, MAKEINTRESOURCEW(IDR_MY_PIXEL_SHADER), L"HLSL");
	if (!resourceInfoHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "FindResource failed: %ls\n", winErr.getMessage()));
		failedToCompilePixelShader = true;
		return;
	}
	HGLOBAL resourceHandle = LoadResource(hInstance, resourceInfoHandle);
	if (!resourceHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "LoadResource failed: %ls\n", winErr.getMessage()));
		failedToCompilePixelShader = true;
		return;
	}
	LPVOID pixelShaderTxtData = LockResource(resourceHandle);
	if (!pixelShaderTxtData) {
		WinError winErr;
		logwrap(fprintf(logfile, "LockResource failed: %ls\n", winErr.getMessage()));
		failedToCompilePixelShader = true;
		return;
	}
	DWORD txtSize = SizeofResource(hInstance, resourceInfoHandle);
	if (!txtSize) {
		WinError winErr;
		logwrap(fprintf(logfile, "SizeofResource failed: %ls\n", winErr.getMessage()));
		failedToCompilePixelShader = true;
		return;
	}
	
	CComPtr<ID3DBlob> code;
	CComPtr<ID3DBlob> errorMsgs;
	if (FAILED(D3DCompile(
		  pixelShaderTxtData,
		  txtSize,
		  "MyPixelShader",
		  NULL,
		  NULL,
		  "main",
		  "ps_3_0",
		  D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
		  NULL,
		  &code,
		  &errorMsgs
		))) {
		if (!errorMsgs) {
			logwrap(fprintf(logfile, "D3DCompile failed\n"));
		} else {
			shaderCompilationError = std::string((const char*)errorMsgs->GetBufferPointer(), (size_t)errorMsgs->GetBufferSize());
			logwrap(fprintf(logfile, "D3DCompile failed: %s\n", shaderCompilationError.c_str()));
		}
		failedToCompilePixelShader = true;
		return;
	}
	
	SIZE_T shaderSize = code->GetBufferSize();
	const DWORD* codeData = (const DWORD*)code->GetBufferPointer();
	
	pixelShaderCode = malloc(shaderSize);
	if (!pixelShaderCode) {
		logwrap(fprintf(logfile, "malloc(code->GetBufferSize()) failed: %u\n", pixelShaderCodeSize));
		failedToCompilePixelShader = true;
		return;
	}
	pixelShaderCodeSize = shaderSize;
	memcpy(pixelShaderCode, codeData, pixelShaderCodeSize);
}

void Graphics::getShaderCompilationError(const std::string** result) {
	*result = nullptr;
	if (shaderCompilationError.empty()) return;
	*result = &shaderCompilationError;
}

IDirect3DPixelShader9* Graphics::getPixelShader(IDirect3DDevice9* device) {
	if (failedToCreatePixelShader) return nullptr;
	if (pixelShader) return pixelShader;
	if (!pixelShaderCode) {
		failedToCreatePixelShader = true;
		return nullptr;
	}
	if (FAILED(device->CreatePixelShader((const DWORD*)pixelShaderCode, &pixelShader))) {
		failedToCreatePixelShader = true;
		logwrap(fprintf(logfile, "CreatePixelShader failed\n"));
		return nullptr;
	}
	return pixelShader;
}

void Graphics::preparePixelShader(IDirect3DDevice9* device) {
	
	IDirect3DTexture9* tex = getOutlinesRTSamplingTexture(device);
	if (!tex) return;
	CComPtr<IDirect3DSurface9> renderTarget;
	if (FAILED(device->GetRenderTarget(0, &renderTarget))) {
		logwrap(fputs("GetRenderTarget failed\n", logfile));
		return;
	}
	CComPtr<IDirect3DSurface9> surfaceLevel;
	if (FAILED(tex->GetSurfaceLevel(0, &surfaceLevel))) {
		logwrap(fputs("GetSurfaceLevel failed\n", logfile));
		return;
	}
	if (FAILED(device->StretchRect(renderTarget, NULL, surfaceLevel, NULL, D3DTEXF_NONE))) {
		logwrap(fputs("StretchRect failed\n", logfile));
		return;
	}
	compilePixelShader();
	IDirect3DPixelShader9* shader = getPixelShader(device);
	if (!shader) return;
	
	device->SetPixelShader(shader);
	device->SetTexture(0, tex);
	
	
	D3DSURFACE_DESC renderTargetDesc;
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
		logwrap(fputs("GetDesc failed\n", logfile));
		return;
	}
	float screenSizeConstant[4] { (float)renderTargetDesc.Width, (float)renderTargetDesc.Height, 0.F, 0.F };
	device->SetPixelShaderConstantF(0, screenSizeConstant, 1);
}

void Graphics::heartbeat() {
	receiveDanger();
	afterDraw();
	checkAndHookEndSceneAndPresent(false);
}

void Graphics::cpuPixelBlenderComplex(void* gameImage, const void* boxesImage, int width, int height) {
	// Thanks to WorseThanYou for writing this CPU pixel blender
	union Pixel {
		struct { unsigned char r, g, b, a; };
		int value;
	};
	Pixel* gameImagePtr = (Pixel*)gameImage;
	Pixel* boxesImagePtr = (Pixel*)boxesImage;
	const size_t offLimit = width * height * 4;
	for (size_t off = 0; off < offLimit; off += 4)
	{
		Pixel& d = *gameImagePtr;
		Pixel& s = *boxesImagePtr;
		
		if (s.a != 255) {
			s.a /= 2;
		}

		d.a ^= 255;
		
		// I added this part to improve visibility of effects like Ky RTL Aura
		unsigned int maxColor = d.r;
		if (d.g > maxColor) maxColor = d.g;
		if (d.b > maxColor) maxColor = d.b;
		if (maxColor > d.a) d.a = maxColor;
		
		unsigned char daInv = ~d.a, saInv = 255 ^ s.a;  // This part written by WorseThanYou
		
		// I added this part to improve visibility of red hitboxes' outlines over things like Ramlethal mirror color f.S
		if (d.a >= 128 && s.a == 255 && s.value != 0 && s.value != 0xFFFFFFF) {
			
			int diffSum = abs((int)s.r - (int)d.r)
				+ abs((int)s.g - (int)d.g)
				+ abs((int)s.b - (int)d.b);
			
			if (diffSum < 21) {
				
				d.value &= 0xFF000000;
				
				++gameImagePtr;
				++boxesImagePtr;
				continue;
			}
		}
		
		// This part written by WorseThanYou
		d.r = (daInv * s.r + d.a * (s.r * s.a + d.r * saInv) / 255) / 255;
		d.g = (daInv * s.g + d.a * (s.g * s.a + d.g * saInv) / 255) / 255;
		d.b = (daInv * s.b + d.a * (s.b * s.a + d.b * saInv) / 255) / 255;
		d.a = (daInv * s.a + d.a * (255 * 255 - daInv * saInv) / 255) / 255;
		++gameImagePtr;
		++boxesImagePtr;
	}
}

void Graphics::cpuPixelBlenderSimple(void* gameImage, const void* boxesImage, int width, int height) {
	// Thanks to WorseThanYou for writing this CPU pixel blender
	union Pixel {
		struct { unsigned char r, g, b, a; };
		int value;
	};
	Pixel* gameImagePtr = (Pixel*)gameImage;
	Pixel* boxesImagePtr = (Pixel*)boxesImage;
	const size_t offLimit = width * height * 4;
	for (size_t off = 0; off < offLimit; off += 4)
	{
		Pixel& d = *gameImagePtr;
		Pixel& s = *boxesImagePtr;
		
		if (s.a != 255) {
			s.a /= 2;
		}

		d.a ^= 255;
		
		unsigned char daInv = ~d.a, saInv = 255 ^ s.a;
		d.r = (daInv * s.r + d.a * (s.r * s.a + d.r * saInv) / 255) / 255;
		d.g = (daInv * s.g + d.a * (s.g * s.a + d.g * saInv) / 255) / 255;
		d.b = (daInv * s.b + d.a * (s.b * s.a + d.b * saInv) / 255) / 255;
		d.a = (daInv * s.a + d.a * (255 * 255 - daInv * saInv) / 255) / 255;
		++gameImagePtr;
		++boxesImagePtr;
	}
}


void Graphics::RenderStateHandler(D3DRS_STENCILENABLE)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(D3DRS_STENCILENABLE, FALSE): graphics.device->SetRenderState(D3DRS_STENCILENABLE, FALSE); break;
		case RenderStateValue(D3DRS_STENCILENABLE, TRUE): graphics.device->SetRenderState(D3DRS_STENCILENABLE, TRUE); break;
	}
}
void Graphics::RenderStateHandler(D3DRS_ALPHABLENDENABLE)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(D3DRS_ALPHABLENDENABLE, FALSE): graphics.device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); break;
		case RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE): graphics.device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); break;
	}
}
void Graphics::RenderStateHandler(PIXEL_SHADER)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(PIXEL_SHADER, CUSTOM_PIXEL_SHADER): graphics.preparePixelShader(graphics.device); break;
		case RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER):
			graphics.device->SetPixelShader(nullptr);
			// texture will get reset to 0 by the RenderStateValue(TEXTURE) handler
			break;
	}
}
void Graphics::RenderStateHandler(TRANSFORM_MATRICES)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(TRANSFORM_MATRICES, 3D): graphics.setTransformMatrices3DProjection(graphics.device); break;
		case RenderStateValue(TRANSFORM_MATRICES, 2D): graphics.setTransformMatricesPlain2D(graphics.device); break;
	}
}
void Graphics::RenderStateHandler(D3DRS_SRCBLEND)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA): graphics.device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); break;
		case RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_ONE): graphics.device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); break;
	}
}
void Graphics::RenderStateHandler(D3DRS_DESTBLEND)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_ZERO): graphics.device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO); break;
		case RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA): graphics.device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); break;
	}
}
void Graphics::RenderStateHandler(D3DRS_SRCBLENDALPHA)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE): graphics.device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE); break;
		case RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO): graphics.device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO); break;
	}
}
void Graphics::RenderStateHandler(D3DRS_DESTBLENDALPHA)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA): graphics.device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA); break;
		case RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO): graphics.device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO); break;
	}
}
void Graphics::RenderStateHandler(VERTEX)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(VERTEX, NONTEXTURE):
			graphics.device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
			graphics.device->SetStreamSource(0, graphics.vertexBuffer, 0, sizeof(Vertex));
			break;
		case RenderStateValue(VERTEX, TEXTURE):
			graphics.device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
			graphics.device->SetStreamSource(0, graphics.vertexBuffer, 0, sizeof(TextureVertex));
			break;
	}
}
void Graphics::RenderStateHandler(TEXTURE)::handleChange(RenderStateValue newValue) {
	switch (newValue) {
		case RenderStateValue(TEXTURE, NONE): graphics.device->SetTexture(0, nullptr); break;
		case RenderStateValue(TEXTURE, FOR_PIXEL_SHADER): break;  // just track the change
		case RenderStateValue(TEXTURE, ICONS):
			graphics.device->SetTexture(0, graphics.iconsTexture);
		    graphics.device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		    graphics.device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		    graphics.device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);
			break;
	}
}

void Graphics::receiveDanger() {
	if (imInDanger && !imInDangerReceived) {
		imInDangerReceived = true;
		SetEvent(responseToImInDanger);
	}
}

void Graphics::afterDraw() {
	if (altRenderTarget && --altRenderTargetLifeRemaining < 0) {
		altRenderTargetLifeRemaining = 0;
		altRenderTarget = nullptr;
	}
}

bool Graphics::canDrawOnThisFrame() const {
	return !(imInDangerReceived && settings.dodgeObsRecording && !endSceneAndPresentHooked);
}

bool Graphics::drawingPostponed() const {
	return settings.dodgeObsRecording && endSceneAndPresentHooked;
}

// Draw boxes, without UI, and take a screenshot if needed
// Runs on the graphics thread
void Graphics::executeBoxesRenderingCommand(IDirect3DDevice9* device) {
	graphics.graphicsThreadId = GetCurrentThreadId();
	graphics.onEndSceneStart(device);
	drawOutlineCallParamsManager.onEndSceneStart();
	camera.onEndSceneStart();
	
	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	viewportW = (float)viewport.Width;
	viewportH = (float)viewport.Height;
	
	bool doYourThing = !dontShowBoxes
		|| (graphics.drawDataUse.inputsSize[0] || graphics.drawDataUse.inputsSize[1])
		&& !noNeedToDrawPoints;
		
	if (!*aswEngine) {
		// since we store pointers to hitbox data instead of copies of it, when aswEngine disappears those are gone and we get a crash if we try to read them
		graphics.drawDataUse.clear();
	}

	if (doYourThing) {
		if (graphics.drawDataUse.needTakeScreenshot && !settings.dontUseScreenshotTransparency) {
			logwrap(fputs("Running the branch with if (needToTakeScreenshot)\n", logfile));
			graphics.takeScreenshotMain(device, false);
		}
		graphics.drawAll();
		if (graphics.drawDataUse.needTakeScreenshot && settings.dontUseScreenshotTransparency) {
			graphics.takeScreenshotMain(device, true);
		}

	} else if (graphics.drawDataUse.needTakeScreenshot) {
		graphics.takeScreenshotMain(device, true);
	}
	graphics.afterDraw();
	graphics.drawDataUse.needTakeScreenshot = false;
}

// can't stop preparing texture vertices
void Graphics::startPreparingTextureVertexBuffer() {
	if (preparingTextureVertexBuffer) return;
	textureVertexBufferPosition = 0;
	textureVertexBufferLength = calculateStartingTextureVertexBufferLength();
	textureVertexBufferRemainingSize = textureVertexBufferSize - textureVertexBufferLength;
	textureVertexIt = (TextureVertex*)vertexArena.data() + textureVertexBufferLength;
	preparingTextureVertexBuffer = true;
}

// can't switch back
void Graphics::switchToRenderingTextureVertices() {
	if (renderingTextureVertices) return;
	int positionBytes = vertexBufferPosition * sizeof Vertex;
	int remainder = positionBytes % sizeof TextureVertex;
	if (remainder) {
		positionBytes += sizeof TextureVertex - remainder;
	}
	textureVertexBufferPosition = positionBytes / sizeof TextureVertex;
	renderingTextureVertices = true;
}

void Graphics::prepareDrawInputs() {
	startPreparingTextureVertexBuffer();
	
	TextureBoxParams box;
	
	float coefW = viewportW / 1280.F;
	float invCoefW = 1280.F / viewportW;
	float extraH_in1280space = (viewportH * invCoefW - 720.0F) * 0.5F;  // if the monitor is wider, this is negative
	float extraH = extraH_in1280space * viewportH / 1280.F;  // if the monitor is wider, this is negative
	const float iconSize = 28.F;
	const float columnWidth = 30.F;
	const float rowHeight = 32.F;
	const float iconSizeMult = iconSize * coefW;
	const float startY = 140.F * coefW + extraH;
	const float rowHeightMult = rowHeight * coefW;
	const float columnWidthMult = columnWidth * coefW;
	
	for (int i = 0; i < 2; ++i) {
		size_t inputsSize = drawDataUse.inputsSize[i];
		const InputsDrawingCommandRow* rows = drawDataUse.inputs[i].data();
		
		box.yStart = startY;
		box.yEnd = box.yStart + iconSizeMult;
		
		int rowIndMin = (int)inputsSize - 18;
		if (rowIndMin < 0) rowIndMin = 0;
		for (int rowInd = (int)inputsSize - 1; rowInd >= rowIndMin; --rowInd) {
			const InputsDrawingCommandRow* row = rows +rowInd;
			
			float x;
			if (i == 0) {
				x = 20.F;
			} else {
				x = 1260.F - 30.F * row->count;
			}
			box.xStart = x * coefW;
			box.xEnd = box.xStart + iconSizeMult;
			
			for (int column = 0; column < row->count; ++column) {
				const InputsDrawingCommand* cmd = row->cmds + column;
				
				box.color = cmd->dark ? 0xffa0a0a0 : 0xffffffff;
				
				const InputsIcon* icon = inputsIcon + cmd->icon;
				box.uStart = icon->uStart;
				box.vStart = icon->vStart;
				box.uEnd = icon->uEnd;
				box.vEnd = icon->vEnd;
				prepareTextureBox(box);
				
				box.xStart += columnWidthMult;
				box.xEnd += columnWidthMult;
			}
			
			box.yStart += rowHeightMult;
			box.yEnd += rowHeightMult;
		}
	}
}

int Graphics::calculateStartingTextureVertexBufferLength() {
	size_t lengthBytes = vertexBufferLength * sizeof Vertex;
	size_t remainder = lengthBytes % sizeof TextureVertex;
	if (remainder) lengthBytes += sizeof TextureVertex - remainder;
	size_t result = lengthBytes == 0 ? 0 : lengthBytes / sizeof TextureVertex;
	
	if (result > textureVertexBufferSize) {
		return textureVertexBufferSize;
	}
	return result;
}
