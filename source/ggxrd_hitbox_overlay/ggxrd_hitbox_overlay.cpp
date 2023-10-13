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

struct FloatRect {
	float left = 0.F;
	float right = 0.F;
	float top = 0.F;
	float bottom = 0.F;
};

const float outlineThickness = 4000.F;
CComPtr<IDirect3DSurface9> stencilSurface = NULL;
bool direct3DError = false;
bool direct3DSuccess = false;
FloatRect nullFloatRect;

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

const float coord_coefficient = 0.42960999705207F;

//char **game_ptr;
char **asw_engine;

//using cast_t = const void*(*)(const void*);
//cast_t cast_REDGameInfo_Battle; // found something like PTR_s_AREDGameInfo_BattleexecRenderUpd_019cd5e0, but no cast

//using is_active_t = bool(__thiscall*)(const void*, int);
//is_active_t is_active;

//using get_pushbox_t = int(__thiscall*)(const void*);
//get_pushbox_t get_pushbox_x, get_pushbox_y, get_pushbox_bottom;

//using is_push_active_t = bool(__thiscall*)(const void*);
//is_push_active_t is_push_active;

using get_pos_t = int(__thiscall*)(const void*);
get_pos_t get_pos_x, get_pos_y;

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

struct RotatedPathElement {
	float x = 0.F;
	float y = 0.F;
	float inX = 0.F;
	float inY = 0.F;
};

void draw_angled_outline(IDirect3DDevice9 *device,
	const D3DXVECTOR3 &p1,
	const D3DXVECTOR3 &p2,
	const D3DXVECTOR3 &p3,
	const D3DXVECTOR3 &p4,
	D3DCOLOR outlineColor) {
	
}

void draw_outline(IDirect3DDevice9 *device, const std::vector<RotatedPathElement>& outline, D3DCOLOR outlineColor) {
	log(fprintf(logfile, "Called draw_outlines with an outline with %d elements\n", outline.size()));
	device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->SetTexture(0, nullptr);

	D3DXVECTOR3 conv;
	for (const RotatedPathElement& elem : outline) {
		world_to_screen(device, D3DXVECTOR3{ elem.x, 0.F, elem.y }, &conv);
		log(fprintf(logfile, "x: %f; y: %f;\n", conv.x, conv.y));
		vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, outlineColor);
		world_to_screen(device, D3DXVECTOR3{ elem.x + outlineThickness * elem.inX,
		                                     0.F,
		                                     elem.y + outlineThickness * elem.inY },
			            &conv);
		vertexArena.emplace_back(conv.x, conv.y, 0.F, 1.F, outlineColor);
	}
	vertexArena.push_back(vertexArena.front());
	vertexArena.push_back(vertexArena[1]);

	// Triangle strip means v1, v2, v3 is a triangle, v2, v3, v4 is a triangle, etc.
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, outline.size() * 2, &vertexArena.front(), sizeof(Vertex));
	vertexArena.clear();
}

void draw_rect(
	IDirect3DDevice9 *device,
	const D3DXVECTOR3 &p1,
	const D3DXVECTOR3 &p2,
	const D3DXVECTOR3 &p3,
	const D3DXVECTOR3 &p4,
	D3DCOLOR fillColor,
	FloatRect& bounds,
	bool& stencilInitialized) {
	
	initializeStencil(device, stencilInitialized);

	D3DXVECTOR3 sp1, sp2, sp3, sp4;
	world_to_screen(device, p1, &sp1);
	world_to_screen(device, p2, &sp2);
	world_to_screen(device, p3, &sp3);
	world_to_screen(device, p4, &sp4);

	if (sp1.x == sp2.x && sp2.x == sp3.x && sp3.x == sp4.x)
		return;
		
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
		device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
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
		{ sp1.x, sp1.y, 0.F, 1.F, fillColor },
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

void draw_point(
	IDirect3DDevice9 *device,
	const D3DXVECTOR3 &p,
	D3DCOLOR inner_color,
	D3DCOLOR outer_color)
{
	log(fprintf(logfile, "draw_point called x: %f; y: %f; z: %f\n", p.x, p.y, p.z));

	D3DXVECTOR3 sp;
	world_to_screen(device, p, &sp);

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
		{ sp.x - 4, sp.y - 1, 0.F, 1.F, outer_color },
		{ sp.x - 4, sp.y + 2, 0.F, 1.F, outer_color },
		{ sp.x + 5, sp.y - 1, 0.F, 1.F, outer_color },
		{ sp.x + 5, sp.y + 2, 0.F, 1.F, outer_color },
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
		{ sp.x - 1, sp.y - 4, 0.F, 1.F, outer_color },
		{ sp.x - 1, sp.y + 5, 0.F, 1.F, outer_color },
		{ sp.x + 2, sp.y - 4, 0.F, 1.F, outer_color },
		{ sp.x + 2, sp.y + 5, 0.F, 1.F, outer_color },
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
		{ sp.x - 3, sp.y, 0.F, 1.F, inner_color },
		{ sp.x + 4, sp.y, 0.F, 1.F, inner_color },
		{ sp.x, sp.y - 3, 0.F, 1.F, inner_color },
		{ sp.x, sp.y + 4, 0.F, 1.F, inner_color },
	};
	
	// Linelist means every two vertices is a line. error on uneven vertices
	device->DrawPrimitiveUP(D3DPT_LINELIST, 2, vertices, sizeof(Vertex));
}

