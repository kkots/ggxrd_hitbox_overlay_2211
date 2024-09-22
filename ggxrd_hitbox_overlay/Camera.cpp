#include "pch.h"
#include "Camera.h"
#include "Game.h"
#include "pi.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "GifMode.h"
#include "EntityList.h"
#include "Entity.h"

Camera camera;

bool Camera::onDllMain() {
	bool error = false;

	if (aswEngine) {
		// offset from aswEngine to its field containing a pointer to an instance of AREDCamera_Battle class
		// ghidra sig: 
		std::vector<char> cameraOffsetSig;
		std::vector<char> cameraOffsetSigMask;
		byteSpecificationToSigMask("8b 4c 24 18 83 c4 08 0b 4c 24 14 74 1e 8b 15 ?? ?? ?? ?? 8b 8a ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 75 09 55 e8 ?? ?? ?? ?? 83 c4 04",
			cameraOffsetSig, cameraOffsetSigMask);

		substituteWildcard(cameraOffsetSig.data(), cameraOffsetSigMask.data(), 0, aswEngine);

		// pointer to REDCamera_Battle
		cameraOffset = (unsigned int)sigscanOffset(
			"GuiltyGearXrd.exe",
			cameraOffsetSig.data(),
			cameraOffsetSigMask.data(),
			{ 0x15, 0 },
			&error, "cameraOffset");

		// cameraOffset+4 is pointing to the parent class, REDGameBattle_Info

	}

	// updateCamera is a virtual function of AREDPawn_CameraAttach
	orig_updateDarken = (updateDarken_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"51 56 8b f1 83 be ?? ?? ?? ?? 00 74 12 f3 0f 10 86 ?? ?? ?? ?? f3 0f 5c 86 ?? ?? ?? ?? eb 10 f3 0f 10 86 ?? ?? ?? ?? f3 0f 58 86 ?? ?? ?? ?? 0f 28 c8 f3 0f 11 86",
		nullptr, "updateDarken");

	if (orig_updateDarken) {
		
		darkenValue1Offset = *(unsigned int*)((char*)orig_updateDarken + 17);
		
		void(HookHelp::*updateDarkenHookPtr)() = &HookHelp::updateDarkenHook;
		detouring.attach(&(PVOID&)(orig_updateDarken),
			(PVOID&)updateDarkenHookPtr,
			&orig_updateDarkenMutex,
			"updateDarken");
	}

	// updateCamera is a virtual function of AREDPawn_CameraAttach
	orig_updateCamera = (updateCamera_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"55 8b ec 83 e4 f0 83 ec ?? 53 56 8b f1 83 be ?? ?? ?? ?? 00 57 8b 7d 08 74 17 8d 86 ?? ?? ?? ?? 3b f8 75 0d f6 86 ?? ?? ?? ?? 01 0f 85 18 01 00 00 f6 86 ?? ?? ?? ?? 03 0f 84 0b 01 00 00 8b 0f d9 86",
		nullptr, "updateCamera");

	if (orig_updateCamera) {
		
		void(HookHelp::*updateCameraHookPtr)(char**, char*) = &HookHelp::updateCameraHook;
		detouring.attach(&(PVOID&)(orig_updateCamera),
			(PVOID&)updateCameraHookPtr,
			&orig_updateCameraMutex,
			"updateCamera");

	}

	coordCoeffOffset = (uintptr_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"89 4c 24 10 e9 f5 00 00 00 f3 0f 10 82 ?? ?? ?? ?? 0f 57 c9 8b ce f3 0f 2a cb e8",
		{ 13, 0 },
		&error, "coordCoeffOffset");

	return !error;
}

void Camera::HookHelp::updateDarkenHook() {
	++detouring.hooksCounter;
	detouring.markHookRunning("updateDarken", true);
	camera.updateDarkenHook((char*)this);
	detouring.markHookRunning("updateDarken", false);
	--detouring.hooksCounter;
}

void Camera::HookHelp::updateCameraHook(char** param1, char* param2) {
	++detouring.hooksCounter;
	detouring.markHookRunning("updateCamera", true);
	camera.updateCameraHook((char*)this, param1, param2);
	detouring.markHookRunning("updateCamera", false);
	--detouring.hooksCounter;
}

void Camera::updateDarkenHook(char* thisArg) {
	if (!shutdown) {
		if (gifMode.gifModeOn || gifMode.gifModeToggleBackgroundOnly) {
			*(float*)(thisArg + darkenValue1Offset) = -1.F;
			*(float*)(thisArg + darkenValue1Offset + 0xC) = 0.F;
		}
	}
	std::unique_lock<std::mutex> guard(orig_updateDarkenMutex);
	orig_updateDarken(thisArg);
}

