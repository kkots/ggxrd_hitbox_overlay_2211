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
	HRESULT result;
	{
		std::unique_lock<std::mutex> guard(graphics.orig_ResetMutex);
		result = graphics.orig_Reset(device, pPresentationParameters);
	}
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
}

void Graphics::onUnload() {
	stencil.surface = NULL;
}

void Graphics::onEndSceneStart(IDirect3DDevice9* device) {
	this->device = device;
	hurtboxes.clear();
	hitboxes.clear();
	outlines.clear();
	throwBoxes.clear();
	points.clear();
	pushboxes.clear();
	stencil.onEndSceneStart();
}

void Graphics::drawBox(const DrawBoxCallParams& params, BoundingRect* const boundingRect, bool useStencil) {
	if (params.left == params.right
		|| params.top == params.bottom) return;

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

	if ((params.fillColor >> 24) != 0) {
		D3DXVECTOR3 v1{ (float)left, 0.F, (float)top };
		D3DXVECTOR3 v2{ (float)left, 0.F, (float)bottom };
		D3DXVECTOR3 v3{ (float)right, 0.F, (float)top };
		D3DXVECTOR3 v4{ (float)right, 0.F, (float)bottom };

		if (useStencil) {
			stencil.initialize(device);
		}

		D3DXVECTOR3 sp1, sp2, sp3, sp4;
		logOnce(fprintf(logfile, "Drawing box v1 { x: %f, z: %f }, v2 { x: %f, z: %f }, v3 { x: %f, z: %f }, v4 { x: %f, z: %f }\n",
			v1.x, v1.z, v2.x, v2.z, v3.x, v3.z, v4.x, v4.z));
		worldToScreen(v1, &sp1);
		worldToScreen(v2, &sp2);
		worldToScreen(v3, &sp3);
		worldToScreen(v4, &sp4);

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

		if (allowedToDrawFills) {
			if (stencil.initialized && allowedToUseStencil) {
				device->SetRenderState(D3DRS_STENCILENABLE, TRUE);  // Thanks to WorseThanYou for the idea of using Stenciling
				device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
				device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
			}
			if (!screenshotMode) {
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}
			device->SetVertexShader(nullptr);
			device->SetPixelShader(nullptr);
			device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
			device->SetTexture(0, nullptr);
			D3DCOLOR fillColor;
			if (screenshotMode) {
				unsigned int newAlphaValue = ((unsigned int)(params.fillColor) >> 24) * 2;
				if (newAlphaValue > 255) newAlphaValue = 255;
				fillColor = replaceAlpha(newAlphaValue, params.fillColor);
			} else {
				fillColor = params.fillColor;
			}

			Vertex vertices[] =
			{
				{ sp1.x, sp1.y, 0.F, 1.F, fillColor },  // Thanks to WorseThanYou for fixing the RHW value for Intel GPUs (was 0.F, didn't work)
				{ sp2.x, sp2.y, 0.F, 1.F, fillColor },
				{ sp3.x, sp3.y, 0.F, 1.F, fillColor },
				{ sp4.x, sp4.y, 0.F, 1.F, fillColor },
			};

			// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
			// Second parameter is the number of triangles.
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
		}
	}

	if (params.thickness) {
		DrawOutlineCallParams drawOutlineCallParams;
		drawOutlineCallParams.reserveSize(4);

		drawOutlineCallParams.addPathElem(left, top, 1, 1);
		drawOutlineCallParams.addPathElem(left, bottom, 1, -1);
		drawOutlineCallParams.addPathElem(right, bottom, -1, -1);
		drawOutlineCallParams.addPathElem(right, top, -1, 1);
		drawOutlineCallParams.outlineColor = params.outlineColor;
		drawOutlineCallParams.thickness = params.thickness;
		outlines.push_back(drawOutlineCallParams);
	}
}