/*auto gif_mode = false;
auto gif_toggle_held = false;

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

/*using hit_detection_t = void(__thiscall*)(void*, int, int);
hit_detection_t orig_hit_detection;
void __fastcall hook_hit_detection(void *thisptr, void*, int a2, int a3)
{
	if (!gif_mode)
		orig_hit_detection(thisptr, a2, a3);
}*/

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

struct Hitbox
{
	int type;
	float offx;
	float offy;
	float sizex;
	float sizey;
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

std::vector<RectCombiner::Polygon> rectCombinerInputBoxes;
std::vector<std::vector<RectCombiner::PathElement>> rectCombinerOutlines;

std::vector<RotatedPathElement> rotatedOutline;

void draw_hitbox_array(IDirect3DDevice9 *device,
                       Hitbox* hitbox_data,
					   int hitboxCount,
					   DrawHitboxArrayParams params,
					   D3DCOLOR fillColor,
					   D3DCOLOR outlineColor,
					   bool& stencilInitialized) {
	if (!hitboxCount) return;

	rectCombinerInputBoxes.reserve(hitboxCount);
	FloatRect bounds { -1.F, 99999.F, -1.F, 99999.F };

	for (int i = hitboxCount; i != 0; --i) {
		log(fprintf(logfile, "drawing box %d\n", hitboxCount - i));
		const auto type = *(int*)(hitbox_data);

		const int box_x = (int)(hitbox_data->offx * params.scaleX);
		const int box_y = (int)(hitbox_data->offy * params.scaleY);
		const int box_width = (int)(hitbox_data->sizex * params.scaleX);
		const int box_height = (int)(hitbox_data->sizey * params.scaleY);

		const int x1 = box_x;
		const int y1 = box_y;

		const int x2 = box_x;
		const int y2 = box_y + box_height;

		const int x3 = box_x + box_width;
		const int y3 = box_y;
		
		const int x4 = box_x + box_width;
		const int y4 = box_y + box_height;

		rectCombinerInputBoxes.emplace_back(box_x, box_x + box_width, box_y, box_y + box_height);
		
		D3DXVECTOR3 v1 { params.posX + (x1 * params.cosAngle - y1 * params.sinAngle) * params.flip,
		                0.F,
						params.posY - (y1 * params.cosAngle + x1 * params.sinAngle) };
		D3DXVECTOR3 v2 { params.posX + (x2 * params.cosAngle - y2 * params.sinAngle) * params.flip,
		                0.F,
						params.posY - (y2 * params.cosAngle + x2 * params.sinAngle) };
		D3DXVECTOR3 v3 { params.posX + (x3 * params.cosAngle - y3 * params.sinAngle) * params.flip,
		                0.F,
						params.posY - (y3 * params.cosAngle + x3 * params.sinAngle) };
		D3DXVECTOR3 v4 { params.posX + (x4 * params.cosAngle - y4 * params.sinAngle) * params.flip,
		                0.F,
						params.posY - (y4 * params.cosAngle + x4 * params.sinAngle) };

		draw_rect(
			device,
			v1,
			v2,
			v3,
			v4,
			fillColor,
			bounds,
			stencilInitialized);

		++hitbox_data;
	}

	if (stencilInitialized) {
		D3DRECT rect{(LONG)bounds.left - 1, (LONG)bounds.top - 1, (LONG)bounds.right + 1, (LONG)bounds.bottom + 1};
		device->Clear(1, &rect, D3DCLEAR_STENCIL, D3DCOLOR{}, 1.f, 0);
	}

	RectCombiner::getOutlines(rectCombinerInputBoxes, rectCombinerOutlines);
	for (const std::vector<RectCombiner::PathElement>& outline : rectCombinerOutlines) {
		rotatedOutline.resize(outline.size());
		auto it = rotatedOutline.begin();
		for (const RectCombiner::PathElement& path : outline) {
			it->x = (float)params.posX + params.flip * ((float)path.x * params.cosAngle - (float)path.y * params.sinAngle);
			it->y = (float)params.posY - ((float)path.y * params.cosAngle + (float)path.x * params.sinAngle);
			it->inX = params.flip * ((float)path.xDir() * params.cosAngle - (float)path.yDir() * params.sinAngle);
			it->inY = -((float)path.yDir() * params.cosAngle + (float)path.xDir() * params.sinAngle);
			++it;
		}
		draw_outline(device, rotatedOutline, outlineColor);
	}
	rectCombinerInputBoxes.clear();
}

void draw_hitboxes(IDirect3DDevice9 *device, char *asw_data, const bool active, bool& stencilInitialized)
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

