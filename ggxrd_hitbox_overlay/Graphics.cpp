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
			&orig_UpdateD3DDeviceFromViewportsMutex,
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
			&orig_FSuspendRenderingThreadMutex,
			"FSuspendRenderingThread")) return false;
	} else return false;
	
	if (orig_FSuspendRenderingThreadDestructor) {
		void(HookHelp::*FSuspendRenderingThreadDestructorHookPtr)() = &HookHelp::FSuspendRenderingThreadDestructorHook;
		if (!detouring.attach(
			&(PVOID&)(orig_FSuspendRenderingThreadDestructor),
			(PVOID&)FSuspendRenderingThreadDestructorHookPtr,
			&orig_FSuspendRenderingThreadDestructorMutex,
			"~FSuspendRenderingThread")) return false;
	} else return false;
	
	shutdownFinishedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!shutdownFinishedEvent || shutdownFinishedEvent == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "Failed to create event: %ls\n", winErr.getMessage()));
		return false;
	}
	
	this->hInstance = hInstance;
	
	return !error;
}

// This function is called from the main thread.
// It 'initializes the D3D device for the current viewport state.'
void Graphics::HookHelp::UpdateD3DDeviceFromViewportsHook() {
	HookGuard hookGuard("UpdateD3DDeviceFromViewports");
	graphics.suspenderThreadId = GetCurrentThreadId();
	// This function will call the constructor of class FSuspendRenderingThread, which we hooked.
	// That constructor stops the rendering thread, so that this function could manipulate graphics
	// resources safely.
	// We must release our own resources only after the suspension occurs.
	// We must recreate our own resources either on the graphics thread or before the resuming occurs.
	{
		std::unique_lock<std::mutex> guard(graphics.orig_UpdateD3DDeviceFromViewportsMutex);
		graphics.orig_UpdateD3DDeviceFromViewports((char*)this);
	}
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

void Graphics::onDllDetach() {
	logwrap(fputs("Graphics::onDllDetach called\n", logfile));
	// this tells various callers to stop trying to use the resources as they're about to be freed
	shutdown = true;
	if (!graphicsThreadId || graphicsThreadId == detouring.dllMainThreadId) {
		resetHook();
		ui.onDllDetachGraphics();
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
		resetHook();
		ui.onDllDetachGraphics();
		return;
	}
	if (GetProcessIdOfThread(graphicsThreadHandle) != GetCurrentProcessId()) {
		CloseHandle(graphicsThreadHandle);
		logwrap(fprintf(logfile, "Graphics freeing resources on DLL thread, because thread is no longer alive"));
		resetHook();
		ui.onDllDetachGraphics();
		return;
	}
	DWORD exitCode;
	bool stillActive = GetExitCodeThread(graphicsThreadHandle, &exitCode) && exitCode == STILL_ACTIVE;
	CloseHandle(graphicsThreadHandle);
	
	// free the resource - and stop using it
	
	if (!stillActive) {
		logwrap(fprintf(logfile, "Graphics freeing resources on DLL thread, because thread is no longer alive (2)"));
		resetHook();
		ui.onDllDetachGraphics();
		return;
	}
	
	logwrap(fputs("Graphics calling WaitForSingleObject\n", logfile));
	DWORD result = WaitForSingleObject(shutdownFinishedEvent, 300);
	if (result != WAIT_OBJECT_0) {
		logwrap(fprintf(logfile, "Graphics freeing resources on DLL thread, because WaitForSingleObject did not return success"));
		// We were hoping to free resources on the graphics thread, but if that's not possible, we free them on this thread
		resetHook();
		ui.onDllDetachGraphics();
		return;
	}
	logwrap(fprintf(logfile, "Graphics freed resources successfully\n"));
}

void Graphics::onEndSceneStart(IDirect3DDevice9* device) {
	if (shutdown) return;
	this->device = device;
	stencil.onEndSceneStart();
}

void Graphics::onShutdown() {
	resetHook();
	ui.onDllDetachGraphics();
	SetEvent(shutdownFinishedEvent);
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

	D3DXVECTOR3 v1{ (float)left, 0.F, (float)top };
	D3DXVECTOR3 v2{ (float)left, 0.F, (float)bottom };
	D3DXVECTOR3 v3{ (float)right, 0.F, (float)top };
	D3DXVECTOR3 v4{ (float)right, 0.F, (float)bottom };

	D3DXVECTOR3 sp1, sp2, sp3, sp4;
	logOnce(fprintf(logfile, "Drawing box v1 { x: %f, z: %f }, v2 { x: %f, z: %f }, v3 { x: %f, z: %f }, v4 { x: %f, z: %f }\n",
			v1.x, v1.z, v2.x, v2.z, v3.x, v3.z, v4.x, v4.z));
	if (!worldToScreen(v1, &sp1)) return false;
	if (!worldToScreen(v2, &sp2)) return false;
	if (!worldToScreen(v3, &sp3)) return false;
	if (!worldToScreen(v4, &sp4)) return false;

	bool drewRect = false;

	if ((params.fillColor & 0xFF000000) != 0 && !ignoreFill) {
		drewRect = true;
		if (boundingRect) {
			boundingRect->addX(sp1.x);
			boundingRect->addX(sp2.x);
			boundingRect->addX(sp3.x);
			boundingRect->addX(sp4.x);

			boundingRect->addY(sp1.y);
			boundingRect->addY(sp2.y);
			boundingRect->addY(sp3.y);
			boundingRect->addY(sp4.y);
		}

		logOnce(fprintf(logfile,
			"Box. Red: %u; Green: %u; Blue: %u; Alpha: %u;\n",
			(params.fillColor >> 16) & 0xff, (params.fillColor >> 8) & 0xff, params.fillColor & 0xff, (params.fillColor >> 24) & 0xff));
		logOnce(fprintf(logfile,
			"sp1 { x: %f; y: %f; }; sp2 { x: %f; y: %f; }; sp3 { x: %f; y: %f; }; sp4 { x: %f; y: %f; }\n",
			sp1.x, sp1.y, sp2.x, sp2.y, sp3.x, sp3.y, sp4.x, sp4.y));

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
				const Vertex firstVertex{ sp1.x, sp1.y, 0.F, 1.F, fillColor };
				*vertexIt = firstVertex;
				++vertexIt;
				*vertexIt = firstVertex;
				++vertexIt;
				vertexBufferLength += 6;
				vertexBufferRemainingSize -= 6;
			} else {
				vertexBufferLength += 4;
				vertexBufferRemainingSize -= 4;
				*vertexIt = Vertex{ sp1.x, sp1.y, 0.F, 1.F, fillColor };
				++vertexIt;
			}
		} else {
			drawIfOutOfSpace(4);
			vertexBufferLength += 4;
			vertexBufferRemainingSize -= 4;
			*vertexIt = Vertex{ sp1.x, sp1.y, 0.F, 1.F, fillColor };
			++vertexIt;
		}
		*vertexIt = Vertex{ sp2.x, sp2.y, 0.F, 1.F, fillColor };
		++vertexIt;
		*vertexIt = Vertex{ sp3.x, sp3.y, 0.F, 1.F, fillColor };
		++vertexIt;
		*vertexIt = Vertex{ sp4.x, sp4.y, 0.F, 1.F, fillColor };
		++vertexIt;
		++preparedBoxesCount;
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_BOX;
	}

	if (params.thickness && !ignoreOutline && screenshotStage != SCREENSHOT_STAGE_BASE_COLOR) {
		DrawOutlineCallParams drawOutlineCallParams;
		drawOutlineCallParams.reserveSize(4);

		drawOutlineCallParams.addPathElem(sp1.x, sp1.y, left, top, 1, 1);
		drawOutlineCallParams.addPathElem(sp2.x, sp2.y, left, bottom, 1, -1);
		drawOutlineCallParams.addPathElem(sp4.x, sp4.y, right, bottom, -1, -1);
		drawOutlineCallParams.addPathElem(sp3.x, sp3.y, right, top, -1, 1);
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

void Graphics::sendAllPreparedVertices() {
	if (vertexBufferSent) return;
	vertexBufferSent = true;
	Vertex* buffer = nullptr;
	if (FAILED(vertexBuffer->Lock(0, 0, (void**)&buffer, D3DLOCK_DISCARD))) return;
	memcpy(buffer, &vertexArena.front(), sizeof(Vertex) * vertexBufferLength);
	if (FAILED(vertexBuffer->Unlock())) return;
}

bool Graphics::drawAllArrayboxes() {
	if (preparedArrayboxes.empty()) return true;
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
		logwrap(fprintf(logfile, "Arrayboxes count: %u\nboxes count (including those in arrayboxes): %u\noutlines count: %u, points count: %u\n",
			preparedArrayboxes.size(), preparedBoxesCount, preparedOutlines.size(), numberOfPointsPrepared));
	}
	switch (1) {
	case 1:
		if (!drawAllArrayboxes()) break;
		drawAllBoxes();
		if (!drawAllOutlines()) break;
		drawAllPoints();
	}
	if (vertexBufferPosition != 0) {
		if (vertexBufferPosition > vertexBufferLength) {
			logwrap(fprintf(logfile, "vertexBufferPosition > vertexBufferLength: %u, %u\n", vertexBufferPosition, vertexBufferLength));
		}
		if (vertexBufferPosition != vertexBufferLength) {
			auto destinationIt = vertexArena.begin();
			const auto itEnd = vertexArena.begin() + vertexBufferLength;
			for (auto it = vertexArena.begin() + vertexBufferPosition; it != itEnd; ++it) {
				*destinationIt = *it;
				++destinationIt;
			}
		}
		vertexBufferLength -= vertexBufferPosition;
		vertexBufferPosition = 0;
		vertexIt = vertexArena.begin() + vertexBufferLength;
		vertexBufferRemainingSize = vertexBufferSize - vertexBufferLength;
		if (!loggedDrawingOperationsOnce) {
			logwrap(fprintf(logfile, "drawAllPrepared: resetting vertex buffer: vertexBufferLength: %u, vertexBufferPosition: 0, vertexIt: %u, vertexBufferRemainingSize: %u\n",
				vertexBufferLength, vertexIt - vertexArena.begin(), vertexBufferRemainingSize));
		}
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	}
	vertexBufferSent = false;
}

