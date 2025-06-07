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
#include "WError.h"
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
#include <WinError.h>
#include "HandleWrapper.h"

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

#define PIPE_NAME "CaptureHook_Pipe"  // from obs-studio\plugins\win-capture\graphics-hook-info.h

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
	
	stack[RENDER_STATE_DRAWING_TEXT] = stack[RENDER_STATE_DRAWING_TEXTURES];
	stack[RENDER_STATE_DRAWING_TEXT][RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, FONT);
	
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
	if (!checkAndHookBeginSceneAndPresent(true)) error = true;
	
	static const int sinArrayStart[] = { 0,1,3,5,6,8,10,12,13,15,17,19,20,22,24,26 };
	sinTable = (const int*)sigscan("GuiltyGearXrd.exe:.rdata", (const char*)sinArrayStart, sizeof sinArrayStart);
	if (!sinTable) {
		logwrap(fputs("sinTable not found", logfile));
		error = true;
	}
	
	uintptr_t presentRectUsage = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 15 ?? ?? ?? ?? a1 ?? ?? ?? ?? 89 5c 24 38 89 5c 24 34 3b d3 75 03 8b 47 10",
		&error, "presentRectUsage");
	if (presentRectUsage) {
		usePresentRectPtr = *(BOOL**)(presentRectUsage + 2);
		presentRectWPtr = *(int**)(presentRectUsage + 7);
		presentRectHPtr = presentRectWPtr + 1;
	}
	
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

bool Graphics::checkAndHookBeginSceneAndPresent(bool transactionActive) {
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
		beginSceneHookStatic,
		"BeginScene")) error = true;
	
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
	graphics.mayRunBeginSceneHook = true;
	// When the application starts, the first barbarian that hooks it is Steam, who installs into Present.
	// Inside Present hook, before actual Present runs, Steam does a BeginScene, draws its overlay, and
	// calls EndScene. We hook the one BeginScene that runs in Steam's Present hook.
	// OBS also hooks Present after Steam and does capturing when it starts. OBS does capturing
	// before running what it thinks is the "real" Present, so anything Steam (or we) draw is invisible to OBS.
	// OBS has a setting to do capturing at the end of Present though, so it is still possible to record
	// Steam overlay and what we draw.
	
	// If Pangaea's mod is installed, its hook will be before Steam's.
	HRESULT result = graphics.orig_present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	graphics.mayRunBeginSceneHook = false;  // sometimes when unlocking the Windows machine while OBS is on and OBS dodging is on,
	// Steam does not run its BeginScene inside Present,
	// so we must unset our flag and wait for better days, until Steam does decide to run it.
	// If we don't unset our flag, the next BeginScene call, that is not inside a Present, will happen from the game itself,
	// and the render target is wrong at that moment. It is some smaller sized render target that is definitely not the game's
	// screen or the like. The alternative method of obtaining the "correct" render target, direct3DVTable.getRenderTarget(), leads to
	// unconditional freezing (I, however, spent 5 minutes investigating it, so there may be a way to actually use it),
	// so we're at the mercy of using whatever render target the game decides to give us at the moment of our shitty hook
	return result;
}

HRESULT __stdcall Graphics::beginSceneHookStatic(IDirect3DDevice9* device) {
	graphics.beginSceneHook(device);
	return graphics.orig_beginScene(device);
}

