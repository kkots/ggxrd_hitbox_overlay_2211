#include "pch.h"
#include <stdexcept>
#include <cmath>
#include <vector>
#include <Psapi.h>
#include <detours.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "atlbase.h"
#include "rect_combiner.h"
#include <strsafe.h>
#include <algorithm>

// TODO: Slayer's ground throw doesn't track his invulnerability during throw fully
// TODO: Raven's Air Scharf Kugel hits twice but doesn't show hitbox on the second hit
// TODO: Air Tech doesn't show as strike invul
// TODO: Pushboxes, throw invulnerability and throw boxes
// TODO: Missing OTG flag
// TODO: Don't show counterhit if strike invul
// TODO: Show counterhit for longer after a hit connects in counterhit state
// TODO: Frame-by-frame stepping, maybe in some other function
// TODO: (impossible) EndScene and Present get called in a different thread from where the game logic is happening,
//       hence there are artifacts when boxes are drawn twice onto a frame in different states or the boxes are one frame ahead of what's on the frame.

// Original was made in 2016 by Altimor. Link to source: http://www.dustloop.com/forums/index.php?/forums/topic/12495-xrd-pc-hitbox-overlay-mod/
// This version is adapted for Guilty Gear Xrd version 2211 (as of 8'th October 2023) with 90% of the features removed as of now (most stuff is not re-found yet)
// 
// Dependencies:
// You need to download and build detours library: https://github.com/microsoft/Detours Follow instructions in the repo
// If you're missing d3dx9.h you could get it from here: https://github.com/apitrace/dxsdk/blob/master/Include/d3dx9.h
//
// Note:
// You need to compile without UNICODE set. Right-click the project in `Solution Explorer` and go to
// `Properties` -> `Advanced` -> `Character Set`. Select `Use Multi-Byte Character Set`.

#define PI 3.14159F

const float outlineThickness = 3000.F;
CComPtr<IDirect3DSurface9> stencilSurface = NULL;  // Thanks to WorseThanYou for telling to use CComPtr class
bool direct3DError = false;
bool direct3DSuccess = false;
char** dev_vtable = nullptr;
const float coord_coefficient = 0.42960999705207F;
char **asw_engine = nullptr;
char **game_data_ptr = nullptr;
char *trainingModeMenuOpenRef = nullptr;
char *versusModeMenuOpenRef = nullptr;
char *isIKCutscenePlaying = nullptr;
std::vector<std::pair<PVOID*, PVOID>> thingsToUndetourAtTheEnd;


DWORD thisProcessId = 0;
HWND thisProcessWindow = NULL;

BOOL CALLBACK EnumWindowsFindMyself(HWND hwnd, LPARAM lParam) {
	DWORD windsProcId = 0;
	DWORD windsThreadId = GetWindowThreadProcessId(hwnd, &windsProcId);
	if (windsProcId == thisProcessId) {
		thisProcessWindow = hwnd;
		return FALSE;
	}
	return TRUE;
}

#ifdef LOG_PATH
FILE *logfile = NULL;
#define logwrap(things) \
{ \
	errno_t err = _wfopen_s(&logfile, LOG_PATH, L"at+"); \
	if (err == 0 && logfile) { \
		things; \
		fclose(logfile); \
	} \
	logfile = NULL; \
}

bool didWriteOnce = false;
int msgLimit = 1000;
#define log(things) if (msgLimit>=0 && !didWriteOnce) { \
	errno_t err = _wfopen_s(&logfile, LOG_PATH, L"at+"); \
	if (err == 0 && logfile) { \
		things; \
		fclose(logfile); \
	} \
	logfile = NULL; \
} \
msgLimit--
#else
#define logwrap(things)
#define log(things)
#endif

//using cast_t = const void*(*)(const void*);
//cast_t cast_REDGameInfo_Battle; // found something like PTR_s_AREDGameInfo_BattleexecRenderUpd_019cd5e0, but no cast

//using get_pushbox_t = int(__thiscall*)(const void*);
//get_pushbox_t get_pushbox_x, get_pushbox_y, get_pushbox_bottom;

//using is_push_active_t = bool(__thiscall*)(const void*);
//is_push_active_t is_push_active;

using get_pos_t = int(__thiscall*)(const void*);
get_pos_t get_pos_x, get_pos_y;

struct Hitbox
{
	int type;
	float offx;
	float offy;
	float sizex;
	float sizey;
};

struct RotatedPathElement {
	float x = 0.F;
	float y = 0.F;
	float inX = 0.F;
	float inY = 0.F;
};

struct DrawHitboxArrayParams {
	int posX = 0;
	int posY = 0;
	char flip = 1;
	int scaleX = 1000;
	int scaleY = 1000;
	float cosAngle = 1.F;
	float sinAngle = 0.F;
};

struct DrawHitboxArrayCallParams {
	Hitbox* hitbox_data = nullptr;
	int hitboxCount = 0;
	DrawHitboxArrayParams params{0};
	D3DCOLOR fillColor{0};
	D3DCOLOR outlineColor{0};
};

struct DrawPointCallParams {
	int posX = 0;
	int posY = 0;
	D3DCOLOR fillColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	D3DCOLOR outlineColor = D3DCOLOR_ARGB(255, 0, 0, 0);
};

std::vector<RotatedPathElement> allRotatedPathElems;

struct DrawOutlineCallParams {
	int outlineStartAddr = 0;
	int outlineCount = 0;
	D3DCOLOR outlineColor{0};
};

void angle_vectors(
	const float p, const float y, const float r,
	float *forward, float *right, float *up)
{
	const auto sp = sin(-p);
	const auto cp = cos(-p);

	const auto sy = sin(y);
	const auto cy = cos(y);

	const auto sr = sin(-r);
	const auto cr = cos(-r);

	if (forward != nullptr)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right != nullptr)
	{
		right[0] = -1 * sr * sp * cy + -1 * cr * -sy;
		right[1] = -1 * sr * sp * sy + -1 * cr * cy;
		right[2] = -1 * sr * cp;
	}
	if (up != nullptr)
	{
		up[0] = cr * sp * cy + -sr * -sy;
		up[1] = cr * sp * sy + -sr * cy;
		up[2] = cr * cp;
	}
}

float vec_dot(float *a, float *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

struct AngleVectors {
	bool isSet = false;
	float forward[3] { 0.F };
	float right[3] { 0.F };
	float up[3] { 0.F };
	D3DXVECTOR3 pos { 0.F, 0.F, 0.F };
	float clipX = 0.F;
	float clipY = 0.F;
	float clipXHalf = 0.F;
	float clipYHalf = 0.F;
	float divisor = 0.F;
} angleVectors;

// Arcsys engine to UE coords

float convert_coord(const int in)
{
	return (float)(in) / 1000.F * /*!*//*   *(float*)(*asw_engine + 0x3EA724)  */ coord_coefficient;  // not found, replacement value
}

float convert_coord(const float in)
{
	return in / 1000.F * /*!*//*   *(float*)(*asw_engine + 0x3EA724)   */ coord_coefficient;  // not found, replacement value
}

void world_to_screen(IDirect3DDevice9 *device, const D3DXVECTOR3 &in, D3DXVECTOR3 *out)
{
	if (!angleVectors.isSet) {
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);

		/*const auto *game = *game_ptr;
		const auto *world = *(char**)(game + 0x50);
		const auto *world_info = **(char***)(world + 0x3C);
		const auto *game_info = *(char**)(world_info + 0x4A8);
		const auto *camera = *(char**)(game_info+0x428);*/
		// I find the camera differently, I go *(*asw_engine + 0x22e62c)
		const char *camera = *(char**)(*asw_engine + 0x22e62c);

		angleVectors.pos.x = *(float*)(camera + /*! 0x384*/0x3C8);  // moved to 0x3C8, the rest follows
		angleVectors.pos.y = *(float*)(camera + /*! 0x388*/0x3CC);
		angleVectors.pos.z = *(float*)(camera + /*! 0x38C*/0x3D0);

		const float clipX = (float)(viewport.Width);
		const float clipY = (float)(viewport.Height);

		const auto pitch = (float)(*(int*)(camera + /*! 0x390*/0x3D4)) / 32768.F * PI;
		const auto yaw = (float)(*(int*)(camera + /*! 0x394*/0x3D8)) / 32768.F * PI;
		const auto roll = (float)(*(int*)(camera + /*! 0x398*/0x3DC)) / 32768.F * PI;

		const float fov = *(float*)(camera + /*! 0x39C*/0x3E0);

		angle_vectors(pitch, yaw, roll, angleVectors.forward, angleVectors.right, angleVectors.up);
		
		angleVectors.clipXHalf = (clipX / 2.F);
		angleVectors.clipYHalf = (clipY / 2.F);
		angleVectors.divisor = (angleVectors.clipXHalf / tan(fov * PI / 360.F));

		angleVectors.isSet = true;
	}

	D3DXVECTOR3 inConverted { convert_coord(in.x), 0.F, convert_coord(in.z) };

	D3DXVECTOR3 relative_pos;
	D3DXVec3Subtract(&relative_pos, &inConverted, &angleVectors.pos);

	out->x = vec_dot(relative_pos, angleVectors.right);
	out->y = vec_dot(relative_pos, angleVectors.up);
	out->z = vec_dot(relative_pos, angleVectors.forward);

	out->x = floorf(angleVectors.clipXHalf - out->x * angleVectors.divisor / out->z + .5F);
	out->y = floorf(angleVectors.clipYHalf - out->y * angleVectors.divisor / out->z + .5F);
	out->z = 0.F;
}

