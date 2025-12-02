#pragma once
#include "EndScene.h"
#include "EndSceneStoredState.h"

extern const unsigned char greenHighlights[21];
extern const PunishFrame punishAnim[46];

extern bool colorIsLess(DWORD colorProtagonist, DWORD colorToCompareAgainst);
extern bool leoHasbtDAvailable(Entity pawn);
extern void increaseFramesCountUnlimited(int& counterUnlimited, int incrBy, int displayedFrames);

static inline bool isDizzyBubble(const char* name) {
	return (*(DWORD*)name & 0xffffff) == ('A' | ('w'<<8) | ('a'<<16))
		&& *(DWORD*)(name + 4) == ('O' | ('b'<<8) | ('j'<<16));
}
static inline bool isGrenadeBomb(const char* name) {
	return *(DWORD*)name == ('G' | ('r'<<8) | ('e'<<16) | ('n'<<24))
		&& *(DWORD*)(name + 4) == ('a' | ('d'<<8) | ('e'<<16) | ('B'<<24))
		&& *(DWORD*)(name + 8) == ('o' | ('m'<<8) | ('b'<<16));
}
static inline bool isVenomBall(const char* name) {
	return *(DWORD*)name == ('B' | ('a'<<8) | ('l'<<16) | ('l'<<24))
		&& *(BYTE*)(name + 4) == 0;
}

template<typename T>
static inline const T& max_inline(const T& a, const T& b) {
	return a > b ? a : b;
}

template<typename T>
static inline const T& min_inline(const T& a, const T& b) {
	return a < b ? a : b;
}

template<typename T>
static inline const T& minmax(const T& Min, const T& Max, const T& Value) {
	if (Value < Min) return Min;
	if (Value > Max) return Max;
	return Value;
}

extern PlayerInfo emptyPlayer;
extern ProjectileInfo emptyProjectile;
extern ProjectileFramebar defaultFramebar;
