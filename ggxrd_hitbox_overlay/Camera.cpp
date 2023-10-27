#include "pch.h"
#include "Camera.h"
#include "Game.h"
#include "pi.h"

Camera camera;

const float coordCoefficient = 0.42960999705207F;

void Camera::onEndSceneStart() {
	isSet = false;
}

void Camera::worldToScreen(IDirect3DDevice9* device, const D3DXVECTOR3& vec, D3DXVECTOR3* out) {
	setValues(device);

	D3DXVECTOR3 vecConverted{ convertCoord(vec.x), 0.F, convertCoord(vec.z) };

	D3DXVECTOR3 relativePos;
	D3DXVec3Subtract(&relativePos, &vecConverted, &pos);

	out->x = vecDot(relativePos, right);
	out->y = vecDot(relativePos, up);
	out->z = vecDot(relativePos, forward);

	out->x = floorf(clipXHalf - out->x * divisor / out->z + .5F);
	out->y = floorf(clipYHalf - out->y * divisor / out->z + .5F);
	out->z = 0.F;
}

void Camera::setValues(IDirect3DDevice9* device) {
	if (isSet) return;
	isSet = true;

	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);

	const char* cam = *(char**)(*aswEngine + 0x22e62c);

	pos.x = *(float*)(cam + 0x3C8);
	pos.y = *(float*)(cam + 0x3CC);
	pos.z = *(float*)(cam + 0x3D0);

	const float clipX = (float)(viewport.Width);
	const float clipY = (float)(viewport.Height);

	const auto pitch = (float)(*(int*)(cam + 0x3D4)) / 32768.F * PI;
	const auto yaw = (float)(*(int*)(cam + 0x3D8)) / 32768.F * PI;
	const auto roll = (float)(*(int*)(cam + 0x3DC)) / 32768.F * PI;

	const float fov = *(float*)(cam + 0x3E0);

	angleVectors(pitch, yaw, roll, forward, right, up);

	clipXHalf = (clipX / 2.F);
	clipYHalf = (clipY / 2.F);
	divisor = (clipXHalf / tan(fov * PI / 360.F));

}

// Arcsys engine to UE coords

float Camera::convertCoord(float in) const {
	return in / 1000.F * /*!*//*   *(float*)(*asw_engine + 0x3EA724)  */ coordCoefficient;  // not found, replacement value
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

float Camera::vecDot(float* a, float* b) const
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