	const auto otg = false;//(*(int*)(asw_data + 0x2410) & 0x800000) != 0;  // not found yet
	const auto invuln_frames = 0;//*(int*)(asw_data + 0x964);  // not found yet
	const auto invuln_flags = *(char*)(asw_data + /*! 0x230*/0x238);
	const auto strike_invuln = invuln_frames > 0 || (invuln_flags & 16) || (invuln_flags & 64);
	const auto throw_invuln = invuln_frames > 0 || (invuln_flags & 32) || (invuln_flags & 64) || otg;
	const auto counterhit = false;//(*(int*)(asw_data + 0x22C) & 256) != 0;  // not found yet
	// Color scheme:
	// purple - hurtbox on counterhit
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

	auto * const hurtbox_data = *(Hitbox**)(asw_data + 0x58);  // this address is correct and this assumes asw_data is the player
	if (hurtbox_data == nullptr)
		return;

	auto * const hitbox_data = *(Hitbox**)(asw_data + 0x5C);
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
	
	draw_hitbox_array(device,
		hurtbox_data,
		hurtbox_count,
		params,
		D3DCOLOR_ARGB(strike_invuln ? 0 : 64, 0, 255, counterhit ? 255 : 0),
		D3DCOLOR_ARGB(255, 0, 255, counterhit ? 255 : 0),
		stencilInitialized);

	draw_hitbox_array(device,
		hitbox_data,
		hitbox_count,
		params,
		D3DCOLOR_ARGB(active ? 64 : 0, 255, 0, 0),
		D3DCOLOR_ARGB(255, 255, 0, 0),
		stencilInitialized);