void Graphics::drawAll() {

	for (const ComplicatedHurtbox& params : hurtboxes) {
		if (params.hasTwo) {
			std::vector<DrawOutlineCallParams> nonOldOutlines;
			BoundingRect boundingRect;
			drawHitboxArray(params.param1, &boundingRect, false, &nonOldOutlines);
			drawHitboxArray(params.param2, &boundingRect, true);
			outlines.insert(outlines.end(), nonOldOutlines.begin(), nonOldOutlines.end());
		} else {
			drawHitboxArray(params.param1);
		}
	}
	for (auto it = hitboxes.cbegin(); it != hitboxes.cend(); ++it) {
		const DrawHitboxArrayCallParams& params = *it;
		bool found = false;
		for (auto itScan = it; itScan != hitboxes.cend(); ++itScan) {
			if (it == itScan) continue;

			if (params == *itScan) {
				found = true;
				break;
			}
		}
		if (!found) drawHitboxArray(params);
	}
	for (const DrawBoxCallParams& params : pushboxes) {
		drawBox(params);
	}
	for (const DrawBoxCallParams& params : throwBoxes) {
		drawBox(params);
	}
	for (const DrawOutlineCallParams& params : outlines) {
		drawOutline(params);
	}
	for (const DrawPointCallParams& params : points) {
		drawPoint(params);
	}

	/*for (const auto &ti : throws)
		draw_throw(device, ti, stencilInitialized);

	throws.clear();*/

	stencil.onEndSceneEnd(device);

}

void Graphics::drawHitboxArray(const DrawHitboxArrayCallParams& params, BoundingRect* boundingRect, bool clearStencil,
								std::vector<DrawOutlineCallParams>* outlinesOverride) {
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
	rectCombinerInputBoxes.reserve(params.hitboxCount);
	BoundingRect localBoundingRect;
	if (!boundingRect) {
		boundingRect = &localBoundingRect;
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
	drawBoxCall.outlineColor = params.outlineColor;
	drawBoxCall.thickness = 0;

	const Hitbox* hitboxData = params.hitboxData;
	for (int i = params.hitboxCount; i != 0; --i) {
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

		rectCombinerInputBoxes.emplace_back(offX, offX + sizeX, offY, offY + sizeY);

		drawBoxCall.left = offX;
		drawBoxCall.right = offX + sizeX;
		drawBoxCall.top = offY;
		drawBoxCall.bottom = offY + sizeY;

		drawBox(drawBoxCall, boundingRect, true);

		++hitboxData;
	}
	if (clearStencil && allowedToUseStencil) {
		stencil.clearRegion(device, *boundingRect);
	}

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

Graphics::Vertex::Vertex(float x, float y, float z, float rhw, DWORD color)
	: x(x), y(y), z(z), rhw(rhw), color(color) { }

void Graphics::drawOutline(const DrawOutlineCallParams& params) {
	if (!allowedToDrawOutlines) return;
	logOnce(fprintf(logfile, "Called drawOutlines with an outline with %d elements\n", params.count()));
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	if (!screenshotMode) {
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}
	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->SetTexture(0, nullptr);

	D3DXVECTOR3 conv;

	if (params.thickness == 1) {

		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			worldToScreen(D3DXVECTOR3{ (float)elem.x, 0.F, (float)elem.y }, &conv);
			logOnce(fprintf(logfile, "x: %f; y: %f;\n", conv.x, conv.y));
			vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, params.outlineColor);
		}
		vertexArena.push_back(vertexArena.front());

		// Line strip means v1, v2 is a line, v2, v3 is a line, etc. It doesn't close the loop on its own.
		// Second parameter is the number of lines (so you would need numOfLines + 1 vertices).
		device->DrawPrimitiveUP(D3DPT_LINESTRIP, params.count(), &vertexArena.front(), sizeof(Vertex));
	} else {

		for (int outlineIndex = 0; outlineIndex < params.count(); ++outlineIndex) {
			const PathElement& elem = params.getPathElem(outlineIndex);

			worldToScreen(D3DXVECTOR3{ (float)elem.x, 0.F, (float)elem.y }, &conv);
			logOnce(fprintf(logfile, "x: %f; y: %f;\n", conv.x, conv.y));
			vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, params.outlineColor);
			worldToScreen(D3DXVECTOR3{ (float)elem.x + params.thickness * elem.inX, 0.F, (float)elem.y + params.thickness * elem.inY }, &conv);
			vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, params.outlineColor);
		}
		vertexArena.push_back(vertexArena.front());
		vertexArena.push_back(vertexArena[1]);

		// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
		// Second parameter is the number of triangles.
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, params.count() * 2, &vertexArena.front(), sizeof(Vertex));
	}
	vertexArena.clear();
}