void Graphics::drawAll() {
	
	initializeVertexBuffers();
	resetVertexBuffer();
	preparedArrayboxIdCounter = 0;
	needClearStencil = false;

	drawingWhat = RENDER_STATE_DRAWING_ARRAYBOXES;
	CComPtr<IDirect3DStateBlock9> oldState = nullptr;
	if (screenshotStage == SCREENSHOT_STAGE_NONE) {
		device->CreateStateBlock(D3DSBT_ALL, &oldState);
		device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetVertexShader(nullptr);
		device->SetPixelShader(nullptr);
		device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		device->SetTexture(0, nullptr);
		device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
	}
	
	if (!onlyDrawPoints) {
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
		for (DrawOutlineCallParams& params : outlines) {
			prepareOutline(params);
		}
	}
	for (const DrawPointCallParams& params : drawDataUse.points) {
		preparePoint(params);
	}
	drawAllPrepared();
	
	outlines.clear();

	if (screenshotStage == SCREENSHOT_STAGE_NONE) {
		stencil.onEndSceneEnd(device);
		oldState->Apply();
	}

	loggedDrawingOperationsOnce = true;

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

Graphics::Vertex::Vertex(float x, float y, float z, float rhw, D3DCOLOR color)
	: x(x), y(y), z(z), rhw(rhw), color(color) { }

void Graphics::prepareOutline(DrawOutlineCallParams& params) {
	if (screenshotStage == SCREENSHOT_STAGE_BASE_COLOR) return;
	logOnce(fprintf(logfile, "Called drawOutlines with an outline with %d elements\n", params.count()));
	
	D3DXVECTOR3 conv;
	PreparedOutline* preparedOutlinePtr = nullptr;
	
	if (params.thickness == 1) {
		
		if (params.empty()) return;

		const bool alreadyProjected = params.getPathElem(0).hasProjectionAlready;
		if (!alreadyProjected) {
			for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
				PathElement& elem = params.getPathElem(outlineIndex);
				if (!worldToScreen(D3DXVECTOR3{ (float)elem.x, 0.F, (float)elem.y }, &conv)) {
					return;
				}
				elem.xProjected = conv.x;
				elem.yProjected = conv.y;
				elem.hasProjectionAlready = true;
			}
		}

		preparedOutlines.emplace_back();
		preparedOutlines.back().isOnePixelThick = true;

		Vertex firstVertex;

		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			drawIfOutOfSpace(1);
			if (outlineIndex != 0) {
				++preparedOutlines.back().linesSoFar;
			}

			logOnce(fprintf(logfile, "x: %f; y: %f;\n", elem.xProjected, elem.yProjected));
			*vertexIt = Vertex{ elem.xProjected, elem.yProjected, 0.F, 1.F, params.outlineColor };
			if (outlineIndex == 0) firstVertex = *vertexIt;
			++vertexIt;
			++vertexBufferLength;
			--vertexBufferRemainingSize;
		}
		drawIfOutOfSpace(1);
		PreparedOutline& preparedOutline = preparedOutlines.back();
		preparedOutlinePtr = &preparedOutline;
		++preparedOutline.linesSoFar;
		preparedOutline.isComplete = true;
		*vertexIt = firstVertex;
		++vertexIt;
		++vertexBufferLength;
		--vertexBufferRemainingSize;
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_THINLINE;

	} else if (!params.empty()) {
		
		const bool alreadyProjected = params.getPathElem(0).hasProjectionAlready;
		if (!alreadyProjected) {
			for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
				PathElement& elem = params.getPathElem(outlineIndex);
				if (!worldToScreen(D3DXVECTOR3{ (float)elem.x, 0.F, (float)elem.y }, &conv)) return;
				elem.xProjected = conv.x;
				elem.yProjected = conv.y;
				elem.hasProjectionAlready = true;
			}
		}
		
		std::vector<D3DXVECTOR3> extraPoints(params.count(), D3DXVECTOR3{ 0.F, 0.F, 0.F });
		
		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			PathElement& elem = params.getPathElem(outlineIndex);
			if (!worldToScreen(
					D3DXVECTOR3{
						(float)elem.x + params.thickness * elem.inX,
						0.F,
						(float)elem.y + params.thickness * elem.inY
					},
					&extraPoints[outlineIndex]
				)) {
				return;
			}
		}

		preparedOutlines.emplace_back();

		Vertex firstVertex;
		Vertex secondVertex;

		bool padTheFirst = (lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_THICKLINE);
		
		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			conv.x = elem.xProjected;
			conv.y = elem.yProjected;

			logOnce(fprintf(logfile, "x: %f; y: %f;\n", elem.xProjected, elem.yProjected));
			if (padTheFirst && !drawIfOutOfSpace(4)) {
				firstVertex = Vertex{ elem.xProjected, elem.yProjected, 0.F, 1.F, params.outlineColor };
				*vertexIt = *(vertexIt - 1);
				++vertexIt;
				*vertexIt = firstVertex;
				++vertexIt;
				*vertexIt = firstVertex;
				++vertexIt;
				vertexBufferLength += 4;
				vertexBufferRemainingSize -= 4;
				preparedOutlines.back().hasPadding = true;
			} else {
				drawIfOutOfSpace(2);
				if (outlineIndex == 0) {
					firstVertex = Vertex{ elem.xProjected, elem.yProjected, 0.F, 1.F, params.outlineColor };
					*vertexIt = firstVertex;
					++vertexIt;
				} else {
					++preparedOutlines.back().linesSoFar;
					*vertexIt = Vertex{ elem.xProjected, elem.yProjected, 0.F, 1.F, params.outlineColor };
					++vertexIt;
				}
				vertexBufferLength += 2;
				vertexBufferRemainingSize -= 2;
			}
			padTheFirst = false;
			
			const D3DXVECTOR3& extraPoint = extraPoints[outlineIndex];

			if (outlineIndex == 0) {
				secondVertex = Vertex{ extraPoint.x, extraPoint.y, 0.F, 1.F, params.outlineColor };
				*vertexIt = secondVertex;
				++vertexIt;
			} else {
				*vertexIt = Vertex{ extraPoint.x, extraPoint.y, 0.F, 1.F, params.outlineColor };
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
		vertexBufferLength += 2;
		vertexBufferRemainingSize -= 2;
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
			enum Direction {
				DIRECTION_NONE,
				DIRECTION_UP,
				DIRECTION_RIGHT,
				DIRECTION_DOWN,
				DIRECTION_LEFT
			} lastDir = DIRECTION_NONE;
			bool lastAddedHatch = false;
			
			const PathElement* pathElementEnd = pathElementStart;
			Direction currentDir;
			
			for (int i = 0; i < pointStart.count; ++i) {
				pathElementStart = pathElementEnd;
				if (i == pointStart.count - 1) {
					pathElementEnd = &DrawOutlineCallParams::getPathElemStatic(pointStart.start, 0);
				} else {
					pathElementEnd = &DrawOutlineCallParams::getPathElemStatic(pointStart.start, i + 1);
				}
				
				bool addedHatch = false;
				int lineHasX;
				int lineHasY;
				int distEnd;
				if (pathElementEnd->x == pathElementStart->x) {
					lineHasX = 0;
					lineHasY = 1;
					if (pathElementEnd->y == pathElementStart->y) {
						continue;
					}
					distEnd = dist + pathElementEnd->y - pathElementStart->y;
					if (pathElementEnd->y > pathElementStart->y) {
						currentDir = DIRECTION_UP;
					} else {
						currentDir = DIRECTION_DOWN;
					}
				} else {
					lineHasX = 1;
					lineHasY = 0;
					distEnd = dist + pathElementEnd->x - pathElementStart->x;
					if (pathElementEnd->x > pathElementStart->x) {
						currentDir = DIRECTION_RIGHT;
					} else {
						currentDir = DIRECTION_LEFT;
					}
				}
				
				int off = -dist;
				if (distEnd > 0) {
					if (dist < 0) {
						hatchArena.emplace_back();
						HatchPoint& hatchPoint = hatchArena.back();
						hatchPoint.n = n;
						hatchPoint.x = pathElementStart->x + lineHasX * off;
						hatchPoint.y = pathElementStart->y + lineHasY * off;
						addedHatch = true;
					}
					while (distEnd >= hatchesDist) {
						++n;
						distEnd -= hatchesDist;
						off += hatchesDist;
						if (distEnd != 0 || pathElementEnd->inX != pathElementEnd->inY) {
							hatchArena.emplace_back();
							HatchPoint& hatchPoint = hatchArena.back();
							hatchPoint.n = n;
							hatchPoint.x = pathElementStart->x + lineHasX * off;
							hatchPoint.y = pathElementStart->y + lineHasY * off;
							addedHatch = true;
						}
					}
				} else {
					if (dist > 0) {
						if (distEnd != 0 || pathElementEnd->inX != pathElementEnd->inY) {
							hatchArena.emplace_back();
							HatchPoint& hatchPoint = hatchArena.back();
							hatchPoint.n = n;
							hatchPoint.x = pathElementStart->x + lineHasX * off;
							hatchPoint.y = pathElementStart->y + lineHasY * off;
							addedHatch = true;
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
							hatchPoint.x = pathElementStart->x + lineHasX * off;
							hatchPoint.y = pathElementStart->y + lineHasY * off;
							addedHatch = true;
						}
					}
				}
				dist = distEnd;
				lastAddedHatch = addedHatch;
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
				if (!worldToScreen(
						D3DXVECTOR3{
							(float)hatchPoint.x,
							0.F,
							(float)hatchPoint.y
						},
						&pnt1
					)) {
					return;
				}
				hasPnt1 = true;
				continue;
			}
			
			D3DXVECTOR3 pnt2;
			if (!worldToScreen(
					D3DXVECTOR3{
						(float)hatchPoint.x,
						0.F,
						(float)hatchPoint.y
					},
					&pnt2
				)) {
				return;
			}
			hasPnt1 = false;
			
			drawIfOutOfSpace(2);
			++preparedOutlinePtr->hatchesCount;
			*vertexIt = Vertex{ pnt1.x, pnt1.y, 0.F, 1.F, params.outlineColor };
			++vertexIt;
			*vertexIt = Vertex{ pnt2.x, pnt2.y, 0.F, 1.F, params.outlineColor };
			++vertexIt;
			vertexBufferLength += 2;
			lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_HATCH;
		}
		preparedOutlinePtr->hatchesComplete = true;
	}
}