void Camera::updateCameraHook(char* thisArg, char** param1, char* param2) {
	if (!shutdown) {
		if ((gifMode.gifModeOn || gifMode.gifModeToggleCameraCenterOnly) && game.isTrainingMode() && *aswEngine) {
			entityList.populate();
	
			char playerSide = game.getPlayerSide();
			if (playerSide == 2) playerSide = 0;
			if (entityList.count > playerSide) {
				Entity ent = entityList.slots[playerSide];
				const auto posX = convertCoord((float)ent.posX());
				const auto posY = convertCoord((float)ent.posY());
	
				char* deref = *param1;
				*(float*)(deref + 0x54) = posX;
				*(float*)(deref + 0x58) = 540.F;
				*(float*)(deref + 0x5C) = posY + 106.4231F;
			}
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_updateCameraMutex);
		orig_updateCamera(thisArg, param1, param2);
	}
	if (*aswEngine && !shutdown) {  // without *aswEngine check it will actually crash when you quit a match
		CameraValues newValues;
		newValues.setValues();
		newValues.id = nextId;
		newValues.sent = false;
		std::unique_lock<std::mutex> guard(valuesPrepareMutex);
		bool foundOldOne = false;
		for (auto it = valuesPrepare.begin(); it != valuesPrepare.end(); ++it) {
			if (it->id == nextId) {
				*it = newValues;
				foundOldOne = true;
				break;
			}
		}
		if (!foundOldOne) {
			valuesPrepare.push_back(newValues);
		}
		if (valuesPrepare.size() > 15) {
			valuesPrepare.erase(valuesPrepare.begin(), valuesPrepare.begin() + (valuesPrepare.size() - 15));
		}
	}
}

void Camera::onEndSceneStart() {
	isSet = false;
}

void CameraValues::copyTo(CameraValues& destination) {
	memcpy(&destination, this, sizeof(CameraValues));
}

void Camera::worldToScreen(IDirect3DDevice9* device, const D3DXVECTOR3& vec, D3DXVECTOR3* out) {
	setValues(device);

	D3DXVECTOR3 vecConverted{ convertCoord(vec.x), 0.F, convertCoord(vec.z) };

	D3DXVECTOR3 relativePos;
	D3DXVec3Subtract(&relativePos, &vecConverted, &valuesUse.pos);

	out->x = vecDot(relativePos, valuesUse.right);
	out->y = vecDot(relativePos, valuesUse.up);
	out->z = vecDot(relativePos, valuesUse.forward);

	out->x = floorf(clipXHalf - out->x * divisor / out->z + .5F);
	out->y = floorf(clipYHalf - out->y * divisor / out->z + .5F);
	out->z = 0.F;
}

bool CameraValues::setValues() {
	const char* cam = *(char**)(*aswEngine + camera.cameraOffset);
	if (!cam) return false;  // without this it will actually crash when a match finishes loading

	pos.x = *(float*)(cam + 0x3C8);
	pos.y = *(float*)(cam + 0x3CC);
	pos.z = *(float*)(cam + 0x3D0);

	const auto pitch = (float)(*(int*)(cam + 0x3D4)) / 32768.F * PI;
	const auto yaw = (float)(*(int*)(cam + 0x3D8)) / 32768.F * PI;
	const auto roll = (float)(*(int*)(cam + 0x3DC)) / 32768.F * PI;

	fov = *(float*)(cam + 0x3E0);

	camera.angleVectors(pitch, yaw, roll, forward, right, up);

	coordCoefficient = *(float*)(*aswEngine + camera.coordCoeffOffset);

	// I counted like 4 distinct copies of the camera:
	// *((*aswEngine + 0x22e62c) + 0x3C0) + 0x54  -  x position + everything else, except fov is always 1 here
	// (0x22e62c is camera.cameraOffset)
	// (*aswEngine + 0x22e62c) + 0x3C8  -  x position + everything else
	// (*aswEngine + 0x22e62c) + 0x3A4  -  x position + everything else
	// (*aswEngine + 0x22e62c) + 0x384  -  x position + everything else

	return true;

}

void Camera::setValues(IDirect3DDevice9* device) {
	if (isSet) return;
	isSet = true;

	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);

	const float clipX = (float)(viewport.Width);
	const float clipY = (float)(viewport.Height);

	clipXHalf = (clipX / 2.F);
	clipYHalf = (clipY / 2.F);
	divisor = (clipXHalf / (float)tan(valuesUse.fov * PI / 360.F));

}

// Arcsys engine to UE coords

float Camera::convertCoord(float in) const {
	return in / 1000.F * /*!*//*   *(float*)(*asw_engine + 0x3EA724)  */ valuesUse.coordCoefficient;  // if this is not found, just use 0.42960999705207F
}

void Camera::angleVectors(
	const float p, const float y, const float r,
	float* forward, float* right, float* up) const
{
	const auto sp = sin(-p);
	const auto cp = cos(-p);

	const auto sy = sin(y);
	const auto cy = cos(y);

	const auto sr = sin(-r);
	const auto cr = cos(-r);

	if (forward != nullptr)
	{
		forward[0] = (float)(cp * cy);
		forward[1] = (float)(cp * sy);
		forward[2] = (float)-sp;
	}
	if (right != nullptr)
	{
		right[0] = (float)(-1.F * sr * sp * cy + -1 * cr * -sy);
		right[1] = (float)(-1.F * sr * sp * sy + -1 * cr * cy);
		right[2] = (float)(-1.F * sr * cp);
	}
	if (up != nullptr)
	{
		up[0] = (float)(cr * sp * cy + -sr * -sy);
		up[1] = (float)(cr * sp * sy + -sr * cy);
		up[2] = (float)(cr * cp);
	}
}

float Camera::vecDot(float* a, float* b) const
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
