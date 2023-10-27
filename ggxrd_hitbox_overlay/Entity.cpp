#include "pch.h"
#include "Entity.h"
#include "memoryFunctions.h"
#include "logging.h"

EntityManager entityManager;

bool EntityManager::onDllMain() {
	bool error = false;

	getPosX = (getPos_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x85\xC9\x75\x35\x8B\x8E",
		{-9},
		&error, "getPosX");

	getPosY = (getPos_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x75\x0A\x6A\x08\xE8",
		{ -0xB },
		&error, "getPosY");

	uintptr_t pushboxTopBottom = sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x0f\x57\xc0\xf3\x0f\x2a\xc0\xf3\x0f\x59\x05\x00\x00\x00\x01\xf3\x0f\x5c\xc8\xf3\x0f\x59\x4c\x24\x0c\xf3\x0f\x2c\xc1\x0f\x57\xc0\xf3\x0f\x2a\xc0\x8b\xce\xf3\x0f\x11\x44\x24\x10\xe8\x00\x00\x00\x00\x8b\xce\x8b\xf8\xe8\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x01",
		"xxxxxxxxxxx???xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxx???xxx???x",
		&error, "pushboxTopBottom");

	if (pushboxTopBottom) {
		logwrap(fprintf(logfile, "getPushboxTop relative call found at: %.8x\n", pushboxTopBottom + 0x2D));
		getPushboxTop = (getPushbox_t)followRelativeCall(pushboxTopBottom + 0x2C);
		logwrap(fprintf(logfile, "getPushboxTop final location at: %p\n", getPushboxTop));
		logwrap(fprintf(logfile, "getPushboxBottom relative call found at: %.8x\n", pushboxTopBottom + 0x36));
		getPushboxBottom = (getPushbox_t)followRelativeCall(pushboxTopBottom + 0x35);
		logwrap(fprintf(logfile, "getPushboxBottom final location at: %p\n", getPushboxBottom));
	}

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
		&& (*(unsigned int*)(ent + 0x234) & 0x40000000) == 0;  // this signals recovery frames and can be simultaneous with 0x100 flag in 0x23C
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

bool Entity::isDoingAThrow() const {
	const unsigned int flagsField = *(unsigned int*)(ent + 0x23C);
	const unsigned int throwFlagsField = *(unsigned int*)(ent + 0x9A4);
	return (flagsField & 0x1000) != 0
		|| (flagsField & 0x800) != 0 && (flagsField & 0x4000) != 0
		|| (throwFlagsField & 0x036F3E43) == 0x036F3E43;  // To find this field you could just compare to 0x036F3E43 exactly
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

char* Entity::operator+(int offset) const {
	return (char*)(ent + offset);
}

bool Entity::operator==(const Entity& other) const {
	return ent == other.ent;
}
