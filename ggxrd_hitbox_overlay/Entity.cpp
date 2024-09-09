#include "pch.h"
#include "Entity.h"
#include "memoryFunctions.h"
#include "logging.h"
#include "EntityList.h"

EntityManager entityManager;

Entity::Entity() { }

bool EntityManager::onDllMain() {
	bool error = false;

	// ghidra sig: 85 C9 75 35 8B 8E
	char getPosXSig[] = "\x85\xC9\x75\x35\x8B\x8E";
	getPosX = (getPos_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		getPosXSig,
		_countof(getPosXSig),
		{-9},
		&error, "getPosX");

	// ghidra sig: 75 0A 6A 08 E8
	char getPosYSig[] = "\x75\x0A\x6A\x08\xE8";
	getPosY = (getPos_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		getPosYSig,
		_countof(getPosYSig),
		{ -0xB },
		&error, "getPosY");

	// ghidra sig: 0f 57 c0 f3 0f 2a c0 f3 0f 59 05 ?? ?? ?? ?? f3 0f 5c c8 f3 0f 59 4c 24 0c f3 0f 2c c1 0f 57 c0 f3 0f 2a c0 8b ce f3 0f 11 44 24 10 e8 ?? ?? ?? ?? 8b ce 8b f8 e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ??
	uintptr_t pushboxTopBottom = sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x0f\x57\xc0\xf3\x0f\x2a\xc0\xf3\x0f\x59\x05\x00\x00\x00\x01\xf3\x0f\x5c\xc8\xf3\x0f\x59\x4c\x24\x0c\xf3\x0f\x2c\xc1\x0f\x57\xc0\xf3\x0f\x2a\xc0\x8b\xce\xf3\x0f\x11\x44\x24\x10\xe8\x00\x00\x00\x00\x8b\xce\x8b\xf8\xe8\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x01",
		"xxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxx????xx????",
		&error, "pushboxTopBottom");

	if (pushboxTopBottom) {
		logwrap(fprintf(logfile, "getPushboxTop relative call found at: %.8x\n", pushboxTopBottom + 0x2D));
		getPushboxTop = (getPushbox_t)followRelativeCall(pushboxTopBottom + 0x2C);
		logwrap(fprintf(logfile, "getPushboxTop final location at: %p\n", getPushboxTop));
		logwrap(fprintf(logfile, "getPushboxBottom relative call found at: %.8x\n", pushboxTopBottom + 0x36));
		getPushboxBottom = (getPushbox_t)followRelativeCall(pushboxTopBottom + 0x35);
		logwrap(fprintf(logfile, "getPushboxBottom final location at: %p\n", getPushboxBottom));
	}

	// ghidra sig: 99 2b c2 8b d8 8b ce d1 fb e8 ?? ?? ?? ?? 99 2b c2 8b e8 d1 fd 03 ae 2c 03 00 00 8b ce e8 ?? ?? ?? ?? 8b 4c 24 44 03 c7 89 01 8b ce e8 ?? ?? ?? ??
	uintptr_t pushboxWidthUsage = sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x99\x2b\xc2\x8b\xd8\x8b\xce\xd1\xfb\xe8\x00\x00\x00\x00\x99\x2b\xc2\x8b\xe8\xd1\xfd\x03\xae\x2c\x03\x00\x00\x8b\xce\xe8\x00\x00\x00\x00\x8b\x4c\x24\x44\x03\xc7\x89\x01\x8b\xce\xe8\x00\x00\x00\x00",
		"xxxxxxxxxx????xxxxxxxxxxxxxxxx????xxxxxxxxxxx????",
		// the second ???? is the getPushboxTop, but it's a relative call and we don't know where this whole sig is so even knowing
		// getPushboxTop exact address we don't know what those bytes will be.
		// the third ???? is getPushboxBottom, also a relative call.
		&error, "getPushboxWidth");

	if (pushboxWidthUsage) {
		getPushboxWidth = (getPushbox_t)followRelativeCall(pushboxWidthUsage + 9);
		logwrap(fprintf(logfile, "getPushboxWidth final location at: %p\n", getPushboxWidth));
	}

	return !error;
}

Entity::Entity(const char* ent) : ent(ent) { }

bool Entity::isActive() const {
	return (*(unsigned int*)(ent + 0x23C) & 0x100) != 0  // this signals that attack's hitboxes are allowed to be active can happen before hitboxes come out
		&& (*(unsigned int*)(ent + 0x234) & 0x40000000) == 0;  // this signals recovery frames and can be simultaneous with 0x100 flag in 0x23C (recovery takes priority)
}

char Entity::team() const {
	return *(char*)(ent + 0x40);
}

CharacterType Entity::characterType() const {
	return (CharacterType)*(char*)(ent + 0x44);
}