void Graphics::beginSceneHook(IDirect3DDevice9* device) {
	graphicsThreadId = GetCurrentThreadId();
	if (mayRunBeginSceneHook) {
		mayRunBeginSceneHook = false;
		
		static char obsPipeName[512] { '\0' };
		if (obsPipeName[0] == '\0') {
			snprintf(obsPipeName, sizeof(obsPipeName), "\\\\.\\pipe\\%s%lu", PIPE_NAME, GetCurrentProcessId());
		}
			
		HANDLE obsPipe = CreateFileA(obsPipeName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (obsPipe == NULL || obsPipe == INVALID_HANDLE_VALUE) {
			DWORD errorCode = GetLastError();
			// ERROR_FILE_NOT_FOUND (0x2): The system cannot find the file specified.
			// ERROR_PIPE_BUSY (0xe7): All pipe instances are busy.
			if (errorCode == ERROR_FILE_NOT_FOUND) {
				if (!obsStoppedCapturing) obsDisappeared = true;
			} else if (errorCode == ERROR_PIPE_BUSY) {
				if (obsStoppedCapturing) obsReappeared = true;
			}
		} else {
			CloseHandle(obsPipe);
		}
		
		if (obsDisappeared) {
			obsDisappeared = false;
			obsStoppedCapturing = true;
		}
		
		if (obsReappeared) {
			obsReappeared = false;
			obsStoppedCapturing = false;
		}
		
		bool drawingPostponedLocal = drawingPostponed();
		
		if (drawingPostponedLocal
				&& endSceneIsAwareOfDrawingPostponement == drawingPostponedLocal
				&& obsStoppedCapturingFromEndScenesPerspective == obsStoppedCapturing
				&& !shutdown
				&& !runningOwnBeginScene) {
			
			runningOwnBeginScene = true;
			device->BeginScene();
			runningOwnBeginScene = false;
			if (pauseMenuOpen) {
				if (uiTexture) {
					ui.onEndScene(device, uiDrawData.data(), uiTexture);
				}
			} else {
				needDrawWholeUiWithPoints = true;
				executeBoxesRenderingCommand(device);
				needDrawWholeUiWithPoints = false;
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
	// What if it closes that thread and opens a new one with the exact same ID in the same process?
	// So like we found it, but it's not the graphics thread anymore, hmm
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
	checkAndHookBeginSceneAndPresent(false);
}

void Graphics::onShutdown() {
	resetHook();
	ui.onDllDetachGraphics();
	if (endSceneAndPresentHooked) {
		const char* hooksToUndetour[] {
			"BeginScene",
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

void Graphics::prepareTextureBox(const TextureBoxParams& box, bool isFont) {
	if (lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_TEXTUREBOX && lastTextureIsFont == isFont) {
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
	if (!isFont) {
		++preparedTextureBoxesCount;
	} else {
		++preparedTextureBoxesCountWithFont;
	}
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_TEXTUREBOX;
	lastTextureIsFont = isFont;
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
	ensureWorldMatrixWorldCenterIsZero();
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
	ensureWorldMatrixWorldCenterIsZero();
	device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, 2 + (preparedBoxesCount - 1) * 6);
	vertexBufferPosition += 4 + (preparedBoxesCount - 1) * 6;
	preparedBoxesCount = 0;
}

void Graphics::drawAllTextureBoxes() {
	if (!preparedTextureBoxesCount && !preparedTextureBoxesCountWithFont) return;
	sendAllPreparedVertices();
	switchToRenderingTextureVertices();
	
	if (preparedTextureBoxesCount) {
		advanceRenderState(RENDER_STATE_DRAWING_TEXTURES);
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, textureVertexBufferPosition, 2 + (preparedTextureBoxesCount - 1) * 6);
		textureVertexBufferPosition += 4 + (preparedTextureBoxesCount - 1) * 6;
		preparedTextureBoxesCount = 0;
	}
	
	if (preparedTextureBoxesCountWithFont) {
		advanceRenderState(RENDER_STATE_DRAWING_TEXT);
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, textureVertexBufferPosition, 2 + (preparedTextureBoxesCountWithFont - 1) * 6);
		textureVertexBufferPosition += 4 + (preparedTextureBoxesCountWithFont - 1) * 6;
		preparedTextureBoxesCountWithFont = 0;
	}
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
	ensureWorldMatrixWorldCenterIsZero();
	
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
	switchToRenderingNonTextureVertices();
	advanceRenderState(RENDER_STATE_DRAWING_POINTS);

	for (unsigned int i = numberOfPointsPrepared; i != 0; --i) {
		device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, 8);
		vertexBufferPosition += 10;
		device->DrawPrimitive(D3DPT_LINELIST, vertexBufferPosition, 2);
		vertexBufferPosition += 4;
	}

	numberOfPointsPrepared = 0;

}

void Graphics::drawAllSmallPoints() {
	if (!numberOfSmallPointsPrepared) return;
	sendAllPreparedVertices();
	switchToRenderingNonTextureVertices();
	advanceRenderState(RENDER_STATE_DRAWING_POINTS);
	
	int verticesNum = 10 * numberOfSmallPointsPrepared + 2 * (numberOfSmallPointsPrepared - 1);
	device->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertexBufferPosition, verticesNum - 2);
	vertexBufferPosition += verticesNum;
	
	numberOfSmallPointsPrepared = 0;

}

void Graphics::drawAllLines() {
	if (!linesPrepared) return;
	sendAllPreparedVertices();
	switchToRenderingNonTextureVertices();
	advanceRenderState(RENDER_STATE_DRAWING_OUTLINES);
	ensureWorldMatrixWorldCenterIsZero();
	
	device->DrawPrimitive(D3DPT_LINELIST, vertexBufferPosition, linesPrepared);
	vertexBufferPosition += 2 * linesPrepared;
	
	linesPrepared = 0;

}

void Graphics::drawAllPrepared() {
	if (!loggedDrawingOperationsOnce) {
		logwrap(fprintf(logfile, "Arrayboxes count: %u\n"
			"boxes count (including those in arrayboxes): %u\n"
			"outlines count: %u\n"
			"small points count: %u\n"
			"points count: %u\n"
			"texture boxes count: %u\n"
			"texture boxes count with font: %u",
			preparedArrayboxes.size(),
			preparedBoxesCount,
			preparedOutlines.size(),
			numberOfSmallPointsPrepared,
			numberOfPointsPrepared,
			preparedTextureBoxesCount,
			preparedTextureBoxesCountWithFont));
	}
	switch (1) {
	case 1:
		if (!drawAllArrayboxes()) break;
		drawAllBoxes();
		drawAllCircles();
		if (!drawAllOutlines()) break;
		drawAllCircleOutlines();
		drawAllLines();
		drawAllTextureBoxes();
		drawAllFramebarDrawData();
		drawAllSmallPoints();
		drawAllPoints();
	}
	
	int maxBufferPositionBytes = vertexBufferPosition * sizeof Vertex;
	int otherBufferPositionBytes = textureVertexBufferPosition * sizeof TextureVertex;
	if (otherBufferPositionBytes > maxBufferPositionBytes) maxBufferPositionBytes = otherBufferPositionBytes;
	
	
	if (maxBufferPositionBytes != 0) {
		int bufferLengthBytes;
		if (preparingTextureVertexBuffer) {
			bufferLengthBytes = textureVertexBufferLength * sizeof TextureVertex;
		} else {
			bufferLengthBytes = vertexBufferLength * sizeof Vertex;
		}
		if (maxBufferPositionBytes != bufferLengthBytes) {
			memmove(vertexArena.data(), vertexArena.data() + maxBufferPositionBytes, bufferLengthBytes - maxBufferPositionBytes);
		}
		
		int remainder;
		int roundDown;
		if (preparingTextureVertexBuffer) {
			remainder = maxBufferPositionBytes % sizeof TextureVertex;
			roundDown = maxBufferPositionBytes / sizeof TextureVertex;
			if (remainder) ++roundDown;
			if (roundDown > (int)textureVertexBufferLength) roundDown = textureVertexBufferLength;
			textureVertexBufferLength -= roundDown;
			textureVertexIt = (TextureVertex*)vertexArena.data() + textureVertexBufferLength;
			textureVertexBufferRemainingSize = textureVertexBufferSize - textureVertexBufferLength;
		} else {
			remainder = maxBufferPositionBytes % sizeof Vertex;
			roundDown = maxBufferPositionBytes / sizeof Vertex;
			if (remainder) ++roundDown;
			if (roundDown > (int)vertexBufferLength) roundDown = vertexBufferLength;
			vertexBufferLength -= roundDown;
			vertexIt = (Vertex*)vertexArena.data() + vertexBufferLength;
			vertexBufferRemainingSize = vertexBufferSize - vertexBufferLength;
		}
		textureVertexBufferPosition = 0;
		vertexBufferPosition = 0;
		
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
		device->SetRenderState(D3DRS_STENCILREF, 0);
		device->SetRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		renderStateValues[RenderStateType(D3DRS_ALPHABLENDENABLE)] = RenderStateValue(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		renderStateValues[RenderStateType(D3DRS_SRCBLEND)] = RenderStateValue(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		renderStateValues[RenderStateType(D3DRS_DESTBLEND)] = RenderStateValue(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_LIGHTING, FALSE);
		
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		renderStateValues[RenderStateType(D3DRS_STENCILENABLE)] = RenderStateValue(D3DRS_STENCILENABLE, FALSE);
		device->SetPixelShader(nullptr);
		renderStateValues[RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NONE);
		preparedPixelShaderOnThisFrame = false;
		device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		renderStateValues[RenderStateType(D3DRS_SRCBLENDALPHA)] = RenderStateValue(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
		renderStateValues[RenderStateType(D3DRS_DESTBLENDALPHA)] = RenderStateValue(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
		
		device->SetTexture(0, nullptr);
		renderStateValues[RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, NONE);
		device->SetVertexShader(nullptr);
		device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
		renderStateValues[RenderStateType(VERTEX)] = RenderStateValue(VERTEX, NONTEXTURE);
		device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
		worldMatrixHasShiftedWorldCenter = false;
		
	}
	
	if (!onlyDrawPoints && !dontShowBoxes && !onlyDrawInputHistory) {
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
		for (const DrawCircleCallParams& params : drawDataUse.circles) {
			prepareCircle(params);
		}
		if (screenshotStage != SCREENSHOT_STAGE_BASE_COLOR) {
			for (DrawOutlineCallParams& params : outlines) {
				prepareOutline(params);
			}
			for (const DrawCircleCallParams& params : drawDataUse.circles) {
				prepareCircleOutline(params);
			}
			for (const DrawLineCallParams& params : drawDataUse.lines) {
				prepareLine(params);
			}
		}
	}
	
	// a special command that draws only the points may also draw inputs
	// this is caused by points needing to be drawn on top of the tension bars, while boxes are under the tension bar
	// well, inputs also need to be on top of the tension bar so we lumped them into one FRenderCommand
	if ((onlyDrawInputHistory || onlyDrawPoints || drawingPostponed())
			&& screenshotStage == SCREENSHOT_STAGE_NONE
			&& (drawDataUse.inputsSize[0] || drawDataUse.inputsSize[1])
			&& (onlyDrawInputHistory || !inputHistoryIsSplitOut)) {
		prepareDrawInputs();
	}
	
	if (
			!onlyDrawInputHistory
			&& (
				needDrawFramebarWithPoints
				|| needDrawWholeUiWithPoints
			) && screenshotStage == SCREENSHOT_STAGE_NONE
	) {
		prepareFramebarDrawData();
	}
	
	if (!onlyDrawInputHistory
			&& screenshotStage != SCREENSHOT_STAGE_BASE_COLOR
			&& (!noNeedToDrawPoints || drawDataUse.needTakeScreenshot)
			&& !dontShowBoxes) {
		for (const DrawPointCallParams& params : drawDataUse.points) {
			if (params.isProjectile) {
				prepareSmallPoint(params);
			}
		}
		for (const DrawPointCallParams& params : drawDataUse.points) {
			if (!params.isProjectile) {
				preparePoint(params);
			}
		}
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
	int anglePrep = -params.params.angle % 360000;
	if (anglePrep < 0) anglePrep = anglePrep + 360000;
	if (anglePrep) {
		anglePrep /= 100;
		cos = getCos(anglePrep);
		sin = getSin(anglePrep);
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
	stopPreparingTextureVertexBuffer();
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
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_POINT;
}

void Graphics::prepareSmallPoint(const DrawPointCallParams& params) {
	stopPreparingTextureVertexBuffer();
	D3DXVECTOR3 p{ (float)params.posX, (float)params.posY, 0.F };
	logOnce(fprintf(logfile, "drawPoint called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));
	
	D3DXVECTOR3 sp;
	if (!worldToScreen(p, &sp)) return;
	
	const D3DCOLOR fillColor = params.fillColor;
	
	const Vertex firstVertex = Vertex{ sp.x - 1, sp.y - 1, 0.F, fillColor };
	
	bool needPadding = lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_SMALL_POINT;
	if (drawIfOutOfSpace(10 + (needPadding ? 2 : 0))) {
		needPadding = false;
	}
	consumeVertexBufferSpace(10 + (needPadding ? 2 : 0));
	if (needPadding) {
		*vertexIt = *(vertexIt - 1);
		++vertexIt;
		*vertexIt = firstVertex;
		++vertexIt;
	}
	
	*vertexIt = firstVertex;
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 1, sp.y + 3, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 3, sp.y - 1, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 3, sp.y + 3, 0.F, fillColor };
	++vertexIt;

	// PADDING
	*vertexIt = Vertex{ sp.x + 3, sp.y + 3, 0.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x, sp.y, 0.F, fillColor };
	++vertexIt;

	const D3DCOLOR outlineColor = params.outlineColor;

	*vertexIt = Vertex{ sp.x, sp.y, 0.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x, sp.y + 2, 0.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 2, sp.y, 0.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 2, sp.y + 2, 0.F, outlineColor };
	++vertexIt;
	++numberOfSmallPointsPrepared;
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_SMALL_POINT;
}

IDirect3DSurface9* Graphics::getOffscreenSurface(D3DSURFACE_DESC* renderTargetDescPtr) {
	D3DSURFACE_DESC renderTargetDesc;
	if (!renderTargetDescPtr) {
		CComPtr<IDirect3DSurface9> renderTarget;// = direct3DVTable.getRenderTarget();  // this will AddRef
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
	CComPtr<IDirect3DSurface9> gameRenderTarget = direct3DVTable.getRenderTarget();  // this will AddRef
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(gameRenderTarget->GetDesc(&renderTargetDesc))) {
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
	whateverOldRenderTarget = oldRenderTarget;
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
	// this is actually correct, we want the render target that we just drew boxes into
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

	CComPtr<IDirect3DSurface9> gameRenderTarget = direct3DVTable.getRenderTarget();  // this will AddRef
	std::vector<unsigned char> gameImage;
	if (!getFramebufferData(device, gameImage, gameRenderTarget, &renderTargetDesc)) return;
	
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
	CComPtr<IDirect3DSurface9> renderTarget;// = direct3DVTable.getRenderTarget();  // this will AddRef
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
		renderTargetComPtr;// = direct3DVTable.getRenderTarget();  // this will AddRef
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
		logwrap(fprintf(logfile, "GetRenderTargetData failed. renderTarget is: %p. offscreenSurface is %p\n", renderTarget, offscreenSurface));
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
	
	setWorld3DMatrix();
	worldMatrixHasShiftedWorldCenter = false;
	
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
	
	float m = PI / 32768.F;
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
    worldMatrixHasShiftedWorldCenter = false;
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
	if (!canDrawOnThisFrame()) return;  // let's not trigger the hook attempt more than once per frame
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
	preparedPixelShaderOnThisFrame = false;
	renderStateValues[RenderStateType(PIXEL_SHADER)] = RenderStateValue(PIXEL_SHADER, NO_PIXEL_SHADER);
	device->SetTexture(0, nullptr);
	renderStateValues[RenderStateType(TEXTURE)] = RenderStateValue(TEXTURE, NONE);
	
	renderStateValues[RenderStateType(TRANSFORM_MATRICES)] = RenderStateValue(TRANSFORM_MATRICES, NONE);
	currentTransformSet = CURRENT_TRANSFORM_DEFAULT;
	worldMatrixHasShiftedWorldCenter = false;
	
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
	device->SetRenderTarget(0, whateverOldRenderTarget);
	whateverOldRenderTarget = nullptr;
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
	vertexArena.resize(vertexBufferSize * sizeof Vertex);

	return true;
}

void Graphics::resetVertexBuffer() {
	vertexBufferRemainingSize = vertexBufferSize;
	vertexBufferLength = 0;
	vertexIt = (Vertex*)vertexArena.data();
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
	lines.clear();
	circles.clear();
	throwBoxes.clear();
	needTakeScreenshot = false;
}

void DrawData::clearInputs() {
	for (int i = 0; i < 2; ++i) {
		inputs[i].clear();
		inputsSize[i] = 0;
	}
	inputsContainsDurations = false;
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
	destination->circles = circles;
	destination->lines = lines;
	destination->throwBoxes = throwBoxes;
	for (int i = 0; i < 2; ++i) {
		destination->inputs[i] = inputs[i];
		destination->inputsSize[i] = inputsSize[i];
	}
	destination->inputsContainsDurations = inputsContainsDurations;
	destination->needTakeScreenshot = needTakeScreenshot;
	destination->gameModeFast = gameModeFast;
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
	CComPtr<IDirect3DSurface9> renderTarget;// = direct3DVTable.getRenderTarget();  // this will AddRef
	// This render target is just wrong sometimes. It has wrong size and everything
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
	
	if (failedToCompilePixelShader || !pixelShaderCode.empty()) return;
	failedToCompilePixelShader = true; return;
	
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
	
	pixelShaderCode.resize(shaderSize);
	memcpy(pixelShaderCode.data(), codeData, pixelShaderCode.size());
}

void Graphics::getShaderCompilationError(const std::string** result) {
	*result = nullptr;
	if (shaderCompilationError.empty()) return;
	*result = &shaderCompilationError;
}

IDirect3DPixelShader9* Graphics::getPixelShader(IDirect3DDevice9* device) {
	if (failedToCreatePixelShader) return nullptr;
	if (pixelShader) return pixelShader;
	if (pixelShaderCode.empty()) {
		failedToCreatePixelShader = true;
		return nullptr;
	}
	if (FAILED(device->CreatePixelShader((const DWORD*)pixelShaderCode.data(), &pixelShader))) {
		failedToCreatePixelShader = true;
		logwrap(fprintf(logfile, "CreatePixelShader failed\n"));
		return nullptr;
	}
	return pixelShader;
}

void Graphics::preparePixelShader(IDirect3DDevice9* device) {
	
	IDirect3DTexture9* tex = getOutlinesRTSamplingTexture(device);
	if (!tex) return;
	
	if (!preparedPixelShaderOnThisFrame) {
		CComPtr<IDirect3DSurface9> renderTarget;// = direct3DVTable.getRenderTarget();
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
		
		D3DSURFACE_DESC renderTargetDesc;
		SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
		if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
			logwrap(fputs("GetDesc failed\n", logfile));
			return;
		}
		screenSizeConstant[0] = (float)renderTargetDesc.Width;
		screenSizeConstant[1] = (float)renderTargetDesc.Height;
		
		compilePixelShader();
	}
	IDirect3DPixelShader9* shader = getPixelShader(device);
	if (!shader) return;
	
	device->SetPixelShader(shader);
	device->SetTexture(0, tex);
	device->SetPixelShaderConstantF(0, screenSizeConstant, 1);
	preparedPixelShaderOnThisFrame = true;
}

void Graphics::heartbeat(IDirect3DDevice9* device) {
	initViewport(device);
	fillInScreenSize(device);
	receiveDanger();
	afterDraw();
	checkAndHookBeginSceneAndPresent(false);
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
		case RenderStateValue(PIXEL_SHADER, CUSTOM_PIXEL_SHADER):
			if (graphics.usePixelShader) {
				graphics.preparePixelShader(graphics.device);
			} else {
				graphics.device->SetPixelShader(nullptr);
			}
			break;
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
		case RenderStateValue(TEXTURE, FONT):
		case RenderStateValue(TEXTURE, ICONS):
			graphics.device->SetTexture(0, newValue == RenderStateValue(TEXTURE, ICONS) ? graphics.iconsTexture : graphics.staticFontTexture);
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
	return !(
		imInDangerReceived
		&& settings.dodgeObsRecording
		&& (
			!endSceneAndPresentHooked
			|| obsStoppedCapturing != obsStoppedCapturingFromEndScenesPerspective
		));
}

bool Graphics::drawingPostponed() const {
	return settings.dodgeObsRecording && endSceneAndPresentHooked && !obsStoppedCapturing;
}

// Draw boxes, without UI, and take a screenshot if needed
// Runs on the graphics thread
void Graphics::executeBoxesRenderingCommand(IDirect3DDevice9* device) {
	drawAllInit(device);
	
	bool doYourThing = !dontShowBoxes && !onlyDrawInputHistory
		|| (graphics.drawDataUse.inputsSize[0] || graphics.drawDataUse.inputsSize[1])
		&& (
			onlyDrawInputHistory
			
			// drawing points may also draw inputs. So no need to draw points = no need to draw inputs
			|| !noNeedToDrawPoints
		)
		&& !inputHistoryIsSplitOut
		|| needDrawWholeUiWithPoints && !uiDrawData.empty()
		|| needDrawFramebarWithPoints && !uiFramebarDrawData.empty();
		
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

void Graphics::startPreparingTextureVertexBuffer() {
	if (preparingTextureVertexBuffer) return;
	textureVertexBufferPosition = 0;
	textureVertexBufferLength = calculateStartingTextureVertexBufferLength();
	textureVertexBufferRemainingSize = textureVertexBufferSize - textureVertexBufferLength;
	textureVertexIt = (TextureVertex*)vertexArena.data() + textureVertexBufferLength;
	preparingTextureVertexBuffer = true;
}

void Graphics::stopPreparingTextureVertexBuffer() {
	if (!preparingTextureVertexBuffer) return;
	vertexBufferPosition = 0;
	vertexBufferLength = calculateStartingVertexBufferLength();
	vertexBufferRemainingSize = vertexBufferSize - vertexBufferLength;
	vertexIt = (Vertex*)vertexArena.data() + vertexBufferLength;
	preparingTextureVertexBuffer = false;
}

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

void Graphics::switchToRenderingNonTextureVertices() {
	if (!renderingTextureVertices) return;
	int positionBytes = textureVertexBufferPosition * sizeof TextureVertex;
	int remainder = positionBytes % sizeof Vertex;
	if (remainder) {
		positionBytes += sizeof Vertex - remainder;
	}
	vertexBufferPosition = positionBytes / sizeof Vertex;
	renderingTextureVertices = false;
}

void Graphics::prepareDrawInputs() {
	startPreparingTextureVertexBuffer();
	
	TextureBoxParams box;
	
	float coef = viewportW / 1280.F;
	float invCoef = 1280.F / viewportW;
	float extraH_in1280space = (viewportH * invCoef - 720.0F) * 0.5F;  // if the monitor is wider, this is negative
	float extraH = extraH_in1280space * viewportH / 1280.F;  // if the monitor is wider, this is negative
	const float iconSize = 28.F;
	const float columnWidth = 30.F;
	const float rowHeight = 32.F;
	const float iconSizeMult = iconSize * coef;
	const float startY = 140.F * coef + extraH;
	const float rowHeightMult = rowHeight * coef;
	const float columnWidthMult = columnWidth * coef;
	
	char strbuf[10] { '\0' };
	const float textScale = 0.6538F;
	const float coef3 = coef * 3.F;
	
	float textWs[2] { 0.F, 0.F };
	
	for (int i = 0; i == 0 || i == 1 && drawDataUse.gameModeFast == GAME_MODE_FAST_NORMAL; ++i) {
		size_t inputsSize = drawDataUse.inputsSize[i];
		const InputsDrawingCommandRow* rows = drawDataUse.inputs[i].data();
		
		box.yStart = startY;
		box.yEnd = box.yStart + iconSizeMult;
		
		int rowIndMin = (int)inputsSize - 18;
		if (rowIndMin < 0) rowIndMin = 0;
		
		float textWMax = 0.F;
		if (drawDataUse.inputsContainsDurations) {
			int maxDuration = 0;
			for (int rowInd = (int)inputsSize - 1; rowInd >= rowIndMin; --rowInd) {
				const InputsDrawingCommandRow* row = rows + rowInd;
				if (row->duration > maxDuration) maxDuration = row->duration;
			}
			
			int digitCount = 0;
			do {
				++digitCount;
				maxDuration /= 10;
			} while (maxDuration);
			
			textWMax = (float)digitCount * 19.F * coef * textScale + (float)(digitCount - 1) * coef3 + coef + coef;
			textWs[i] = textWMax;
		}
		
		for (int rowInd = (int)inputsSize - 1; rowInd >= rowIndMin; --rowInd) {
			const InputsDrawingCommandRow* row = rows + rowInd;
			
			float x;
			if (i == 0 && drawDataUse.gameModeFast == GAME_MODE_FAST_NORMAL) {
				x = 20.F;
				if (drawDataUse.inputsContainsDurations) {
					x += coef3 + textWMax;
				}
			} else {
				x = 1260.F - 30.F * row->count;
				if (drawDataUse.inputsContainsDurations) {
					x -= coef3 + textWMax;
				}
			}
			box.xStart = x * coef;
			box.xEnd = box.xStart + iconSizeMult;
			
			bool inReverse = drawDataUse.gameModeFast != GAME_MODE_FAST_NORMAL;
			
			for (int column = 0; column < row->count; ++column) {
				const InputsDrawingCommand* cmd = inReverse ? row->cmds + row->count - 1 - column : row->cmds + column;
				
				box.color = cmd->dark ? 0xffa0a0a0 : 0xffffffff;
				
				const InputsIcon* icon = inputsIcon + cmd->icon;
				box.uStart = icon->uStart;
				box.vStart = icon->vStart;
				box.uEnd = icon->uEnd;
				box.vEnd = icon->vEnd;
				prepareTextureBox(box, false);
				
				box.xStart += columnWidthMult;
				box.xEnd += columnWidthMult;
			}
			
			box.yStart += rowHeightMult;
			box.yEnd += rowHeightMult;
		}
	}
	
	if (drawDataUse.inputsContainsDurations) {
		
		for (int i = 0; i == 0 || i == 1 && drawDataUse.gameModeFast == GAME_MODE_FAST_NORMAL; ++i) {
			size_t inputsSize = drawDataUse.inputsSize[i];
			const InputsDrawingCommandRow* rows = drawDataUse.inputs[i].data();
			
			float y = startY + coef + (rowHeightMult - 26.F * textScale * coef) * 0.5F;
			
			int rowIndMin = (int)inputsSize - 18;
			if (rowIndMin < 0) rowIndMin = 0;
			
			for (int rowInd = (int)inputsSize - 1; rowInd >= rowIndMin; --rowInd) {
				const InputsDrawingCommandRow* row = rows + rowInd;
				
				float x;
				if (i == 0 && drawDataUse.gameModeFast == GAME_MODE_FAST_NORMAL) {
					x = 20.F * coef;
				} else {
					x = 1260.F * coef - textWs[i] + coef;
				}
				
				sprintf_s(strbuf, "%d", row->duration);
				printTextWithOutline(x, y, strbuf, coef, textScale);
				
				y += rowHeightMult;
			}
				
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

int Graphics::calculateStartingVertexBufferLength() {
	size_t lengthBytes = textureVertexBufferLength * sizeof TextureVertex;
	size_t remainder = lengthBytes % sizeof Vertex;
	if (remainder) lengthBytes += sizeof Vertex - remainder;
	size_t result = lengthBytes == 0 ? 0 : lengthBytes / sizeof Vertex;
	
	if (result > vertexBufferSize) {
		return vertexBufferSize;
	}
	return result;
}

void Graphics::drawAllFramebarDrawData() {
	if (!framebarDrawDataPrepared) return;
	void* ddUse;
	if (needDrawWholeUiWithPoints) {
		ddUse = uiDrawData.data();
	} else {
		ddUse = uiFramebarDrawData.data();
	}
	ui.onEndScene(device, ddUse, uiTexture);
	framebarDrawDataPrepared = false;
}

void Graphics::prepareLine(const DrawLineCallParams& params) {
	stopPreparingTextureVertexBuffer();
	drawIfOutOfSpace(2);
	consumeVertexBufferSpace(2);
	*vertexIt = Vertex{ (float)params.posX1, (float)params.posY1, 0.F, params.color };
	++vertexIt;
	*vertexIt = Vertex{ (float)params.posX2, (float)params.posY2, 0.F, params.color };
	++vertexIt;
	++linesPrepared;
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
}

void Graphics::prepareCircle(const DrawCircleCallParams& params) {
	if ((params.fillColor & 0xff000000) == 0) return;
	stopPreparingTextureVertexBuffer();
	
	int nextInd = setupCircle(params.radius, params.fillColor, params.outlineColor);
	
	Vertex firstVertex{ 0.F, 0.F, 0.F, params.fillColor };
	const CircleCacheElement& elem = circleCache[nextInd - 1];
	const Vertex* lastSourceVtx = elem.vertices.data();
	int remainingVertices = elem.vertices.size();
	while (remainingVertices) {
		if (vertexBufferRemainingSize < 3) {
			drawIfOutOfSpace(3);
		}
		if (vertexBufferRemainingSize >= (unsigned int)remainingVertices + 1) {
			consumeVertexBufferSpace(remainingVertices + 1);
			*vertexIt = firstVertex;
			++vertexIt;
			memcpy(vertexIt, lastSourceVtx, remainingVertices * sizeof Vertex);
			vertexIt += remainingVertices;
			lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
			
			preparedCircles.emplace_back();
			PreparedCircle& newCircle = preparedCircles.back();
			newCircle.x = params.posX;
			newCircle.y = params.posY;
			newCircle.verticesCount = remainingVertices + 1;
			
			return;
		}
		unsigned int oldSize = vertexBufferRemainingSize;
		consumeVertexBufferSpace(oldSize);
		*vertexIt = firstVertex;
		++vertexIt;
		memcpy(vertexIt, lastSourceVtx, (oldSize - 1) * sizeof Vertex);
		vertexIt += oldSize - 1;
		lastSourceVtx += oldSize - 2;
		remainingVertices -= oldSize - 2;
		
		preparedCircles.emplace_back();
		PreparedCircle& newCircle = preparedCircles.back();
		newCircle.x = params.posX;
		newCircle.y = params.posY;
		newCircle.verticesCount = oldSize;
		
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	}
}

void Graphics::prepareCircleOutline(const DrawCircleCallParams& params) {
	if ((params.outlineColor & 0xff000000) == 0) return;
	
	int nextInd = setupCircle(params.radius, params.fillColor, params.outlineColor);
	
	const CircleCacheElement& elem = circleCache[nextInd - 1];
	const std::vector<Vertex>* vecPtr;
	if ((params.fillColor & 0xff000000) != 0 && params.fillColor == params.outlineColor) {
		vecPtr = &elem.vertices;
	} else {
		vecPtr = &elem.outlineVertices;
	}
	const Vertex* lastSourceVtx = vecPtr->data();
	int remainingVertices = vecPtr->size();
	while (remainingVertices) {
		if (vertexBufferRemainingSize < 2) {
			drawIfOutOfSpace(2);
		}
		if (vertexBufferRemainingSize >= (unsigned int)remainingVertices) {
			consumeVertexBufferSpace(remainingVertices);
			memcpy(vertexIt, lastSourceVtx, remainingVertices * sizeof Vertex);
			vertexIt += remainingVertices;
			lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
			
			preparedCircleOutlines.emplace_back();
			PreparedCircle& newCircle = preparedCircleOutlines.back();
			newCircle.x = params.posX;
			newCircle.y = params.posY;
			newCircle.verticesCount = remainingVertices;
			
			return;
		}
		unsigned int oldSize = vertexBufferRemainingSize;
		consumeVertexBufferSpace(oldSize);
		memcpy(vertexIt, lastSourceVtx, oldSize);
		vertexIt += oldSize;
		lastSourceVtx += oldSize - 1;
		remainingVertices -= oldSize - 1;
		
		preparedCircleOutlines.emplace_back();
		PreparedCircle& newCircle = preparedCircleOutlines.back();
		newCircle.x = params.posX;
		newCircle.y = params.posY;
		newCircle.verticesCount = oldSize;
		
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	}
}

void Graphics::drawAllCircles() {
	if (preparedCircles.empty()) return;
	if (screenshotStage == SCREENSHOT_STAGE_BASE_COLOR) {
		stencil.initialize(device);
	}
	sendAllPreparedVertices();
	advanceRenderState(RENDER_STATE_DRAWING_BOXES);
	for (const PreparedCircle& circle : preparedCircles) {
		setWorld3DMatrix(circle.x, circle.y);
		worldMatrixHasShiftedWorldCenter = true;
		device->DrawPrimitive(D3DPT_TRIANGLEFAN, vertexBufferPosition, circle.verticesCount - 2);
		vertexBufferPosition += circle.verticesCount;
	}
	preparedCircles.clear();
}

void Graphics::drawAllCircleOutlines() {
	if (preparedCircleOutlines.empty()) return;
	sendAllPreparedVertices();
	advanceRenderState(RENDER_STATE_DRAWING_OUTLINES);
	for (const PreparedCircle& circle : preparedCircleOutlines) {
		setWorld3DMatrix(circle.x, circle.y);
		worldMatrixHasShiftedWorldCenter = true;
		device->DrawPrimitive(D3DPT_LINESTRIP, vertexBufferPosition, circle.verticesCount - 1);
		vertexBufferPosition += circle.verticesCount;
	}
	preparedCircleOutlines.clear();
}

int Graphics::getCos(int degrees) {
	return getSin(degrees + 900);
}

// This function produces wrong results but it is a copy of FUN_00d712b0 from the game itself
int Graphics::getSin(int degrees) {
	int i = degrees % 3600;
	if (i < 0) i += 3600;
	if (i < 900) return graphics.sinTable[i];
	if (i < 1800) return graphics.sinTable[899 - (i - 900)];
	if (i < 2700) return -graphics.sinTable[i - 1800];
	return -graphics.sinTable[899 - (i - 2700)];
}

int Graphics::setupCircle(int radius, D3DCOLOR fillColor, D3DCOLOR outlineColor) {
	if (circleCacheHashmap.empty()) {
		circleCacheHashmap.resize(100);
	}
	int cacheIndex = (radius / 1000) % 100;
	int lastInd = 0;
	int nextInd = circleCacheHashmap[cacheIndex];
	while (nextInd) {
		lastInd = nextInd;
		const CircleCacheElement& seekElem = circleCache[nextInd - 1];
		if (seekElem.radius == radius
				&& seekElem.fillColor == fillColor
				&& seekElem.outlineColor == outlineColor) {
			break;
		}
		nextInd = seekElem.next;
	}
	if (nextInd == 0 || circleCache[nextInd - 1].vertices.empty()) {
		// approximate conversion of ArcSys size to the on-screen size in pixels at 1280x720 resolution
		unsigned int totalCircumference = (unsigned int)radius * 229U / 266000U;
		if (totalCircumference > 68356) {
			totalCircumference = (unsigned int)(2.F * PI * (float)totalCircumference);
		} else {
			totalCircumference = 2U * 31416U * totalCircumference / 10000U;
		}
		int segments;
		if (totalCircumference > 1000) {
			segments = totalCircumference / 20;  // 20px long line segments in the circle
		} else if (totalCircumference > 500) {
			segments = totalCircumference / 10;
		} else {
			segments = totalCircumference / 4;
		}
		if (segments < 4) segments = 4;
		CircleCacheElement* newElemPtr;
		if (nextInd == 0) {
			if (lastInd == 0) {
				circleCacheHashmap[cacheIndex] = circleCache.size() + 1;
			} else {
				circleCache[lastInd - 1].next = circleCache.size() + 1;
			}
			nextInd = circleCache.size() + 1;
			circleCache.emplace_back();
			newElemPtr = &circleCache.back();
		} else {
			newElemPtr = &circleCache[nextInd - 1];
		}
		CircleCacheElement& newElem = *newElemPtr;
		newElem.radius = radius;
		newElem.fillColor = fillColor;
		newElem.outlineColor = outlineColor;
		if (fillColor != outlineColor && (fillColor & 0xff000000) != 0 && (outlineColor & 0xff000000) != 0) {
			newElem.outlineVertices.resize(segments + 1);
			newElem.vertices.resize(segments + 1);
			Vertex* currentOutlineVert = newElem.outlineVertices.data();
			Vertex* currentVert = newElem.vertices.data();
			Vertex vtx;
			for (int i = 0; i < segments; ++i) {
				int angle = 3600 * i / segments;
				vtx = Vertex{
					(float)(getCos(angle) * radius / 1000),
					(float)(getSin(angle) * radius / 1000),
					0.F,
					fillColor
				};
				*currentVert = vtx;
				++currentVert;
				vtx.color = outlineColor;
				*currentOutlineVert = vtx;
				++currentOutlineVert;
			}
			vtx = Vertex{
				(float)radius,
				0.F,
				0.F,
				fillColor
			};
			*currentVert = vtx;
			vtx.color = outlineColor;
			*currentOutlineVert = vtx;
		} else if ((fillColor & 0xff000000) != 0 || (outlineColor & 0xff000000) != 0) {
			std::vector<Vertex>* vec;
			D3DCOLOR color;
			if ((fillColor & 0xff000000) != 0) {
				vec = &newElem.vertices;
				color = fillColor;
			} else {
				vec = &newElem.outlineVertices;
				color = outlineColor;
			}
			vec->resize(segments + 1);
			Vertex* currentVert = vec->data();
			for (int i = 0; i < segments; ++i) {
				int angle = 3600 * i / segments;
				*currentVert = Vertex{
					(float)(getCos(angle) * radius / 1000),
					(float)(getSin(angle) * radius / 1000),
					0.F,
					color
				};
				++currentVert;
			}
			*currentVert = Vertex{
				(float)radius,
				0.F,
				0.F,
				color
			};
		}
	}
	return nextInd;
}

void Graphics::ensureWorldMatrixWorldCenterIsZero() {
	if (worldMatrixHasShiftedWorldCenter) {
		worldMatrixHasShiftedWorldCenter = false;
		setWorld3DMatrix();
	}
}

void Graphics::setWorld3DMatrix(int worldCenterShiftX, int worldCenterShiftY) {
	if (currentTransformSet == CURRENT_TRANSFORM_3D_PROJECTION
			&& currentWorld3DMatrixWorldShiftX == worldCenterShiftX
			&& currentWorld3DMatrixWorldShiftY == worldCenterShiftY) return;
	
	currentWorld3DMatrixWorldShiftX = worldCenterShiftX;
	currentWorld3DMatrixWorldShiftY = worldCenterShiftY;
	
	float m = camera.valuesUse.coordCoefficient / 1000.F;
	D3DXMATRIX world {
		m,                            0.F, 0.F,                          0.F,
		0.F,                          0.F, m,                            0.F,
		0.F,                          m,   0.F,                          0.F,
		(float)worldCenterShiftX * m, 0.F, (float)worldCenterShiftY * m, 1.F
	};
	device->SetTransform(D3DTS_WORLD, &world);
}

void Graphics::calcTextSize(const char* txt, float coef, float textScale, bool outline, float* sizeX, float* sizeY) {
	float totalWidth = 0.F;
	float prevExtraSpaceRight = 0.F;
	float coef3 = coef * 3.F;
	float coefPremult = coef * textScale;
	bool isFirst = true;
	bool prevCharSkipped = false;
	for (const char* c = txt; *c != '\0'; ++c) {
		char cVal = *c;
		if (!prevCharSkipped) {
			if (!isFirst) {
				totalWidth += coef3 + prevExtraSpaceRight;
			}
			isFirst = false;
		}
		const CharInfo* info = nullptr;
		if (cVal == '(') {
			info = &staticFontOpenParenthesis;
		} else if (cVal == ')') {
			info = &staticFontCloseParenthesis;
		} else if (cVal >= '0' && cVal <= '9') {
			info = staticFontDigit + (cVal - '0');
		}
		if (info) {
			prevCharSkipped = false;
			prevExtraSpaceRight = (float)info->extraSpaceRight * charInfoOffsetScale * coefPremult;
			totalWidth += (float)info->sizeX * coefPremult;
		} else {
			prevCharSkipped = true;
		}
	}
	if (outline) {
		if (totalWidth) totalWidth += coef + coef;
	}
	*sizeX = totalWidth;
	*sizeY = (outline ? 28.F : 26.F) * coefPremult;
}

void Graphics::printTextWithOutline(float x, float y, const char* txt, float coef, float textScale) {
	DWORD black = D3DCOLOR_ARGB(255, 0, 0, 0);
	printText(x - coef, y - coef, txt, coef, textScale, black);
	printText(x - coef, y + coef, txt, coef, textScale, black);
	printText(x + coef, y - coef, txt, coef, textScale, black);
	printText(x + coef, y + coef, txt, coef, textScale, black);
	printText(x, y, txt, coef, textScale, (DWORD)-1);
}

void Graphics::printText(float x, float y, const char* txt, float coef, float textScale, DWORD color) {
	bool isFirst = true;
	float prevExtraSpaceRight = 0.F;
	bool prevCharSkipped = false;
	float coef3 = coef * 3.F;
	float coefPremult = coef * textScale;
	for (const char* c = txt; *c != '\0'; ++c) {
		char cVal = *c;
		if (!prevCharSkipped) {
			if (!isFirst) {
				x += coef3 + prevExtraSpaceRight;
			}
			isFirst = false;
		}
		const CharInfo* info = nullptr;
		if (cVal == '(') {
			info = &staticFontOpenParenthesis;
		} else if (cVal == ')') {
			info = &staticFontCloseParenthesis;
		} else if (cVal >= '0' && cVal <= '9') {
			info = staticFontDigit + (cVal - '0');
		}
		if (info) {
			prevCharSkipped = false;
			prevExtraSpaceRight = (float)info->extraSpaceRight * charInfoOffsetScale * coefPremult;
			
			TextureBoxParams box;
			box.color = color;
			box.xStart = x;
			x += (float)info->sizeX * coefPremult;
			box.xEnd = x;
			box.yStart = y + (float)info->offsetY * charInfoOffsetScale * coefPremult;
			box.yEnd = box.yStart + (float)info->sizeY * coefPremult;
			box.uStart = info->uStart;
			box.uEnd = info->uEnd;
			box.vStart = info->vStart;
			box.vEnd = info->vEnd;
			prepareTextureBox(box, true);
			
		} else {
			prevCharSkipped = true;
		}
	}
}

void Graphics::drawAllFromOutside(IDirect3DDevice9* device) {
	drawAllInit(device);
	drawAll();
}

void Graphics::drawAllInit(IDirect3DDevice9* device) {
	graphicsThreadId = GetCurrentThreadId();
	onEndSceneStart(device);
	drawOutlineCallParamsManager.onEndSceneStart();
	camera.onEndSceneStart();
	initViewport(device);
	fillInScreenSize(device);
}

void Graphics::initViewport(IDirect3DDevice9* device) {
	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	viewportW = (float)viewport.Width;
	viewportH = (float)viewport.Height;
}

void Graphics::fillInScreenSize(IDirect3DDevice9* device) {
	ui.screenSizeKnown = true;
	ui.screenWidth = viewportW;
	ui.screenHeight = viewportH;
	ui.usePresentRect = *usePresentRectPtr != 0;
	ui.presentRectW = *presentRectWPtr;
	ui.presentRectH = *presentRectHPtr;
}