void Graphics::worldToScreen(const D3DXVECTOR3& vec, D3DXVECTOR3* out) {
	camera.worldToScreen(device, vec, out);
}

void Graphics::drawPoint(const DrawPointCallParams& params)
{
	if (!allowedToDrawPoints) return;
	D3DXVECTOR3 p{ (float)params.posX, 0.F, (float)params.posY };
	logOnce(fprintf(logfile, "drawPoint called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));

	D3DXVECTOR3 sp;
	worldToScreen(p, &sp);

	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->SetTexture(0, nullptr);


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
	Vertex outline1[] =
	{
		{ sp.x - 4, sp.y - 1, 0.F, 1.F, params.fillColor },
		{ sp.x - 4, sp.y + 2, 0.F, 1.F, params.fillColor },
		{ sp.x + 5, sp.y - 1, 0.F, 1.F, params.fillColor },
		{ sp.x + 5, sp.y + 2, 0.F, 1.F, params.fillColor },
	};

	// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
	// Second parameter is the number of triangles.
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, outline1, sizeof(Vertex));

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
	Vertex outline2[] =
	{
		{ sp.x - 1, sp.y - 4, 0.F, 1.F, params.fillColor },
		{ sp.x - 1, sp.y + 5, 0.F, 1.F, params.fillColor },
		{ sp.x + 2, sp.y - 4, 0.F, 1.F, params.fillColor },
		{ sp.x + 2, sp.y + 5, 0.F, 1.F, params.fillColor },
	};

	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, outline2, sizeof(Vertex));

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
	Vertex vertices[] =
	{
		{ sp.x - 3, sp.y, 0.F, 1.F, params.outlineColor },
		{ sp.x + 4, sp.y, 0.F, 1.F, params.outlineColor },
		{ sp.x, sp.y - 3, 0.F, 1.F, params.outlineColor },
		{ sp.x, sp.y + 4, 0.F, 1.F, params.outlineColor },
	};

	// Linelist means v1, v2 is a line, v3, v4 is a line, etc. Error on uneven vertices
	// Second parameter is the number of lines.
	device->DrawPrimitiveUP(D3DPT_LINELIST, 2, vertices, sizeof(Vertex));
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
	device->CreateStateBlock(D3DSBT_ALL, &oldState);

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

	if (FAILED(device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.F, 0))) {
		logwrap(fputs("Clear failed\n", logfile));
		device->SetRenderTarget(0, gamesRenderTarget);
		gamesRenderTarget = nullptr;
		return false;
	}

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

	screenshotMode = true;
	allowedToDrawFills = true;
	allowedToDrawOutlines = false;
	allowedToUseStencil = false;
	allowedToDrawPoints = false;
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
	screenshotMode = false;
	allowedToDrawFills = true;
	allowedToDrawOutlines = true;
	allowedToUseStencil = true;
	allowedToDrawPoints = true;
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
		unsigned int average = (d.r + d.g + d.b) / 3;
		if (average > d.a) d.a = average;
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

	// Thanks to WorseThanYou for writing this CPU pixel blender
	union Pixel {
		struct { unsigned char r, g, b, a; };
		int value;
	};
	Pixel* gameImagePtr = (Pixel*)&gameImage.front();
	const size_t offLimit = renderTargetDesc.Width * renderTargetDesc.Height * 4;
	for (size_t off = 0; off < offLimit; off += 4)
	{
		Pixel& d = *gameImagePtr;

		d.a ^= 255;
		++gameImagePtr;
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
	if (!takeScreenshotBegin(device)) return;
	// thanks to WorseThanYou for coming up with this box blending sequence
	drawAll();
	logOnce(fputs("drawAll() (for screenshot) call successful\n", logfile));
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	allowedToUseStencil = true;
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	// 1-(1-a)*(1-b) = a+b(1-a)
	device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
	drawAll();
	allowedToDrawOutlines = true;
	allowedToDrawPoints = true;
	allowedToDrawFills = false;
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	drawAll();
	takeScreenshotEnd(device);
	device->SetRenderTarget(0, gamesRenderTarget);
	gamesRenderTarget = nullptr;
	oldState->Apply();
	oldState = nullptr;
}