bool Graphics::worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out) {
	return camera.worldToScreen(device, vec, out);
}

void Graphics::preparePoint(const DrawPointCallParams& params) {
	if (screenshotStage == SCREENSHOT_STAGE_BASE_COLOR) return;
	D3DXVECTOR3 p{ (float)params.posX, 0.F, (float)params.posY };
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
	vertexBufferLength += 14;
	vertexBufferRemainingSize -= 14;

	const D3DCOLOR fillColor = params.fillColor;

	*vertexIt = Vertex{ sp.x - 4, sp.y - 1, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 4, sp.y + 2, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 5, sp.y - 1, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 5, sp.y + 2, 0.F, 1.F, fillColor };
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
	*vertexIt = Vertex{ sp.x + 5, sp.y + 2, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 1, sp.y - 4, 0.F, 1.F, fillColor };
	++vertexIt;

	*vertexIt = Vertex{ sp.x - 1, sp.y - 4, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x - 1, sp.y + 5, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 2, sp.y - 4, 0.F, 1.F, fillColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 2, sp.y + 5, 0.F, 1.F, fillColor };
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

	*vertexIt = Vertex{ sp.x - 3, sp.y, 0.F, 1.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x + 4, sp.y, 0.F, 1.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x, sp.y - 3, 0.F, 1.F, outlineColor };
	++vertexIt;
	*vertexIt = Vertex{ sp.x, sp.y + 4, 0.F, 1.F, outlineColor };
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
	pngRelated.writePngToPath(filename, width, height, &gameImage.front());

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
	
	pngRelated.saveScreenshotData(renderTargetDesc.Width, renderTargetDesc.Height, &gameImage.front());
	
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
	std::vector<unsigned char> gameImage;
	if (!getFramebufferData(device, gameImage, renderTarget, &renderTargetDesc)) return;

	if (!settings.dontUseScreenshotTransparency) {
		union Pixel {
			struct { unsigned char r, g, b, a; };
			int value;
		};
		Pixel* gameImagePtr = (Pixel*)&gameImage.front();
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

	pngRelated.saveScreenshotData(renderTargetDesc.Width, renderTargetDesc.Height, &gameImage.front());

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
	memcpy((void*)&buffer.front(), lockedRect.pBits, imageSize * 4);

	offscreenSurface->UnlockRect();

	return true;
}

void Graphics::takeScreenshotMain(IDirect3DDevice9* device, bool useSimpleVerion) {
	if (useSimpleVerion) {
		takeScreenshotSimple(device);
		return;
	}
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
	device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

	device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
	device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
	device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);

	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
	device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	
	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->SetTexture(0, nullptr);
	device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
	screenshotStage = SCREENSHOT_STAGE_BASE_COLOR;
	
	PERFORMANCE_MEASUREMENT_START
	
	drawAll();
	
	PERFORMANCE_MEASUREMENT_END(screenshotDrawAll1)
	
	logOnce(fputs("drawAll() (for screenshot) call successful\n", logfile));
	
	// This step blends the colors with alpha and just does everything normally
	device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	// 1-(1-a)*(1-b) = a+b(1-a)
	device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
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
}

