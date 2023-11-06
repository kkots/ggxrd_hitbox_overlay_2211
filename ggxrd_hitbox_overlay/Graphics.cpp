#include "pch.h"
#include "Graphics.h"
#include "Detouring.h"
#include "Direct3DVTable.h"
#include "Hitbox.h"
#include "Camera.h"
#include "BoundingRect.h"
#include "logging.h"
#include "pi.h"

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
	graphics.resetHook();
	HRESULT result;
	{
		MutexWhichTellsWhatThreadItsLockedByGuard guard(graphics.orig_ResetMutex);
		result = graphics.orig_Reset(device, pPresentationParameters);
	}
	--detouring.hooksCounter;
	return result;
}

void Graphics::resetHook() {
	stencil.surface = NULL;
	stencil.direct3DSuccess = false;
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

		if (stencil.initialized) {
			device->SetRenderState(D3DRS_STENCILENABLE, TRUE);  // Thanks to WorseThanYou for the idea of using Stenciling
			device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
			device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
		}
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetVertexShader(nullptr);
		device->SetPixelShader(nullptr);
		device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		device->SetTexture(0, nullptr);

		Vertex vertices[] =
		{
			{ sp1.x, sp1.y, 0.F, 1.F, params.fillColor },  // Thanks to WorseThanYou for fixing the RHW value for Intel GPUs (was 0.F, didn't work)
			{ sp2.x, sp2.y, 0.F, 1.F, params.fillColor },
			{ sp3.x, sp3.y, 0.F, 1.F, params.fillColor },
			{ sp4.x, sp4.y, 0.F, 1.F, params.fillColor },
		};

		// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
		// Second parameter is the number of triangles.
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
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
	/*	const Hitbox* hitboxData = nullptr;
	int hitboxCount = 0;
	DrawHitboxArrayParams params{ 0 };
	D3DCOLOR fillColor{ 0 };
	D3DCOLOR outlineColor{ 0 };
	bool operator==(const DrawHitboxArrayCallParams& other) const;
	bool operator!=(const DrawHitboxArrayCallParams& other) const;
		int posX = 0;
	int posY = 0;
	char flip = 1;  // 1 for facing left, -1 for facing right
	int scaleX = 1000;
	int scaleY = 1000;
	int angle = 0;
	int hitboxOffsetX = 0;
	int hitboxOffsetY = 0;
	*/
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
	if (clearStencil) {
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
	logOnce(fprintf(logfile, "Called drawOutlines with an outline with %d elements\n", params.count()));
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
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
	D3DXVECTOR3 p{ (float)params.posX, 0.F, (float)params.posY };
	logOnce(fprintf(logfile, "drawPoint called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));

	D3DXVECTOR3 sp;
	worldToScreen(p, &sp);

	device->SetRenderState(D3DRS_STENCILENABLE, false);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
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