void initializeStencil(IDirect3DDevice9 *device, bool& stencilInitialized) {
	if (direct3DError) return;
	if (!direct3DSuccess) {
	
		CComPtr<IDirect3D9> d3d = NULL;
		D3DCAPS9 caps;
		D3DDISPLAYMODE d3dDisplayMode{0};

		if (FAILED(device->GetDirect3D(&d3d)) || !d3d) {
			log(fputs("GetDirect3D failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("GetDirect3D succeeded\n", logfile));
		}

		SecureZeroMemory(&caps, sizeof(caps));
		if (FAILED(device->GetDeviceCaps(&caps))) {
			log(fputs("GetDeviceCaps failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("GetDeviceCaps succeeded\n", logfile));
		}

		if (FAILED(device->GetDisplayMode(0, &d3dDisplayMode))) {
			log(fputs("GetDisplayMode failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("GetDisplayMode succeeded\n", logfile));
		}

		if(FAILED(d3d->CheckDeviceFormat(
						caps.AdapterOrdinal,
						caps.DeviceType,  
						d3dDisplayMode.Format,  
						D3DUSAGE_DEPTHSTENCIL, 
						D3DRTYPE_SURFACE,
						D3DFMT_D24S8))) {
			log(fputs("CheckDeviceFormat failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("CheckDeviceFormat succeeded\n", logfile));
		}

		D3DSURFACE_DESC renderTargetDesc;
		SecureZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
		DWORD renderTargetIndex = 0;
		for (   ;  ; ++renderTargetIndex) {
			CComPtr<IDirect3DSurface9> renderTarget;
			HRESULT getRenderTargetResult = device->GetRenderTarget(renderTargetIndex, &renderTarget);
			if (getRenderTargetResult == D3DERR_NOTFOUND) {
				break;
			}
			if (FAILED(getRenderTargetResult)) {
				log(fputs("GetRenderTarget failed\n", logfile));
				direct3DError = true;
				return;
			}

			if (FAILED(renderTarget->GetDesc(&renderTargetDesc))) {
				log(fputs("GetDesc failed\n", logfile));
				direct3DError = true;
				return;
			}
		}
		log(fprintf(logfile, "GetRenderTargetResult returned %d targets\n", renderTargetIndex));

		if (FAILED(d3d->CheckDepthStencilMatch(caps.AdapterOrdinal,
					caps.DeviceType,  
					d3dDisplayMode.Format,
					renderTargetDesc.Format,
					D3DFMT_D24S8))) {
			log(fputs("CheckDepthStencilMatch failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("CheckDepthStencilMatch succeeded\n", logfile));
		}

		if (FAILED(device->CreateDepthStencilSurface(
				renderTargetDesc.Width,
				renderTargetDesc.Height,
				D3DFMT_D24S8,
				renderTargetDesc.MultiSampleType,
				renderTargetDesc.MultiSampleQuality,
				TRUE,
				&stencilSurface,
				NULL))) {
			log(fputs("CreateDepthStencilSurface failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("CreateDepthStencilSurface succeeded\n", logfile));
		}

		direct3DSuccess = true;
	}

	if (!stencilInitialized) {
		if (FAILED(device->SetDepthStencilSurface(stencilSurface))) {
			log(fputs("SetDepthStencilSurface failed\n", logfile));
			direct3DError = true;
			return;
		} else {
			log(fputs("SetDepthStencilSurface succeeded\n", logfile));
		}
		device->Clear(0, NULL, D3DCLEAR_STENCIL, D3DCOLOR{}, 1.f, 0);
		
		stencilInitialized = true;
	
	}
}

struct Vertex {
	float x, y, z, rhw;
	DWORD color;
	Vertex(float x, float y, float z, float rhw, DWORD color)
	      : x(x), y(y), z(z), rhw(rhw), color(color) { }
};

std::vector<Vertex> vertexArena;

void draw_angled_outline(IDirect3DDevice9 *device,
	const D3DXVECTOR3 &p1,
	const D3DXVECTOR3 &p2,
	const D3DXVECTOR3 &p3,
	const D3DXVECTOR3 &p4,
	D3DCOLOR outlineColor) {
	
}

void draw_outline(IDirect3DDevice9 *device, const DrawOutlineCallParams& params) {
	log(fprintf(logfile, "Called draw_outlines with an outline with %d elements\n", params.outlineCount));
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->SetTexture(0, nullptr);

	D3DXVECTOR3 conv;
	
	for (auto elem = allRotatedPathElems.begin() + params.outlineStartAddr;
			elem != allRotatedPathElems.begin() + params.outlineStartAddr + params.outlineCount;
			++elem) {
		world_to_screen(device, D3DXVECTOR3{ elem->x, 0.F, elem->y }, &conv);
		log(fprintf(logfile, "x: %f; y: %f;\n", conv.x, conv.y));
		vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, params.outlineColor);
		world_to_screen(device, D3DXVECTOR3{ elem->x + outlineThickness * elem->inX,
		                                     0.F,
		                                     elem->y + outlineThickness * elem->inY },
			            &conv);
		vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, params.outlineColor);
	}
	vertexArena.push_back(vertexArena.front());
	vertexArena.push_back(vertexArena[1]);

	// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, params.outlineCount * 2, &vertexArena.front(), sizeof(Vertex));
	vertexArena.clear();
}

struct FloatRect {
	float left = 0.F;
	float right = 0.F;
	float top = 0.F;
	float bottom = 0.F;
} nullFloatRect;

void draw_rect(
	IDirect3DDevice9 *device,
	const D3DXVECTOR3 &p1,
	const D3DXVECTOR3 &p2,
	const D3DXVECTOR3 &p3,
	const D3DXVECTOR3 &p4,
	D3DCOLOR fillColor,
	FloatRect& bounds,
	bool& stencilInitialized) {

	if (p1.x == p2.x && p2.x == p3.x && p3.x == p4.x || ((fillColor >> 24) & 0xff) == 0)
		return;
	
	initializeStencil(device, stencilInitialized);

	D3DXVECTOR3 sp1, sp2, sp3, sp4;
	world_to_screen(device, p1, &sp1);
	world_to_screen(device, p2, &sp2);
	world_to_screen(device, p3, &sp3);
	world_to_screen(device, p4, &sp4);
		
	log(fprintf(logfile,
		"Box. Red: %u; Green: %u; Blue: %u; Alpha: %u;\n",
		(fillColor >> 16) & 0xff, (fillColor >> 8) & 0xff, fillColor & 0xff, (fillColor >> 24) & 0xff));
	log(fprintf(logfile,
		"sp1 { x: %f; y: %f; }; sp2 { x: %f; y: %f; }; sp3 { x: %f; y: %f; }; sp4 { x: %f; y: %f; }\n",
		sp1.x, sp1.y, sp2.x, sp2.y, sp3.x, sp3.y, sp4.x, sp4.y));

	if (sp1.x < bounds.left) bounds.left = sp1.x;
	if (sp2.x < bounds.left) bounds.left = sp2.x;
	if (sp3.x < bounds.left) bounds.left = sp3.x;
	if (sp4.x < bounds.left) bounds.left = sp4.x;
	
	if (sp1.x > bounds.right) bounds.right = sp1.x;
	if (sp2.x > bounds.right) bounds.right = sp2.x;
	if (sp3.x > bounds.right) bounds.right = sp3.x;
	if (sp4.x > bounds.right) bounds.right = sp4.x;
	
	if (sp1.y < bounds.top) bounds.top = sp1.y;
	if (sp2.y < bounds.top) bounds.top = sp2.y;
	if (sp3.y < bounds.top) bounds.top = sp3.y;
	if (sp4.y < bounds.top) bounds.top = sp4.y;
	
	if (sp1.y > bounds.bottom) bounds.bottom = sp1.y;
	if (sp2.y > bounds.bottom) bounds.bottom = sp2.y;
	if (sp3.y > bounds.bottom) bounds.bottom = sp3.y;
	if (sp4.y > bounds.bottom) bounds.bottom = sp4.y;
	
	if (stencilInitialized) {
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
		{ sp1.x, sp1.y, 0.F, 1.F, fillColor },  // Thanks to WorseThanYou for fixing the RHW value for Intel GPUs (was 0.F, didn't work)
		{ sp2.x, sp2.y, 0.F, 1.F, fillColor },
		{ sp3.x, sp3.y, 0.F, 1.F, fillColor },
		{ sp4.x, sp4.y, 0.F, 1.F, fillColor },
	};

	// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));

	/* Old code for drawing the outline
	vertex outline[] =
	{
		{ sp1.x, sp1.y, 0.F, 1.F, outlineColor },
		{ sp2.x, sp2.y, 0.F, 1.F, outlineColor },
		{ sp4.x, sp4.y, 0.F, 1.F, outlineColor },
		{ sp3.x, sp3.y, 0.F, 1.F, outlineColor },
		{ sp1.x, sp1.y, 0.F, 1.F, outlineColor },
	};

	device->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, outline, sizeof(vertex));*/
}

void draw_point(IDirect3DDevice9 *device, const DrawPointCallParams& params)
{
	D3DXVECTOR3 p { (float)params.posX, 0.F, (float)params.posY };
	log(fprintf(logfile, "draw_point called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));

	D3DXVECTOR3 sp;
	world_to_screen(device, p, &sp);
	
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
	
	// Linelist means every two vertices is a line. error on uneven vertices
	device->DrawPrimitiveUP(D3DPT_LINELIST, 2, vertices, sizeof(Vertex));
}



/*auto gif_mode = false;
bool gif_toggle_held = false;

auto nograv_mode = false;
auto nograv_toggle_held = false;*/

/*using update_darken_t = void(__thiscall*)(char*);
update_darken_t orig_update_darken;
void __fastcall hook_update_darken(char *thisptr, void*)
{
	if (gif_mode)
	{
		*(float*)(thisptr + 0x3E83CC) = -1.F;
		*(float*)(thisptr + 0x3E83D8) = 0.F;
	}

	orig_update_darken(thisptr);
}*/

// won't need this hook if not using gif mode
/*using update_camera_t = void(__thiscall*)(void*, void*, void*);
update_camera_t orig_update_camera;
void __fastcall hook_update_camera(void *thisptr, void*, void *a2, void *a3)
{
	if (!gif_mode || *asw_engine == nullptr)
		return orig_update_camera(thisptr, a2, a3);

	const auto ent_slots = (char**)(*asw_engine + 0xC4);

	const auto posx = convert_coord(get_pos_x(ent_slots[0]));
	const auto posy = convert_coord(get_pos_y(ent_slots[0]));

	const auto *deref = *(char**)(a2);
	*(float*)(deref + 0x54) = posx;
	*(float*)(deref + 0x58) = 540.F;
	*(float*)(deref + 0x5C) = posy + 106.4231F;

	orig_update_camera(thisptr, a2, a3);
}*/

/*using update_hud_t = void(__thiscall*)(void*);
update_hud_t orig_update_hud;
void __fastcall hook_update_hud(char *thisptr)
{
	// bShowHud is in a bitfield
	auto *bShowHud = (int*)(thisptr + 0x1D8);
	*bShowHud &= ~2;
	if (!gif_mode)
		*bShowHud |= 2;

	orig_update_hud(thisptr);
}*/

void collect_hitboxes(char* const asw_data,
	const bool active,
	std::vector<DrawHitboxArrayCallParams>* const hurtboxes,
	std::vector<DrawHitboxArrayCallParams>* const hitboxes,
	std::vector<DrawPointCallParams>* const points)
{

	DrawHitboxArrayParams params;
	/*{
	float posx = 0.F;
	float posy = 0.F;
	float flip = 1.F;
	float scale_x = 1000.F;
	float scale_y = 1000.F;
	float ca = 1.F;
	float sa = 0.F;*/

	log(fprintf(logfile, "pox_x: %d\n", get_pos_x(asw_data)));
	log(fprintf(logfile, "pox_y: %d\n", get_pos_y(asw_data)));
	params.posX = get_pos_x(asw_data);
	params.posY = get_pos_y(asw_data);
	log(fprintf(logfile, "converted posx: %d\n", params.posX));
	log(fprintf(logfile, "converted posy: %d\n", params.posY));
	params.flip = *(int*)(asw_data + /*! 0x23C*/0x248) == 1 ? 1 : -1; // is this facing? it moved to 0x248
	log(fprintf(logfile, "flip (original): %d; flip: %d\n", *(int*)(asw_data + 0x248), (int)params.flip));

	const bool doingAThrow = (*(unsigned int*)(asw_data + 0x460) & 0xFF) == 0x8C;
	const unsigned int flagsField = *(unsigned int*)(asw_data + 0x23C);
	const bool gettingThrown = (*(unsigned int*)(asw_data + 0x43C) & 0xFF) != 0
		&& (flagsField & 0x800)
		&& (flagsField & 0x40000);
	const auto otg = false;//(*(int*)(asw_data + 0x2410) & 0x800000) != 0;  // not found yet
	const auto invuln_frames = *(int*)(asw_data + /*! 0x964*/0x9A0);
	const auto invuln_flags = *(char*)(asw_data + /*! 0x230*/0x238);
	const auto strike_invuln = invuln_frames > 0 || (invuln_flags & 16) || (invuln_flags & 64);
	const auto throw_invuln = invuln_frames > 0 || (invuln_flags & 32) || (invuln_flags & 64) || otg;
	const auto counterhit = (*(int*)(asw_data + /*! 0x22C*/0x234) & 256) != 0;  // Thanks to WorseThanYou for finding this
	// Color scheme:
	// light blue - hurtbox on counterhit
	// green - hurtbox on not counterhit
	// red - hitbox
	// yellow - pushbox
	// blue - throwbox


	// Draw pushbox and throw box
	/*if (is_push_active(asw_data))
	{
		const auto pushbox_top = convert_coord(get_pushbox_y(asw_data));
		const auto pushbox_bottom = convert_coord(get_pushbox_bottom(asw_data));

		const auto pushbox_x = get_pushbox_x(asw_data);
		const auto pushbox_back = convert_coord(pushbox_x / 2);
		const auto front_offset = *(int*)(asw_data + 0x300);
		const auto pushbox_front = convert_coord(pushbox_x / 2 + front_offset);

		draw_rect(
			device,
			D3DXVECTOR3(posx - pushbox_back * flip, 0.F, posy + pushbox_top),
			D3DXVECTOR3(posx - pushbox_back * flip, 0.F, posy - pushbox_bottom),
			D3DXVECTOR3(posx + pushbox_front * flip, 0.F, posy + pushbox_top),
			D3DXVECTOR3(posx + pushbox_front * flip, 0.F, posy - pushbox_bottom),
			D3DCOLOR_ARGB(throw_invuln ? 0 : 64, 255, 255, 0),
			D3DCOLOR_ARGB(255, 255, 255, 0),
			nullFloatRect,
			stencilInitialized);
	}*/

	log(fprintf(logfile, "hp: %d\n", *(int*)(asw_data + 0x9cc)));

	auto* const hurtbox_data = *(Hitbox**)(asw_data + 0x58);  // this address is correct and this assumes asw_data is the player
	if (hurtbox_data == nullptr)
		return;

	auto* const hitbox_data = *(Hitbox**)(asw_data + 0x5C);
	if (hurtbox_data == nullptr)
		return;


	const auto hurtbox_count = *(int*)(asw_data + 0xA0); // this address is correct
	const auto hitbox_count = *(int*)(asw_data + 0xA4);

	log(fprintf(logfile, "hurtbox_count: %d; hitbox_count: %d\n", hurtbox_count, hitbox_count));

	const auto angle = (float)(*(int*)(asw_data + /*!*/0x258)) / 1000.F;  // confirmed to be the angle
	params.cosAngle = cos(angle * PI / 180.F);
	params.sinAngle = sin(angle * PI / 180.F);
	log(fprintf(logfile, "angle: %d\n", *(int*)(asw_data + /*!*/0x258)));

	// Thanks to jedpossum on dustloop for these offsets
	params.scaleX = *(int*)(asw_data + /*!*/0x264); // moved to 0x264 and it's more complicated, there's a function but what if it breaks on update...
	params.scaleY = *(int*)(asw_data + /*!*/0x268); // moved to 0x268 and it's more complicated, there's a function but what if it breaks on update...
	// this will probably all break anyway on updates...
	log(fprintf(logfile, "scale_x: %d; scale_y: %d\n", *(int*)(asw_data + /*!*/0x264), *(int*)(asw_data + /*!*/0x268)));

	DrawHitboxArrayCallParams callParams;

	if (hurtboxes) {
		callParams.hitbox_data = hurtbox_data;
		callParams.hitboxCount = hurtbox_count;
		callParams.params = params;
		callParams.fillColor = D3DCOLOR_ARGB(strike_invuln
			|| gettingThrown
			|| doingAThrow ? 0 : 64, 0, 255, counterhit ? 255 : 0);
		callParams.outlineColor = D3DCOLOR_ARGB(255, 0, 255, counterhit ? 255 : 0);
		hurtboxes->push_back(callParams);
	}

	if (hitboxes && active && !doingAThrow) {
		callParams.hitbox_data = hitbox_data;
		callParams.hitboxCount = hitbox_count;
		callParams.params = params;
		callParams.fillColor = D3DCOLOR_ARGB(64, 255, 0, 0);
		callParams.outlineColor = D3DCOLOR_ARGB(255, 255, 0, 0);
		hitboxes->push_back(callParams);
	}

	if (points) {
		DrawPointCallParams pointCallParams;
		pointCallParams.posX = params.posX;
		pointCallParams.posY = params.posY;
		points->push_back(pointCallParams);
	}
}

using hit_detection_t = BOOL(__thiscall*)(void*, void*, int, int, int*, int*);
hit_detection_t orig_hit_detection;

struct HitboxThatHitStruct {
	char* attacker;
	int attackerTeam;
	DrawHitboxArrayCallParams hitboxes;
	int counter;
};
std::vector<HitboxThatHitStruct> hitboxesThatHit;

// this is for Millia's Tandem Top because it disappears as soon as it hits and before that it's inactive,
// so basically, we just never get to see its hitbox if it hits frame 1. This solves that
class HitDetectionHookHelp {
public:
	BOOL hook_hit_detection(void* defender, int attackerHitboxIndex, int defenderHitboxIndex, int* intersectionX, int* intersectionY) {
		// this  ==  attacker
		BOOL hitResult = orig_hit_detection(this, defender, attackerHitboxIndex, defenderHitboxIndex, intersectionX, intersectionY);
		if (hitResult) {
			std::vector<DrawHitboxArrayCallParams> theHitbox;
			collect_hitboxes((char*)this, true, nullptr, &theHitbox, nullptr);
			if (!theHitbox.empty()) {
				hitboxesThatHit.push_back({ (char*)this, *(int*)((char*)this + 0x40), theHitbox.front(), 5 });
			}
		}
		return hitResult;
	}
};

struct throw_info
{
	bool pushbox_check;
	float min_pushbox_x;
	float max_pushbox_x;

	bool x_check;
	float min_origin_x;
	float max_origin_x;

	bool y_check;
	float min_origin_y;
	float max_origin_y;
};

/*std::vector<throw_info> throws;

using can_throw_t = bool(__thiscall*)(void*, void*);
can_throw_t orig_can_throw;
bool __fastcall hook_can_throw(char *thisptr, void*, void *target)
{
	const auto posx = convert_coord(get_pos_x(thisptr));
	const auto posy = convert_coord(get_pos_y(thisptr));
	const auto flip = *(int*)(thisptr + 0x23C) == 1 ? 1.F : -1.F;

	throw_info ti;

	const auto min_pushbox_dist = *(int*)(thisptr + 0x468);
	ti.pushbox_check = min_pushbox_dist > 0;
	if (ti.pushbox_check)
	{
		const auto pushbox_x = get_pushbox_x(thisptr);
		const auto front_offset = *(int*)(thisptr + 0x300);
		const auto pushbox_back = convert_coord(pushbox_x / 2);
		const auto pushbox_front = convert_coord(pushbox_x / 2 + front_offset);
		const auto throwbox_back = pushbox_back + convert_coord(*(int*)(thisptr + 0x468));
		const auto throwbox_front = pushbox_front + convert_coord(*(int*)(thisptr + 0x468));
		ti.min_pushbox_x = posx - throwbox_back * flip;
		ti.max_pushbox_x = posx + throwbox_front * flip;
	}

	const auto min_x = *(int*)(thisptr + 0x460);
	const auto max_x = *(int*)(thisptr + 0x458);
	ti.x_check = max_x > min_x;
	if (ti.x_check)
	{
		ti.min_origin_x = posx + convert_coord(min_x) * flip;
		ti.max_origin_x = posx + convert_coord(max_x) * flip;
	}

	const auto min_y = *(int*)(thisptr + 0x464);
	const auto max_y = *(int*)(thisptr + 0x45C);
	ti.y_check = max_y > min_y;
	if (ti.y_check)
	{
		ti.min_origin_y = posy + convert_coord(min_y);
		ti.max_origin_y = posy + convert_coord(max_y);
	}

	throws.push_back(ti);

	return orig_can_throw(thisptr, target);
}*/

std::vector<RectCombiner::Polygon> rectCombinerInputBoxes;
std::vector<std::vector<RectCombiner::PathElement>> rectCombinerOutlines;

std::vector<RotatedPathElement> rotatedOutline;

void draw_hitbox_array(IDirect3DDevice9 *device,
                       DrawHitboxArrayCallParams params,
					   std::vector<DrawOutlineCallParams>& pendingOutlines,
					   bool& stencilInitialized) {
	if (!params.hitboxCount) return;

	rectCombinerInputBoxes.reserve(params.hitboxCount);
	FloatRect bounds { -1.F, 99999.F, -1.F, 99999.F };

	Hitbox* hitbox_data = params.hitbox_data;
	for (int i = params.hitboxCount; i != 0; --i) {
		log(fprintf(logfile, "drawing box %d\n", params.hitboxCount - i));
		const auto type = *(int*)(hitbox_data);

		const int box_x = (int)(hitbox_data->offx * params.params.scaleX);
		const int box_y = (int)(hitbox_data->offy * params.params.scaleY);
		const int box_width = (int)(hitbox_data->sizex * params.params.scaleX);
		const int box_height = (int)(hitbox_data->sizey * params.params.scaleY);

		const int x1 = box_x;
		const int y1 = box_y;

		const int x2 = box_x;
		const int y2 = box_y + box_height;

		const int x3 = box_x + box_width;
		const int y3 = box_y;
		
		const int x4 = box_x + box_width;
		const int y4 = box_y + box_height;

		rectCombinerInputBoxes.emplace_back(box_x, box_x + box_width, box_y, box_y + box_height);
		
		D3DXVECTOR3 v1 { params.params.posX + (x1 * params.params.cosAngle - y1 * params.params.sinAngle) * params.params.flip,
		                0.F,
						params.params.posY - (y1 * params.params.cosAngle + x1 * params.params.sinAngle) };
		D3DXVECTOR3 v2 { params.params.posX + (x2 * params.params.cosAngle - y2 * params.params.sinAngle) * params.params.flip,
		                0.F,
						params.params.posY - (y2 * params.params.cosAngle + x2 * params.params.sinAngle) };
		D3DXVECTOR3 v3 { params.params.posX + (x3 * params.params.cosAngle - y3 * params.params.sinAngle) * params.params.flip,
		                0.F,
						params.params.posY - (y3 * params.params.cosAngle + x3 * params.params.sinAngle) };
		D3DXVECTOR3 v4 { params.params.posX + (x4 * params.params.cosAngle - y4 * params.params.sinAngle) * params.params.flip,
		                0.F,
						params.params.posY - (y4 * params.params.cosAngle + x4 * params.params.sinAngle) };

		draw_rect(
			device,
			v1,
			v2,
			v3,
			v4,
			params.fillColor,
			bounds,
			stencilInitialized);

		++hitbox_data;
	}

	if (stencilInitialized) {
		D3DRECT rect{(LONG)bounds.left - 1, (LONG)bounds.top - 1, (LONG)bounds.right + 1, (LONG)bounds.bottom + 1};
		device->Clear(1, &rect, D3DCLEAR_STENCIL, D3DCOLOR{}, 1.f, 0);
	}

	RectCombiner::getOutlines(rectCombinerInputBoxes, rectCombinerOutlines);
	rectCombinerInputBoxes.clear();
	for (const std::vector<RectCombiner::PathElement>& outline : rectCombinerOutlines) {
		rotatedOutline.resize(outline.size());
		auto it = rotatedOutline.begin();
		for (const RectCombiner::PathElement& path : outline) {
			it->x = (float)params.params.posX + params.params.flip * (
				(float)path.x * params.params.cosAngle - (float)path.y * params.params.sinAngle
			);
			it->y = (float)params.params.posY - (
				(float)path.y * params.params.cosAngle + (float)path.x * params.params.sinAngle
			);
			it->inX = params.params.flip * (
				(float)path.xDir() * params.params.cosAngle - (float)path.yDir() * params.params.sinAngle
			);
			it->inY = -((float)path.yDir() * params.params.cosAngle + (float)path.xDir() * params.params.sinAngle);
			++it;
		}
		DrawOutlineCallParams drawOutlineCallParams;
		size_t oldSize = allRotatedPathElems.size();
		drawOutlineCallParams.outlineStartAddr = oldSize;
		allRotatedPathElems.resize(oldSize + rotatedOutline.size());
		auto allRotatedPathElemsIter = allRotatedPathElems.begin() + oldSize;
		for (auto rotatedOutlineIter = rotatedOutline.cbegin(); rotatedOutlineIter != rotatedOutline.cend(); ++rotatedOutlineIter) {
			*allRotatedPathElemsIter = *rotatedOutlineIter;
			++allRotatedPathElemsIter;
		}
		drawOutlineCallParams.outlineCount = rotatedOutline.size();
		drawOutlineCallParams.outlineColor = params.outlineColor;
		pendingOutlines.push_back(drawOutlineCallParams);
	}
}

/*void draw_throw(IDirect3DDevice9 *device, const throw_info &ti, bool& stencilInitialized)
{
	float ymin, ymax;
	if (ti.y_check)
	{
		ymin = ti.min_origin_y;
		ymax = ti.max_origin_y;
	}
	else
	{
		ymin = -10000.F;
		ymax = 10000.F;
	}

	if (ti.pushbox_check)
	{
		draw_rect(
			device,
			D3DXVECTOR3(ti.min_pushbox_x, 0.F, ymax),
			D3DXVECTOR3(ti.min_pushbox_x, 0.F, ymin),
			D3DXVECTOR3(ti.max_pushbox_x, 0.F, ymax),
			D3DXVECTOR3(ti.max_pushbox_x, 0.F, ymin),
			D3DCOLOR_ARGB(64, 0, 0, 255),
			D3DCOLOR_ARGB(255, 0, 0, 255),
			nullFloatRect,
			stencilInitialized);
	}

	if (ti.x_check)
	{
		draw_rect(
			device,
			D3DXVECTOR3(ti.min_pushbox_x, 0.F, ymax),
			D3DXVECTOR3(ti.min_pushbox_x, 0.F, ymin),
			D3DXVECTOR3(ti.max_pushbox_x, 0.F, ymax),
			D3DXVECTOR3(ti.max_pushbox_x, 0.F, ymin),
			D3DCOLOR_ARGB(64, 128, 0, 255),
			D3DCOLOR_ARGB(255, 128, 0, 255),
			nullFloatRect,
			stencilInitialized);
	}
}*/

using Reset_t = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS *pPresentationParameters);
Reset_t orig_Reset;
HRESULT __stdcall hook_Reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *pPresentationParameters) {
	stencilSurface = NULL;
	direct3DSuccess = false;
	return orig_Reset(device, pPresentationParameters);
}

enum GameModes {
	GAME_MODE_DEBUG_BATTLE,
	GAME_MODE_ADVERTISE,
	GAME_MODE_ARCADE,
	GAME_MODE_MOM,
	GAME_MODE_SPARRING,
	GAME_MODE_VERSUS,
	GAME_MODE_TRAINING,
	GAME_MODE_RANNYU_VERSUS,
	GAME_MODE_EVENT,
	GAME_MODE_STORY,
	GAME_MODE_DEGITALFIGURE,
	GAME_MODE_MAINMENU,
	GAME_MODE_TUTORIAL,
	GAME_MODE_CHALLENGE,
	GAME_MODE_KENTEI,
	GAME_MODE_NETWORK,
	GAME_MODE_REPLAY,
	GAME_MODE_FISHING,
	GAME_MODE_UNDECIDED,
	GAME_MODE_INVALID
};

void printDetourTransactionBeginError(LONG err) {
	if (err == ERROR_INVALID_OPERATION) {
		logwrap(fputs("DetourTransactionBegin: ERROR_INVALID_OPERATION: A pending transaction already exists.\n", logfile));
	} else if (err  != NO_ERROR) {
		logwrap(fprintf(logfile, "DetourTransactionBegin: %d\n", err));
	}
}

void printDetourUpdateThreadError(LONG err) {
	if (err == ERROR_NOT_ENOUGH_MEMORY) {
		logwrap(fputs("DetourUpdateThread: ERROR_NOT_ENOUGH_MEMORY: Not enough memory to record identity of thread.\n", logfile));
	} else if (err != NO_ERROR) {
		logwrap(fprintf(logfile, "DetourUpdateThread: %d\n", err));
	}
}

void printDetourDetachError(LONG err) {
	switch (err) {
	case ERROR_INVALID_BLOCK: logwrap(fputs("ERROR_INVALID_BLOCK :  The function to be detached was too small to be detoured.\n", logfile)); break;
	case ERROR_INVALID_HANDLE: logwrap(fputs("ERROR_INVALID_HANDLE : The ppPointer parameter is NULL or references a NULL address.\n", logfile)); break;
	case ERROR_INVALID_OPERATION: logwrap(fputs("ERROR_INVALID_OPERATION : No pending transaction exists.\n", logfile)); break;
	case ERROR_NOT_ENOUGH_MEMORY: logwrap(fputs("ERROR_NOT_ENOUGH_MEMORY : Not enough memory exists to complete the operation.\n", logfile)); break;
	default: {
		if (err != NO_ERROR) {
			logwrap(fprintf(logfile, "DetourDetach: %d\n", err));
		}
	}
	}
}

void printDetourAttachError(LONG err) {
	switch (err) {
	case ERROR_INVALID_BLOCK: logwrap(fputs("ERROR_INVALID_BLOCK : The function referenced is too small to be detoured.\n", logfile)); break;
	case ERROR_INVALID_HANDLE: logwrap(fputs("ERROR_INVALID_HANDLE : The ppPointer parameter is NULL or points to a NULL pointer.\n", logfile)); break;
	case ERROR_INVALID_OPERATION: logwrap(fputs("ERROR_INVALID_OPERATION : No pending transaction exists.\n", logfile)); break;
	case ERROR_NOT_ENOUGH_MEMORY: logwrap(fputs("ERROR_NOT_ENOUGH_MEMORY : Not enough memory exists to complete the operation.\n", logfile)); break;
	default: {
		if (err != NO_ERROR) {
			logwrap(fprintf(logfile, "DetourAttach: %d\n", err));
		}
	}
	}
}

void printDetourTransactionCommitError(LONG err) {
	if (err == ERROR_INVALID_DATA) {
		logwrap(fputs("DetourTransactionCommit: ERROR_INVALID_DATA: Target function was changed by third party between steps of the transaction.\n", logfile));
	} else if (err == ERROR_INVALID_OPERATION) {
		logwrap(fputs("DetourTransactionCommit: ERROR_INVALID_OPERATION: No pending transaction exists..\n", logfile));
	} else if (err != NO_ERROR) {
		logwrap(fprintf(logfile, "DetourTransactionCommit: %d\n", err));
	}
}

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
EndScene_t orig_EndScene;

HRESULT __stdcall hook_EndScene(IDirect3DDevice9 *device);

void detachFunctions(std::string calledFrom) {
	DWORD detourResult;
	if (dev_vtable != nullptr) {
		detourResult = DetourTransactionBegin();
		if (detourResult != NO_ERROR) {
			printDetourTransactionBeginError(detourResult);
			return;
		}

		detourResult = DetourUpdateThread(GetCurrentThread());
		if (detourResult != NO_ERROR) {
			printDetourUpdateThreadError(detourResult);
			return;
		}

		bool allSuccess = true;

		for (const std::pair<PVOID*, PVOID>& thing : thingsToUndetourAtTheEnd) {
			detourResult = DetourDetach(
				thing.first,
				thing.second);
			if (detourResult != NO_ERROR) {
				printDetourDetachError(detourResult);
				allSuccess = false;
				break;
			}
		}
		if (allSuccess) {
			logwrap(fputs("Successfully undetoured all the hooks\n", logfile));
		} else {
			logwrap(fputs("Successfully undetoured some or all of the hooks\n", logfile));
		}
		thingsToUndetourAtTheEnd.clear();
	
		detourResult = DetourTransactionCommit();
		if (detourResult != NO_ERROR) {
			printDetourTransactionCommitError(detourResult);
			return;
		}
		logwrap(fputs("Successfully committed undetour transaction\n", logfile));
		dev_vtable = nullptr;
	}
}

bool get_module_bounds(const char *name, uintptr_t *start, uintptr_t *end)
{
	const auto module = GetModuleHandle(name);
	if(module == nullptr)
		return false;

	MODULEINFO info;
	GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info));
	*start = (uintptr_t)(info.lpBaseOfDll);
	*end = *start + info.SizeOfImage;
	return true;
}

enum CHARACTER_TYPE {
	CHARACTER_TYPE_SOL,
	CHARACTER_TYPE_KY,
	CHARACTER_TYPE_MAY,
	CHARACTER_TYPE_MILLIA,
	CHARACTER_TYPE_ZATO,
	CHARACTER_TYPE_POTEMKIN,
	CHARACTER_TYPE_CHIPP,
	CHARACTER_TYPE_FAUST,
	CHARACTER_TYPE_AXL,
	CHARACTER_TYPE_VENOM,
	CHARACTER_TYPE_SLAYER,
	CHARACTER_TYPE_INO,
	CHARACTER_TYPE_BEDMAN,
	CHARACTER_TYPE_RAMLETHAL,
	CHARACTER_TYPE_SIN,
	CHARACTER_TYPE_ELPHELT,
	CHARACTER_TYPE_LEO,
	CHARACTER_TYPE_JOHNNY,
	CHARACTER_TYPE_JACKO,
	CHARACTER_TYPE_JAM,
	CHARACTER_TYPE_HAEHYUN,
	CHARACTER_TYPE_RAVEN,
	CHARACTER_TYPE_DIZZY,
	CHARACTER_TYPE_BAIKEN,
	CHARACTER_TYPE_ANSWER
};

bool determineInvisChipp(char* asw_data) {
	return (*(char*)(asw_data + 0x44) == CHARACTER_TYPE_CHIPP)
		&& *(short*)(asw_data + 0x24C50) != 0;
}

std::vector<DrawHitboxArrayCallParams> hurtboxes;
std::vector<DrawHitboxArrayCallParams> hitboxes;
std::vector<DrawOutlineCallParams> outlines;
std::vector<DrawPointCallParams> points;
std::vector<char*> drawnEntities;

HRESULT __stdcall hook_EndScene(IDirect3DDevice9 *device)
{
	/*if (*game_ptr == nullptr)
		return orig_EndScene(device);

	const auto *world = *(char**)(*game_ptr + 0x50);
	if (world == nullptr)
		return orig_EndScene(device);

	const auto *world_info = **(char***)(world + 0x3C);
	if (world_info == nullptr)
		return orig_EndScene(device);

	const auto *game_info = *(char**)(world_info + 0x4A8);
	if (game_info == nullptr)
		return orig_EndScene(device);

	// Make sure it's a REDGameInfo_Battle
	if (cast_REDGameInfo_Battle(game_info) == nullptr)
		return orig_EndScene(device);*/

	/*HWND activeWindow = GetForegroundWindow();  // GetActiveWindow wasn't working so I used this instead
	if (activeWindow == thisProcessWindow) {
		const bool pressedF3 = (GetKeyState(VK_F3) & 0x8000) != 0;  // without the window check this will work even if window is not in focus which is bad
		if (pressedF3) {
		}
	}*/

	bool prematureExit = false;
	if (*asw_engine == nullptr) {
		hitboxesThatHit.clear();
		prematureExit = true;
	}

	if (trainingModeMenuOpenRef && *trainingModeMenuOpenRef
			|| versusModeMenuOpenRef && *versusModeMenuOpenRef) {
		prematureExit = true;
	}

	if (isIKCutscenePlaying && *isIKCutscenePlaying) {
		hitboxesThatHit.clear();
	}

	if (prematureExit) {
		return orig_EndScene(device);
	}

	log(fputs("hook_EndScene called\n", logfile));

	angleVectors.isSet = false;
	bool stencilInitialized = false;

	const auto ent_count = *(int*)(*asw_engine + /*! 0xB0*/0xB4);  // moved to 0xB4
	const auto ent_list = (char**)(*asw_engine + /*! 0x70C*/0x1FC);  // moved to 0x1FC

	const auto ent_slots = (char**)(*asw_engine + /*! 0xC4*/0xC8);  // moved to 0xC8

	/*const bool gif_toggle_pressed = (GetKeyState(VK_F1) & 0x8000) != 0;
	if (!gif_toggle_held && gif_toggle_pressed)
	{
		gif_mode = !gif_mode;
		if (!gif_mode)
		{
			// Scale
			*(int*)(ent_slots[1] + 0x258) = 1000; // moved to 0x264
			*(int*)(ent_slots[1] + 0x25C) = 1000; // moved to 0x268
			*(int*)(ent_slots[1] + 0x260) = 1000; // moved to 0x26c but not confirmed if it's indeed zScale or if zScale exists at all

			// Default scale
			*(int*)(ent_slots[1] + 0x229C) = 1000; // this has to be the 0x2594 offset
		}
	}

	const auto nograv_toggle_pressed = (GetKeyState(VK_F2) & 0x8000) != 0;
	if (!nograv_toggle_held && nograv_toggle_pressed)
	{
		nograv_mode = !nograv_mode;
		if (!nograv_mode)
			*(int*)(ent_slots[0] + 0x2D8) = 1925;
	}

	if (gif_mode)
	{
		// Scale
		*(int*)(ent_slots[1] + 0x258) = 0;
		*(int*)(ent_slots[1] + 0x25C) = 0;
		*(int*)(ent_slots[1] + 0x260) = 0;

		// Default scale
		*(int*)(ent_slots[1] + 0x229C) = 0;
	}

	if (nograv_mode)
	{
		*(int*)(ent_slots[0] + 0x2D8) = 0;
	}

	gif_toggle_held = gif_toggle_pressed;
	nograv_toggle_held = nograv_toggle_pressed;*/
	
    hurtboxes.clear();
    hitboxes.clear();
	outlines.clear();
    points.clear();
	drawnEntities.clear();
	allRotatedPathElems.clear();

	bool p1IsInvisChipp = false;
	bool p2IsInvisChipp = false;
	char gameMode = *(*game_data_ptr + 0x45);
	if (!(gameMode == GAME_MODE_ARCADE
				|| gameMode == GAME_MODE_CHALLENGE
				|| gameMode == GAME_MODE_REPLAY
				|| gameMode == GAME_MODE_STORY
				|| gameMode == GAME_MODE_TRAINING
				|| gameMode == GAME_MODE_TUTORIAL
				|| gameMode == GAME_MODE_VERSUS)
			&& ent_count >= 2) {

		p1IsInvisChipp = determineInvisChipp(ent_slots[0]);
		p2IsInvisChipp = determineInvisChipp(ent_slots[1]);
	}

	log(fprintf(logfile, "ent_count: %d\n", ent_count));
	for (auto i = 0; i < ent_count; i++)
	{
		const auto ent = ent_list[i];
		if (std::find(drawnEntities.cbegin(), drawnEntities.cend(), ent) != drawnEntities.cend()) {
			continue;
		}
		/*if (gif_mode && ent == ent_slots[1])
			continue;*/

		bool active = (*(unsigned int*)(ent + 0x23C) & 0x100) != 0
		              && (*(unsigned int*)(ent + 0x234) & 0x40000000) == 0;
		log(fprintf(logfile, "drawing entity # %d. active: %c\n", i, active));
		
		bool needToHide = false;
		if (*(char*)(ent + 0x40) == 0) {  // 0x40 - side to which entity belongs, 0 for p1's side, 1 for p2's side
			needToHide = p1IsInvisChipp;
		} else if (ent_count > 1) {
			needToHide = p2IsInvisChipp;
		}
		if (needToHide) continue;
		collect_hitboxes(ent, active, &hurtboxes, &hitboxes, &points);
		drawnEntities.push_back(ent);

		// Attached entities like dusts
		const auto attached = *(char**)(ent + 0x204);
		if (attached != nullptr) {
			log(fprintf(logfile, "Attached entity: %p\n", attached));
			collect_hitboxes(attached, active, &hurtboxes, &hitboxes, &points);
			drawnEntities.push_back(attached);
		}
	}

	auto it = hitboxesThatHit.begin();
	while (it != hitboxesThatHit.end()) {
		HitboxThatHitStruct& hitboxThatHit = *it;

		bool entityInTheList = false;
		for (auto i = 0; i < ent_count; i++) {
			char * const ent = ent_list[i];
			// this is needed for Sol's Gunflame. The gunflame continues to exist as entity but stops being active as soon as it hits
			const bool entityIsActive = (*(unsigned int*)(ent + 0x23C) & 0x100) != 0;
			if (hitboxThatHit.attacker == ent && entityIsActive) {
				entityInTheList = true;
				break;
			}
		}

		if (entityInTheList) {
			it = hitboxesThatHit.erase(it);
			continue;
		} else {
			bool needToHide = false;
			if (hitboxThatHit.attackerTeam == 0) {
				needToHide = p1IsInvisChipp;
			} else if (ent_count > 1) {
				needToHide = p2IsInvisChipp;
			}
			if (needToHide) {
				it = hitboxesThatHit.erase(it);
				continue;
			}
			hitboxes.push_back(hitboxThatHit.hitboxes);
			--hitboxThatHit.counter;
			if (hitboxThatHit.counter <= 0) {
				it = hitboxesThatHit.erase(it);
				continue;
			}
		}
		++it;
	}
	
	for (const DrawHitboxArrayCallParams& params : hurtboxes) {
		draw_hitbox_array(device, params, outlines, stencilInitialized);
	}
	for (auto it = hitboxes.cbegin(); it != hitboxes.cend(); ++it) {
		const DrawHitboxArrayCallParams& params = *it;
		bool found = false;
		for (auto itScan = it; itScan != hitboxes.cend(); ++itScan) {
			if (it == itScan) continue;

			const DrawHitboxArrayCallParams& paramsScan = *itScan;
			if (!(params.hitboxCount == paramsScan.hitboxCount
					&& params.params.flip == paramsScan.params.flip
					&& params.params.scaleX == paramsScan.params.scaleX
					&& params.params.scaleY == paramsScan.params.scaleY
					&& params.params.cosAngle == paramsScan.params.cosAngle
					&& params.params.sinAngle == paramsScan.params.sinAngle
					&& params.params.posX == paramsScan.params.posX
					&& params.params.posY == paramsScan.params.posY
					&& params.fillColor == paramsScan.fillColor)) continue;
			

			found = true;
			Hitbox* hitboxPtr = params.hitbox_data;
			Hitbox* hitboxScanPtr = paramsScan.hitbox_data;
			if (hitboxPtr != hitboxScanPtr) {
				for (int hitboxScanCounter = params.hitboxCount; hitboxScanCounter != 0; --hitboxScanCounter) {
					if (!(hitboxPtr->offx == hitboxScanPtr->offx
							&& hitboxPtr->offy == hitboxScanPtr->offy
							&& hitboxPtr->sizex == hitboxScanPtr->sizex
							&& hitboxPtr->sizey == hitboxScanPtr->sizey)) {
						found = false;
						break;
					}
				
					++hitboxPtr;
					++hitboxScanPtr;
				}
			}
			if (found) break;
		}
		if (!found) draw_hitbox_array(device, params, outlines, stencilInitialized);
	}
	for (const DrawOutlineCallParams& params : outlines) {
		draw_outline(device, params);
	}
	for (const DrawPointCallParams& params : points) {
		draw_point(device, params);
	}

	/*for (const auto &ti : throws)
		draw_throw(device, ti, stencilInitialized);

	throws.clear();*/
	
	if (stencilInitialized) {
		if (FAILED(device->SetDepthStencilSurface(NULL))) {
			log(fputs("SetDepthStencilSurface to NULL failed\n", logfile));
			direct3DError = true;
		} else {
			log(fputs("SetDepthStencilSurface to NULL succeeded\n", logfile));
		}
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	}
	
	#ifdef LOG_PATH
	didWriteOnce = true;
	#endif
	return orig_EndScene(device);
}

uintptr_t sigscan(const char *name, const char *sig, const char *mask)
{
	uintptr_t start, end;
	if (!get_module_bounds(name, &start, &end)) {
		logwrap(fputs("Module not loaded\n", logfile));
		return 0;
	}

	const auto last_scan = end - strlen(mask) + 1;
	for (auto addr = start; addr < last_scan; addr++) {
		for (size_t i = 0;; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
				break;
		}
	}
	logwrap(fputs("Sigscan failed\n", logfile));
	return 0;
}

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	)
{
	LONG detourResult;
	#ifdef LOG_PATH
	if (fdwReason == DLL_PROCESS_ATTACH) {
		errno_t err;
		err = _wfopen_s(&logfile, LOG_PATH, L"wt");
		if (err != 0 || logfile == NULL) {
			return FALSE;
		}
		fputs("DllMain called with fdwReason DLL_PROCESS_ATTACH\n", logfile);
		fclose(logfile);
	} else if (fdwReason != DLL_PROCESS_DETACH) {
		return TRUE;
	}
	#else
	if (fdwReason != DLL_PROCESS_DETACH && fdwReason != DLL_PROCESS_ATTACH) {
		return TRUE;
	}
	#endif

	if (fdwReason == DLL_PROCESS_DETACH) {
		logwrap(fputs("DLL_PROCESS_DETACH\n", logfile));
		detachFunctions("DllMain");

		stencilSurface = NULL;
		return TRUE;
	}

	thisProcessId = GetCurrentProcessId();
	EnumWindows(EnumWindowsFindMyself, NULL);

	uintptr_t sigscanResult;
	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\x33\xC0\x38\x41\x44\x0F\x95\xC0\xC3\xCC",
		"xxxxxxxxxx");
	if (!sigscanResult) {
		logwrap(fputs("game_data_ptr not found\n", logfile));
		return TRUE;
	}
	game_data_ptr = *(char***)(sigscanResult - 0x4);

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\x85\xC0\x78\x74\x83\xF8\x01",  // found
		"xxxxxxx");
	if (!sigscanResult) {
		logwrap(fputs("asw_engine not found\n", logfile));
		return TRUE;
	}
	asw_engine = *(char***)(sigscanResult - 4);

	logwrap(fprintf(logfile, "Found asw_engine at %p\n", asw_engine));

	/*get_pushbox_x = (get_pushbox_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x56\x8B\xF1\x8B\x86\xF4\x02\x00\x00", // found
		"xxxxxxxxx"));

	get_pushbox_y = (get_pushbox_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x56\x8B\xF1\x8B\x86\xF8\x02\x00\x00",  // found but should be -7
		"xxxxxxxxx"));

	get_pushbox_bottom = (get_pushbox_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x3D\xFF\xFF\xFF\x0F\x75\x2B", // found
		"xxxxxxx") - 9);

	is_push_active = (is_push_active_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x83\x7E\x0C\x00\x74\x10\xF7\x86", // not found
		"xxxxxxxx") - 3);*/

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\x85\xC9\x75\x35\x8B\x8E", // found, but it's probably the old version or simplified version
		"xxxxxx");
	if (!sigscanResult) {
		logwrap(fputs("get_pos_x not found\n", logfile));
		return TRUE;
	}
	get_pos_x = (get_pos_t)(sigscanResult - 9);             // there's a better function at image base + 0xB618C0

	logwrap(fprintf(logfile, "Found get_pos_x at %p\n", get_pos_x));

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\x75\x0A\x6A\x08\xE8", // found
		"xxxxx");
	if (!sigscanResult) {
		logwrap(fputs("get_pos_y not found\n", logfile));
		return TRUE;
	}
	get_pos_y = (get_pos_t)(sigscanResult - 0xB);  // is encountered twice in the same func, the first encounter is the right one and sigscan should return that

	logwrap(fprintf(logfile, "Found get_pos_y at %p\n", get_pos_y));

	/*const auto cast_ref = sigscan(
		"GuiltyGearXrd.exe",
		"\x8B\x88\x00\x00\x00\x00\x51\xC7\x44\x24\x00\x00\x00\x00\x00\xE8", // not found
		"xx????xxxx?????x") + 0xF;

	cast_REDGameInfo_Battle = (cast_t)(cast_ref + *(intptr_t*)(cast_ref + 1) + 5);*/

	sigscanResult = sigscan(
		"d3d9.dll",
		"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86",  // found
		"xx????xx????xx");
	 if (!sigscanResult) {
		 logwrap(fputs("dev_vtable not found\n", logfile));
		 return TRUE;
	 }
	dev_vtable = *(char***)(sigscanResult + 0x2);

	logwrap(fprintf(logfile, "Found dev_vtable at %p\n", dev_vtable));

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\x85\xc0\x75\x0b\x68\x00\x00\x00\x01\xff\x15\x00\x00\x00\x01\x33\xc0\x68\x00\x00\x00\x01\xa3\x00\x00\x00\x01\xc7\x05\x00\x00\x00\x01\x00\x00\x00\x01\xa3\x00\x00\x00\x01\xff\x15\x00\x00\x00\x01\xc3",
		"xxxxx???xxx???xxxx???xx???xxx???x???xx???xxx???xx");
	if (!sigscanResult) {
		logwrap(fputs("Training mode in-game menu not found\n", logfile));
		// not a critical error
	} else {
		logwrap(fprintf(logfile, "Training mode in-game menu found at: %x\n", sigscanResult));
		trainingModeMenuOpenRef = *(char**)(sigscanResult + 33) + 0x38;
		logwrap(fprintf(logfile, "Training mode in-game is open value at: %p\n", trainingModeMenuOpenRef));
	}

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\xe8\x00\x00\x00\x00\x03\xf3\x83\xfe\x66\x7c\xe5\xa1\x00\x00\x00\x01\x39\x3d\x00\x00\x00\x01\x74\x0f\x29\x1d\x00\x00\x00\x01\x83\xf8\x05\x73\x13\x03\xc3\xeb\x0a\x3b\xc7\x0f\x84\x99\x00\x00\x00\x2b\xc3",
		"x????xxxxxxxx???xxx???xxxxx???xxxxxxxxxxxxxxxxxxxx");
	if (!sigscanResult) {
		logwrap(fputs("Versus mode in-game menu not found\n", logfile));
		// not a critical error
	} else {
		logwrap(fprintf(logfile, "Versus mode in-game menu found at: %x\n", sigscanResult));
		versusModeMenuOpenRef = *(char**)(sigscanResult + 19);
		logwrap(fprintf(logfile, "Versus mode in-game is open value at: %p\n", versusModeMenuOpenRef));
	}

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\xa1\x00\x00\x00\x00\xb9\x00\x00\x00\x00\x89\x47\x28\xe8\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x00\x8b\xb1\x00\x00\x00\x00\xa1\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x01\x0f\x57\xc0\x8b\x1e\x33\xed\x55\x8d\x54\x24\x14",
		"x????x????xxxx????xx????xx????x????xx????xxxxxxxxxxxx");
	if (!sigscanResult) {
		logwrap(fputs("Is IK cutscene playing variable not found\n", logfile));
		// not a critical error
	} else {
		logwrap(fprintf(logfile, "Reference to Is IK cutscene playing found at: %x\n", sigscanResult));
		isIKCutscenePlaying = *(char**)(sigscanResult + 1);
		logwrap(fprintf(logfile, "Is IK cutscene playing variable at: %p\n", isIKCutscenePlaying));
	}
	

	detourResult = DetourTransactionBegin();
	if (detourResult != NO_ERROR) {
		printDetourTransactionBeginError(detourResult);
		return TRUE;
	}

	detourResult = DetourUpdateThread(GetCurrentThread());
	if (detourResult != NO_ERROR) {
		printDetourUpdateThreadError(detourResult);
		return TRUE;
	}

	orig_EndScene = (EndScene_t)dev_vtable[42];
	detourResult = DetourAttach(
		&(PVOID&)(orig_EndScene),
		hook_EndScene);
	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully detoured EndScene\n", logfile));
	thingsToUndetourAtTheEnd.push_back({ &(PVOID&)(orig_EndScene), hook_EndScene });

	orig_Reset = (Reset_t)dev_vtable[16];
	detourResult = DetourAttach(
		&(PVOID&)(orig_Reset),
		hook_Reset);
	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully detoured Reset\n", logfile));
	thingsToUndetourAtTheEnd.push_back({ &(PVOID&)(orig_Reset), hook_Reset });


	/*const auto can_throw = sigscan(
		"GuiltyGearXrd.exe",
		"\x8B\x6F\x0C\xF7\xDD\x1B\xED", // not found
		"xxxxxxx") - 7;

	orig_can_throw = (can_throw_t)(DetourAttach(
		reinterpret_cast<void**>(can_throw),
		reinterpret_cast<void*>(hook_can_throw)));*/

	// removed because couldn't find this
	/*const auto update_darken = sigscan(
		"GuiltyGearXrd.exe",
		"\x57\x74\x12\xF3\x0F\x10\x86", // not found
		"xxxxxxx") - 0xC;

	orig_update_darken = (update_darken_t)(DetourAttach(
		reinterpret_cast<void**>(update_darken),
		reinterpret_cast<void*>(hook_update_darken)));*/

	/*const auto update_camera = sigscan(
		"GuiltyGearXrd.exe",
		"\x57\x8B\x7D\x08\x74\x17", // found
		"xxxxxx") - 0x14;

	orig_update_camera = (update_camera_t)(DetourAttach(
		reinterpret_cast<void**>(update_camera),
		reinterpret_cast<void*>(hook_update_camera)));*/

	/*const auto update_hud = sigscan(
		"GuiltyGearXrd.exe",
		"\x8B\x86\xD8\x00\x00\x00\x8B\x88\xA8\x04\x00\x00\x51",
		"xxxxxxxxxxxxx") - 0x52;  // found but -0x52

	orig_update_hud = (update_hud_t)(DetourAttach(
		reinterpret_cast<void**>(update_hud),
		reinterpret_cast<void*>(hook_update_hud)));*/

	sigscanResult = sigscan(
		"GuiltyGearXrd.exe",
		"\x74\x12\x85\xed\x75\x0e\x85\xdb\x74\x39\x85\xff\x74\x35\x8b\x74\x24\x14\xeb\x1d\x83\xfd\x01",  // refound
		"xxxxxxx");
	orig_hit_detection = (hit_detection_t)(sigscanResult - 0x3D);

	BOOL(HitDetectionHookHelp::*hook_hit_detection_ptr)(void*, int, int, int*, int*) = &HitDetectionHookHelp::hook_hit_detection;
	detourResult = DetourAttach(
		&(PVOID&)(orig_hit_detection),
		*(PVOID*)&hook_hit_detection_ptr);
	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully detoured hit_detection\n", logfile));
	thingsToUndetourAtTheEnd.push_back({ &(PVOID&)(orig_hit_detection), *(PVOID*)&hook_hit_detection_ptr });

	detourResult = DetourTransactionCommit();
	if (detourResult != NO_ERROR) {
		printDetourTransactionCommitError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully committed detour transaction\n", logfile));

	return TRUE;
}