void Graphics::advanceRenderState(RenderStateDrawingWhat newState) {
	if (screenshotStage != SCREENSHOT_STAGE_BASE_COLOR
			&& drawingWhat == RENDER_STATE_DRAWING_ARRAYBOXES
			&& newState != RENDER_STATE_DRAWING_ARRAYBOXES) {
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	}
	if (drawingWhat != RENDER_STATE_DRAWING_OUTLINES
			&& drawingWhat != RENDER_STATE_DRAWING_POINTS
			&& (newState == RENDER_STATE_DRAWING_OUTLINES
			|| newState == RENDER_STATE_DRAWING_POINTS)) {
		if (screenshotStage != SCREENSHOT_STAGE_NONE) {
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
		} else {
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}
	}
	if (drawingWhat != RENDER_STATE_DRAWING_OUTLINES
			&& newState == RENDER_STATE_DRAWING_OUTLINES) {
		if (screenshotStage == SCREENSHOT_STAGE_NONE) {
			// Thanks to WorseThanYou for advice on this
			preparePixelShader(device);
		}
	}
	if (drawingWhat == RENDER_STATE_DRAWING_OUTLINES
			&& newState != RENDER_STATE_DRAWING_OUTLINES) {
		if (screenshotStage == SCREENSHOT_STAGE_NONE) {
			device->SetPixelShader(nullptr);
			device->SetTexture(0, nullptr);
		}
	}
	drawingWhat = newState;
}