	draw_point(
		device,
		D3DXVECTOR3(params.posX, 0.F, params.posY),
		D3DCOLOR_ARGB(255, 255, 255, 255),
		D3DCOLOR_ARGB(255, 0, 0, 0));
	log(fprintf(logfile, "draw_hitboxes finishing successfully\n"));
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

bool printedDimensionsOnce = false;

int resetLimit = 10;

using Reset_t = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS *pPresentationParameters);
Reset_t orig_Reset;
HRESULT __stdcall hook_Reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *pPresentationParameters) {
	stencilSurface = NULL;
	direct3DSuccess = false;
	return orig_Reset(device, pPresentationParameters);
}

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
EndScene_t orig_EndScene;
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


	if (*asw_engine == nullptr) {
		log(fputs("hook_EndScene called\n", logfile));
		logwrap(didWriteOnce = true);
		return orig_EndScene(device);
	}

	log(fputs("hook_EndScene called\n", logfile));

	angleVectors.isSet = false;
	bool stencilInitialized = false;

	const auto ent_count = *(int*)(*asw_engine + /*! 0xB0*/0xB4);  // moved to 0xB4
	const auto ent_list = (char**)(*asw_engine + /*! 0x70C*/0x1FC);  // moved to 0x1FC

	const auto ent_slots = (char**)(*asw_engine + /*! 0xC4*/0xC8);  // moved to 0xC8

	/*const auto gif_toggle_pressed = (GetKeyState(VK_F1) & 0x8000) != 0;  // we'll have to get rid of gif mode, too much is not found or unconfirmed
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
			*(int*)(ent_slots[1] + 0x229C) = 1000;
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

	log(fprintf(logfile, "ent_count: %d\n", ent_count));
	for (auto i = 0; i < ent_count; i++)
	{
		const auto ent = ent_list[i];
		/*if (gif_mode && ent == ent_slots[1])
			continue;*/

		const auto active = /*!*/true;//is_active(ent, 0);
		log(fprintf(logfile, "drawing entity # %d\n", i));
		draw_hitboxes(device, ent, active, stencilInitialized);

		// Attached entities like dusts
		/*const auto attached = *(char**)(ent + 0x200);
		if (attached != nullptr)
			draw_hitboxes(device, attached, active);*/
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
	
	logwrap(didWriteOnce = true;);
	return orig_EndScene(device);
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

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	)
{
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
		logwrap(fputs("DLL_PROCESS_DETACH", logfile));
		stencilSurface = NULL;
		return TRUE;
	}

	/*game_ptr = *(char***)(sigscan(
		"GuiltyGearXrd.exe",
		"\x33\xFF\x3B\xDF\x74\x26",  // found
		"xxxxxx") - 0xD);*/

	/*is_active = (is_active_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\xA8\x03\x75\x28\xF7\x86",
		"xxxxxx") - 0x14);*/

	/*is_active = (is_active_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x8B\x86\x00\x00\x00\x00\x83\xE0\x01\x74\x10\xF7\x86",  // not found
		"xx????xxxxxxx") - 3);*/

	asw_engine = *(char***)(sigscan(
		"GuiltyGearXrd.exe",
		"\x85\xC0\x78\x74\x83\xF8\x01",  // found
		"xxxxxxx") - 4);

	if ((int)asw_engine < 0) {
		logwrap(fputs("asw_engine sigscan failed\n", logfile));
		return FALSE;
	}
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

	get_pos_x = (get_pos_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x85\xC9\x75\x35\x8B\x8E", // found, but it's probably the old version or simplified version
		"xxxxxx") - 9);             // there's a better function at image base + 0xB618C0

	if ((int)get_pos_x < 0) {
		logwrap(fputs("get_pos_x sigscan failed\n", logfile));
		return FALSE;
	}
	logwrap(fprintf(logfile, "Found get_pos_x at %p\n", get_pos_x));

	get_pos_y = (get_pos_t)(sigscan(
		"GuiltyGearXrd.exe",
		"\x75\x0A\x6A\x08\xE8", // found
		"xxxxx") - 0xB);  // is encountered twice in the same func, the first encounter is the right one and sigscan should return that

	if ((int)get_pos_y < 0) {
		logwrap(fputs("get_pos_y sigscan failed\n", logfile));
		return FALSE;
	}
	logwrap(fprintf(logfile, "Found get_pos_y at %p\n", get_pos_y));

	/*const auto cast_ref = sigscan(
		"GuiltyGearXrd.exe",
		"\x8B\x88\x00\x00\x00\x00\x51\xC7\x44\x24\x00\x00\x00\x00\x00\xE8", // not found
		"xx????xxxx?????x") + 0xF;

	cast_REDGameInfo_Battle = (cast_t)(cast_ref + *(intptr_t*)(cast_ref + 1) + 5);*/

	const auto *dev_vtable = *(void***)(sigscan(
		"d3d9.dll",
		"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86",  // found
		"xx????xx????xx") + 0x2);

	if ((int)dev_vtable == 0x2) {
		logwrap(fputs("dev_vtable sigscan failed\n", logfile));
		return FALSE;
	}
	logwrap(fprintf(logfile, "Found dev_vtable at %p\n", dev_vtable));

	LONG detourResult;
	logwrap(fputs("About to call DetourTransactionBegin\n", logfile));
	detourResult = DetourTransactionBegin();
	logwrap(fputs("Called DetourTransactionBegin\n", logfile));
	if (detourResult != NO_ERROR) {
		printDetourTransactionBeginError(detourResult);
		return TRUE;
	}

	logwrap(fputs("About to call DetourUpdateThread\n", logfile));
	detourResult = DetourUpdateThread(GetCurrentThread());
	logwrap(fputs("Called DetourUpdateThread\n", logfile));
	if (detourResult != NO_ERROR) {
		printDetourUpdateThreadError(detourResult);
		return TRUE;
	}

	orig_EndScene = (EndScene_t)dev_vtable[42];
	logwrap(fputs("About to call DetourAttach on EndScene\n", logfile));
	detourResult = DetourAttach(
		&(PVOID&)(orig_EndScene),
		hook_EndScene);
	logwrap(fputs("Called DetourAttach\n", logfile));
	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully detoured EndScene\n", logfile));

	orig_Reset = (Reset_t)dev_vtable[16];
	logwrap(fputs("About to call DetourAttach on Reset\n", logfile));
	detourResult = DetourAttach(
		&(PVOID&)(orig_Reset),
		hook_Reset);
	logwrap(fputs("Called DetourAttach\n", logfile));
	if (detourResult != NO_ERROR) {
		printDetourAttachError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully detoured Reset\n", logfile));
	
	detourResult = DetourTransactionCommit();
	if (detourResult != NO_ERROR) {
		printDetourTransactionCommitError(detourResult);
		return TRUE;
	}
	logwrap(fputs("Successfully committed detour transaction\n", logfile));


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

	/*const auto hit_detection = sigscan(
		"GuiltyGearXrd.exe",
		"\x83\xC5\x04\xF7\xD8\x1B\xC0",  // not found
		"xxxxxxx") - 0x3B;*/

	/*orig_hit_detection = (hit_detection_t)(DetourAttach(
		reinterpret_cast<void**>(hit_detection),
		reinterpret_cast<void*>(hook_hit_detection)));*/

	return TRUE;
}