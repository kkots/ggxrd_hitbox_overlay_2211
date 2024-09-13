#include "pch.h"
#include "Graphics.h"
#include "Detouring.h"
#include "Direct3DVTable.h"
#include "Hitbox.h"
#include "Camera.h"
#include "BoundingRect.h"
#include "logging.h"
#include "pi.h"
#include "colors.h"
#include "PngRelated.h"
#include "Settings.h"
#include "UI.h"

Graphics graphics;

bool Graphics::onDllMain() {

	orig_Reset = (Reset_t)direct3DVTable.getDirect3DVTable()[16];
	if (!detouring.attach(
		&(PVOID&)(orig_Reset),
		hook_Reset,
		&orig_ResetMutex,
		"Reset")) return false;

	return true;
}

HRESULT __stdcall hook_Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters) {
	++detouring.hooksCounter;
	detouring.markHookRunning("Reset", true);
	graphics.resetHook();
	ui.handleResetBefore();
	HRESULT result;
	{
		std::unique_lock<std::mutex> guard(graphics.orig_ResetMutex);
		result = graphics.orig_Reset(device, pPresentationParameters);
	}
	ui.handleResetAfter();
	detouring.markHookRunning("Reset", false);
	--detouring.hooksCounter;
	return result;
}

void Graphics::resetHook() {
	stencil.surface = NULL;
	stencil.direct3DSuccess = false;
	offscreenSurface = NULL;
	offscreenSurfaceWidth = 0;
	offscreenSurfaceHeight = 0;
	vertexBuffer = NULL;
}

void Graphics::onUnload() {
	resetHook();
}