int Entity::posX() const {
	return entityManager.getPosX(ent);
}

int Entity::posY() const {
	return entityManager.getPosY(ent);
}

bool Entity::isFacingLeft() const {
	return *(int*)(ent + 0x248) == 1;
}

void Entity::getState(EntityState* state) const {
	state->flagsField = *(unsigned int*)(ent + 0x23C);
	const unsigned int throwFlagsField = *(unsigned int*)(ent + 0x9A4);
	state->doingAThrow = (state->flagsField & 0x800) != 0 && (state->flagsField & 0x4000) != 0
		|| (throwFlagsField & 0x036F3E43) == 0x036F3E43;  // To find this field you could just compare to 0x036F3E43 exactly

	logOnce(fprintf(logfile, "doingAThrow: %d\n", (int)state->doingAThrow));
	state->isGettingThrown = isGettingThrown();
	logOnce(fprintf(logfile, "isGettingThrown: %d\n", (int)state->isGettingThrown));

	state->inHitstunBlockstun = *(unsigned int*)(ent + 0x9fE4) > 0;  // this is > 0 in hitstun, blockstun,
	// including 6f after hitstun, 5f after blockstun and 9f after wakeup

	state->posY = posY();
	state->prejumpFrames = false; ///*  *(unsigned int*)(ent + 0x234) != 0 && */ state->posY == 0;

	const auto otg = (*(unsigned int*)(ent + 0x4D24) & 0x800000) != 0;
	const auto invulnFrames = *(int*)(ent + 0x9A0);
	logOnce(fprintf(logfile, "invulnFrames: %x\n", invulnFrames));
	const auto invulnFlags = *(char*)(ent + 0x238);
	logOnce(fprintf(logfile, "invulnFlags: %x\n", invulnFlags));
	state->strikeInvuln = invulnFrames > 0 || (invulnFlags & 16) || (invulnFlags & 64) || state->doingAThrow || state->isGettingThrown;
	logOnce(fprintf(logfile, "strikeInvuln: %u\n", (int)state->strikeInvuln));
	state->throwInvuln = (invulnFlags & 32) || (invulnFlags & 64) || otg || state->inHitstunBlockstun || state->prejumpFrames;
	logOnce(fprintf(logfile, "throwInvuln: %u\n", (int)state->throwInvuln));
	state->charType = characterType();
	state->isASummon = state->charType == -1;
	state->ownerCharType = (CharacterType)(-1);
	state->team = team();
	if (state->isASummon && entityList.count >= 2) {
		state->ownerCharType = Entity{ entityList.slots[state->team] }.characterType();
	}
	logOnce(fprintf(logfile, "isASummon: %u\n", (int)state->isASummon));
	state->counterhit = (*(int*)(ent + 0x234) & 256) != 0  // Thanks to WorseThanYou for finding this
		&& !state->strikeInvuln
		&& !state->isASummon;
	logOnce(fprintf(logfile, "counterhit: %u\n", (int)state->counterhit));
}

bool Entity::isGettingThrown() const {
	const unsigned int flagsField = *(unsigned int*)(ent + 0x23C);
	return (*(unsigned int*)(ent + 0x43C) & 0xFF) != 0
		&& (flagsField & 0x800)
		&& (flagsField & 0x40000);
}

int Entity::pushboxWidth() const {
	return entityManager.getPushboxWidth(ent);
}

// Usually returns a positive number when pawn is on the ground
int Entity::pushboxTop() const {
	return entityManager.getPushboxTop(ent);
}

// Everyone always seems to have this value set to 0 no matter what
int Entity::pushboxFrontWidthOffset() const {
	return *(int*)(ent + 0x32C);
}

// Usually returns 0 when pawn is on the ground
int Entity::pushboxBottom() const {
	return entityManager.getPushboxBottom(ent);
}

void Entity::pushboxLeftRight(int* left, int* right) const {
	int flip = isFacingLeft() ? -1 : 1;
	int width = pushboxWidth() / 2;
	int frontOffset = pushboxFrontWidthOffset();
	int posX = this->posX();
	int back = posX - width * flip;
	int front = posX + (width + frontOffset) * flip;
	if (back < front) {
		*left = back;
		*right = front;
	} else {
		*left = front;
		*right = back;
	}
}

unsigned int Entity::currentAnimDuration() const {
	return *(const unsigned int*)(ent + 0x130);
}

char* Entity::operator+(int offset) const {
	return (char*)(ent + offset);
}

bool Entity::operator==(const Entity& other) const {
	return ent == other.ent;
}

Entity::operator bool() const {
	return ent != nullptr;
}

const char* Entity::animationName() const {
	return (const char*)(ent + 0x2444);
}