bool Graphics::initializeVertexBuffers() {
	if (failedToCreateVertexBuffers) return false;
	if (vertexBuffer) return true;
	if (FAILED(device->CreateVertexBuffer(sizeof(Vertex) * vertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vertexBuffer, NULL))) {
		logwrap(fputs("CreateVertexBuffer failed\n", logfile));
		failedToCreateVertexBuffers = false;
		return false;
	}
	vertexArena.resize(vertexBufferSize);

	return true;
}

void Graphics::resetVertexBuffer() {
	vertexBufferRemainingSize = vertexBufferSize;
	vertexBufferLength = 0;
	vertexIt = vertexArena.begin();
	lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_NOTHING;
	vertexBufferPosition = 0;
}

void DrawData::clear() {
	hurtboxes.clear();
	hitboxes.clear();
	pushboxes.clear();
	points.clear();
	throwBoxes.clear();
	needTakeScreenshot = false;
}

bool Graphics::drawIfOutOfSpace(unsigned int verticesCountRequired) {
	if (vertexBufferRemainingSize < verticesCountRequired) {
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
	destination->hurtboxes.insert(destination->hurtboxes.begin(), hurtboxes.begin(), hurtboxes.end());
	destination->hitboxes.insert(destination->hitboxes.begin(), hitboxes.begin(), hitboxes.end());
	destination->pushboxes.insert(destination->pushboxes.begin(), pushboxes.begin(), pushboxes.end());
	destination->points.insert(destination->points.begin(), points.begin(), points.end());
	destination->throwBoxes.insert(destination->throwBoxes.begin(), throwBoxes.begin(), throwBoxes.end());
	destination->needTakeScreenshot = needTakeScreenshot;
}

void Graphics::HookHelp::FSuspendRenderingThreadHook(unsigned int InSuspendThreadFlags) {
	HookGuard hookGuard("FSuspendRenderingThread");
	{
		std::unique_lock<std::mutex> guard(graphics.orig_FSuspendRenderingThreadMutex);
		graphics.orig_FSuspendRenderingThread((char*)this, InSuspendThreadFlags);
	}
	if (graphics.suspenderThreadId == GetCurrentThreadId()) {
		graphics.resetHook();
		ui.handleResetBefore();
	}
}

void Graphics::HookHelp::FSuspendRenderingThreadDestructorHook() {
	HookGuard hookGuard("~FSuspendRenderingThread");
	if (graphics.suspenderThreadId == GetCurrentThreadId() && !graphics.shutdown) {
		// We must recreate our resources before FSuspendRenderingThread resumes the graphics thread, so that
		// there's no race condition between us and the graphics thread
		ui.handleResetAfter();
	}
	{
		std::unique_lock<std::mutex> guard(graphics.orig_FSuspendRenderingThreadDestructorMutex);
		graphics.orig_FSuspendRenderingThreadDestructor((char*)this);
	}
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
	
	// If user can't find D3DCOMPILER_47.dll on their computer, use LoadLibraryA to load an older version of the compiler:
	// D3DCompiler_33.dll
	// D3DCompiler_34.dll
	// D3DCompiler_35.dll
	// D3DCompiler_36.dll
	// D3DCompiler_37.dll
	// D3DCompiler_38.dll
	// D3DCompiler_39.dll
	// D3DCompiler_40.dll
	// D3DCompiler_41.dll
	// D3DCompiler_42.dll
	// D3DCompiler_43.dll
	// Use GetProcAddress to locate D3DCompile and call it through a pointer.
	// Normally these DLLs should be found in C:\Windows\SysWOW64\ on the user's machine.
	// Redistributing these DLLs may be a violation of copyright.
	// Last resort may be using fxc to precompile the shader.
	
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

void Graphics::checkAltRenderTargetLifeTime() {
	if (!altRenderTarget) return;
	if (--altRenderTargetLifeRemaining < 0) {
		altRenderTargetLifeRemaining = 0;
		altRenderTarget = nullptr;
	}
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