void Graphics::onEndSceneStart(IDirect3DDevice9* device) {
	this->device = device;
	stencil.onEndSceneStart();
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
	worldToScreen(v1, &sp1);
	worldToScreen(v2, &sp2);
	worldToScreen(v3, &sp3);
	worldToScreen(v4, &sp4);

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
			const bool drew = drawIfOutOfSpace(6, 0);
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
			drawIfOutOfSpace(4, 0);
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
			} else {
				logwrap(fputs("Suspicious outline behavior in drawAllOutlines\n", logfile));
			}
		} else {
			if (it->linesSoFar) {
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
	
	for (const ComplicatedHurtbox& params : drawDataUse.hurtboxes) {
		prepareComplicatedHurtbox(params);
	}

	for (auto it = drawDataUse.hitboxes.cbegin(); it != drawDataUse.hitboxes.cend(); ++it) {
		const DrawHitboxArrayCallParams& params = *it;
		bool found = false;
		for (auto itScan = it + 1; itScan != drawDataUse.hitboxes.cend(); ++itScan) {
			if (params == *itScan) {
				found = true;
				break;
			}
		}
		if (!found) prepareArraybox(params, false);
	}
	for (const DrawBoxCallParams& params : drawDataUse.pushboxes) {
		prepareBox(params);
	}
	for (const DrawBoxCallParams& params : drawDataUse.throwBoxes) {
		prepareBox(params);
	}
	for (const DrawOutlineCallParams& params : outlines) {
		prepareOutline(params);
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

	float angleRads;
	int cos;
	int sin;
	int angleCapped;
	const bool isRotated = (params.params.angle != 0);
	if (isRotated) {
		angleRads = -(float)params.params.angle / 1000.F / 180.F * PI;
		angleCapped = params.params.angle % 360000;
		if (angleCapped < 0) angleCapped += 360000;
		cos = (int)(::cos(angleRads) * 1000.F);
		sin = (int)(::sin(angleRads) * 1000.F);
	}
	
	DrawBoxCallParams drawBoxCall;
	drawBoxCall.fillColor = params.fillColor;

	const Hitbox* hitboxData = params.hitboxData;
	for (int i = 0; i < params.hitboxCount; ++i) {
		logOnce(fprintf(logfile, "drawing box %d\n", params.hitboxCount - i));

		int offX = params.params.scaleX * ((int)hitboxData->offX + params.params.hitboxOffsetX / 1000 * params.params.flip);
		int offY = params.params.scaleY * (-(int)hitboxData->offY + params.params.hitboxOffsetY / 1000);
		int sizeX = (int)hitboxData->sizeX * params.params.scaleX;
		int sizeY = -(int)hitboxData->sizeY * params.params.scaleY;

		if (isRotated) {
			int centerX = offX + sizeX / 2;
			int centerY = offY + sizeY / 2;
			if (angleCapped >= 45000 && (angleCapped < 135000 || angleCapped >= 225000)) {
				std::swap(sizeX, sizeY);
			}
			offX = (cos * centerX - sin * centerY) / 1000 - sizeX / 2;
			offY = (cos * centerY + sin * centerX) / 1000 - sizeY / 2;
		}

		offX -= params.params.hitboxOffsetX;
		offX = params.params.posX + offX * params.params.flip;
		sizeX *= params.params.flip;

		offY += params.params.posY + params.params.hitboxOffsetY;
		
		if (drawOutlines) {
			rectCombinerInputBoxes.emplace_back(offX, offX + sizeX, offY, offY + sizeY);
		}

		drawBoxCall.left = offX;
		drawBoxCall.right = offX + sizeX;
		drawBoxCall.top = offY;
		drawBoxCall.bottom = offY + sizeY;

		if (prepareBox(drawBoxCall, boundingRect, false, true)) {
			++preparedArrayboxes.back().boxesPreparedSoFar;
		}

		++hitboxData;
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
		for (const std::vector<RectCombiner::PathElement>& outline : rectCombinerOutlines) {
			std::vector<DrawOutlineCallParams>* outlinesDest = outlinesOverride ? outlinesOverride : &outlines;
			outlinesDest->emplace_back();
			DrawOutlineCallParams& drawOutlineCallParams = outlinesDest->back();
			drawOutlineCallParams.outlineColor = params.outlineColor;
			drawOutlineCallParams.thickness = params.thickness;
			drawOutlineCallParams.reserveSize(outline.size());
			for (const RectCombiner::PathElement& path : outline) {
				drawOutlineCallParams.addPathElem(path.x, path.y, path.xDir(), path.yDir());
			}
		}
	}
}

Graphics::Vertex::Vertex(float x, float y, float z, float rhw, D3DCOLOR color)
	: x(x), y(y), z(z), rhw(rhw), color(color) { }

void Graphics::prepareOutline(const DrawOutlineCallParams& params) {
	if (screenshotStage == SCREENSHOT_STAGE_BASE_COLOR) return;
	logOnce(fprintf(logfile, "Called drawOutlines with an outline with %d elements\n", params.count()));
	
	D3DXVECTOR3 conv;

	if (params.thickness == 1) {
		
		if (params.empty()) return;

		preparedOutlines.emplace_back();
		preparedOutlines.back().isOnePixelThick = true;

		Vertex firstVertex;

		const bool alreadyProjected = params.getPathElem(0).hasProjectionAlready;

		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			drawIfOutOfSpace(1, 0);
			if (outlineIndex != 0) {
				++preparedOutlines.back().linesSoFar;
			}

			if (!alreadyProjected) {
				worldToScreen(D3DXVECTOR3{ (float)elem.x, 0.F, (float)elem.y }, &conv);
			} else {
				conv.x = elem.xProjected;
				conv.y = elem.yProjected;
			}

			logOnce(fprintf(logfile, "x: %f; y: %f;\n", conv.x, conv.y));
			*vertexIt = Vertex{ conv.x, conv.y, 0.F, 1.F, params.outlineColor };
			if (outlineIndex == 0) firstVertex = *vertexIt;
			++vertexIt;
			++vertexBufferLength;
			--vertexBufferRemainingSize;
		}
		drawIfOutOfSpace(1, 0);
		PreparedOutline& preparedOutline = preparedOutlines.back();
		++preparedOutline.linesSoFar;
		preparedOutline.isComplete = true;
		*vertexIt = firstVertex;
		++vertexIt;
		++vertexBufferLength;
		--vertexBufferRemainingSize;
		lastThingInVertexBuffer = LAST_THING_IN_VERTEX_BUFFER_END_OF_THINLINE;

	} else if (!params.empty()) {

		preparedOutlines.emplace_back();

		Vertex firstVertex;
		Vertex secondVertex;

		bool padTheFirst = (lastThingInVertexBuffer == LAST_THING_IN_VERTEX_BUFFER_END_OF_THICKLINE);
		const bool alreadyProjected = params.getPathElem(0).hasProjectionAlready;

		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			if (!alreadyProjected) {
				worldToScreen(D3DXVECTOR3{ (float)elem.x, 0.F, (float)elem.y }, &conv);
			} else {
				conv.x = elem.xProjected;
				conv.y = elem.yProjected;
			}

			logOnce(fprintf(logfile, "x: %f; y: %f;\n", conv.x, conv.y));
			if (padTheFirst && !drawIfOutOfSpace(4, 0)) {
				firstVertex = Vertex{ conv.x, conv.y, 0.F, 1.F, params.outlineColor };
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
				drawIfOutOfSpace(2, 0);
				if (outlineIndex == 0) {
					firstVertex = Vertex{ conv.x, conv.y, 0.F, 1.F, params.outlineColor };
					*vertexIt = firstVertex;
					++vertexIt;
				} else {
					++preparedOutlines.back().linesSoFar;
					*vertexIt = Vertex{ conv.x, conv.y, 0.F, 1.F, params.outlineColor };
					++vertexIt;
				}
				vertexBufferLength += 2;
				vertexBufferRemainingSize -= 2;
			}
			padTheFirst = false;
			worldToScreen(D3DXVECTOR3{ (float)elem.x + params.thickness * elem.inX, 0.F, (float)elem.y + params.thickness * elem.inY }, &conv);

			if (outlineIndex == 0) {
				secondVertex = Vertex{ conv.x, conv.y, 0.F, 1.F, params.outlineColor };
				*vertexIt = secondVertex;
				++vertexIt;
			} else {
				*vertexIt = Vertex{ conv.x, conv.y, 0.F, 1.F, params.outlineColor };
				++vertexIt;
			}
		}
		drawIfOutOfSpace(2, 0);
		PreparedOutline& preparedOutline = preparedOutlines.back();
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
}

void Graphics::worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out) {
	camera.worldToScreen(device, vec, out);
}

void Graphics::preparePoint(const DrawPointCallParams& params) {
	if (screenshotStage == SCREENSHOT_STAGE_BASE_COLOR) return;
	D3DXVECTOR3 p{ (float)params.posX, 0.F, (float)params.posY };
	logOnce(fprintf(logfile, "drawPoint called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));

	D3DXVECTOR3 sp;
	worldToScreen(p, &sp);

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

	drawIfOutOfSpace(14, 0);
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

	if (FAILED(device->GetRenderTarget(0, &gamesRenderTarget))) {
		logwrap(fputs("GetRenderTarget failed\n", logfile));
		return false;
	}

	D3DSURFACE_DESC renderTargetDesc;
	SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
	if (FAILED(gamesRenderTarget->GetDesc(&renderTargetDesc))) {
		logwrap(fputs("GetDesc failed\n", logfile));
		return false;
	}

	CComPtr<IDirect3DSurface9> newRenderTarget = nullptr;
	if (FAILED(device->CreateRenderTarget(renderTargetDesc.Width,
		renderTargetDesc.Height,
		renderTargetDesc.Format,
		renderTargetDesc.MultiSampleType,
		renderTargetDesc.MultiSampleQuality,
		FALSE,
		&newRenderTarget,
		NULL))) {
		logwrap(fputs("CreateRenderTarget failed\n", logfile));
		return false;
	}

	if (FAILED(device->SetRenderTarget(0, newRenderTarget))) {
		logwrap(fputs("SetRenderTarget failed\n", logfile));
		return false;
	}
	
	struct SetOldRenderTargetOnFail {
	public:
		~SetOldRenderTargetOnFail() {
			if (success) return;
			device->SetRenderTarget(0, gamesRenderTarget);
			gamesRenderTarget = nullptr;
		}
		IDirect3DDevice9* device = nullptr;
		CComPtr<IDirect3DSurface9>& gamesRenderTarget;
		bool success = false;
	} setOldRenderTargetOnFail { device, gamesRenderTarget };
	
	if (FAILED(device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.F, 0))) {
		logwrap(fputs("Clear failed\n", logfile));
		return false;
	}
	
	setOldRenderTargetOnFail.success = true;
	return true;
}

void Graphics::takeScreenshotDebug(IDirect3DDevice9* device, const wchar_t* filename) {
	std::vector<unsigned char> gameImage;
	unsigned int width = 0;
	unsigned int height = 0;
	if (!!getFramebufferData(device, gameImage, nullptr, nullptr, &width, &height)) return;
	pngRelated.writePngToPath(filename, width, height, &gameImage.front());

}

void Graphics::takeScreenshotEnd(IDirect3DDevice9* device) {
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

	// Thanks to WorseThanYou for writing this CPU pixel blender
	union Pixel {
		struct { unsigned char r, g, b, a; };
		int value;
	};
	Pixel* gameImagePtr = (Pixel*)&gameImage.front();
	Pixel* boxesImagePtr = (Pixel*)&boxesImage.front();
	const size_t offLimit = renderTargetDesc.Width * renderTargetDesc.Height * 4;
	for (size_t off = 0; off < offLimit; off += 4)
	{
		Pixel& d = *gameImagePtr;
		Pixel& s = *boxesImagePtr;

		if (s.a != 255) {
			s.a /= 2;
		}

		d.a ^= 255;
		unsigned int maxColor = d.r;
		if (d.g > maxColor) maxColor = d.g;
		if (d.b > maxColor) maxColor = d.b;
		if (maxColor > d.a) d.a = maxColor;
		unsigned char daInv = ~d.a, saInv = 255 ^ s.a;
		d.r = (daInv * s.r + d.a * (s.r * s.a + d.r * saInv) / 255) / 255;
		d.g = (daInv * s.g + d.a * (s.g * s.a + d.g * saInv) / 255) / 255;
		d.b = (daInv * s.b + d.a * (s.b * s.a + d.b * saInv) / 255) / 255;
		d.a = (daInv * s.a + d.a * (255 * 255 - daInv * saInv) / 255) / 255;
		++gameImagePtr;
		++boxesImagePtr;
	}

	pngRelated.saveScreenshotData(renderTargetDesc.Width, renderTargetDesc.Height, &gameImage.front());
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
	
	drawAll();
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
	
	drawAll();
	
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
	drawingWhat = newState;
}

bool Graphics::initializeVertexBuffers() {
	if (failedToCreateVertexBuffers) return false;
	if (vertexBuffer) return true;
	if (FAILED(device->CreateVertexBuffer(sizeof(Vertex) * vertexBufferSize, D3DUSAGE_DYNAMIC, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vertexBuffer, NULL))) {
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

bool Graphics::drawIfOutOfSpace(unsigned int verticesCountRequired, unsigned int texturedVerticesCountRequired) {
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
	destination->id = id;
}
