#include "pch.h"
#include "Entity.h"
#include "memoryFunctions.h"
#include "logging.h"
#include "EntityList.h"
#include <unordered_map>
#include <string>

EntityManager entityManager;

using isIdleHandler_t = bool(Entity::*)() const;

struct IsIdleHashEntry {
	CharacterType charType;
	const char* animName;
	bool operator==(const IsIdleHashEntry& other) const {
		return charType == other.charType && strcmp(animName, other.animName) == 0;
	}
};

static int hashString(const char* str, int startingHash = 0);

template <>
struct std::hash<IsIdleHashEntry>
{
	std::size_t operator()(const IsIdleHashEntry& k) const {
		return hashString(k.animName);
	}
};

static std::unordered_map<IsIdleHashEntry, isIdleHandler_t> idleHandlers;

// There's no convenient way to sigscan this and usually we do it the other way around:
// we use numbers like these to find stuff using sigscan. So we're hardcoding this and it's a decision that's made.
static char gutsTable[6][6] {     // We're NOT hardcoding guts rating of each char. Char names are here for info
	{ 100, 90, 76, 60, 50, 40 },  // Answer, Bedman, Elphelt, Faust, Zato
	{ 100, 87, 72, 58, 48, 40 },  // Axl, I-No, Ramlethal, Sin, Slayer, Sol, Venom, Dizzy
	{ 100, 84, 68, 56, 46, 38 },  // Ky, Haehyun, Jack-O'
	{ 100, 81, 66, 54, 44, 38 },  // Johnny, Leo, May, Millia, Potemkin, Jam
	{ 100, 78, 64, 50, 42, 38 },  // Baiken, Chipp
	{ 100, 75, 60, 48, 40, 36 }   // Raven
};

Entity::Entity() { }

bool EntityManager::onDllMain() {
	bool error = false;

	getPosX = (getPos_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"85 C9 75 35 8B 8E",
		{-9},
		&error, "getPosX");

	// ghidra sig: 
	getPosY = (getPos_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"75 0A 6A 08 E8",
		{ -0xB },
		&error, "getPosY");

	uintptr_t pushboxTopBottom = sigscanOffset(
		"GuiltyGearXrd.exe",
		"0f 57 c0 f3 0f 2a c0 f3 0f 59 05 ?? ?? ?? ?? f3 0f 5c c8 f3 0f 59 4c 24 0c f3 0f 2c c1 0f 57 c0 f3 0f 2a c0 8b ce f3 0f 11 44 24 10 e8 ?? ?? ?? ?? 8b ce 8b f8 e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ??",
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
		"99 2b c2 8b d8 8b ce d1 fb e8 ?? ?? ?? ?? 99 2b c2 8b e8 d1 fd 03 ae 2c 03 00 00 8b ce e8 ?? ?? ?? ?? 8b 4c 24 44 03 c7 89 01 8b ce e8 ?? ?? ?? ??",
		// the second ???? is the getPushboxTop, but it's a relative call and we don't know where this whole sig is so even knowing
		// getPushboxTop exact address we don't know what those bytes will be.
		// the third ???? is getPushboxBottom, also a relative call.
		&error, "getPushboxWidth");

	if (pushboxWidthUsage) {
		getPushboxWidth = (getPushbox_t)followRelativeCall(pushboxWidthUsage + 9);
		logwrap(fprintf(logfile, "getPushboxWidth final location at: %p\n", getPushboxWidth));
	}
	
	idleHandlers.insert( { {CHARACTER_TYPE_CHIPP, "HaritsukiKeep"}, &Entity::isIdleHaritsukiKeep });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "Souten"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "SoutenA"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "Souten9"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "Souten44"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "Souten66"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "SoutenB"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "SoutenC"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "SoutenE"}, &Entity::isIdleSouten });
	idleHandlers.insert( { {CHARACTER_TYPE_FAUST, "Souten8"}, &Entity::isIdleSouten8 });
	idleHandlers.insert( { {CHARACTER_TYPE_AXL, "DaiRensen"}, &Entity::isIdleDaiRensen });
	idleHandlers.insert( { {CHARACTER_TYPE_ELPHELT, "Rifle_Start"}, &Entity::isIdleRifle });
	idleHandlers.insert( { {CHARACTER_TYPE_ELPHELT, "Rifle_Reload"}, &Entity::isIdleRifle });
	idleHandlers.insert( { {CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Perfect"}, &Entity::isIdleRifle });
	idleHandlers.insert( { {CHARACTER_TYPE_ELPHELT, "Rifle_Reload_Roman"}, &Entity::isIdleRifle });
	idleHandlers.insert( { {CHARACTER_TYPE_LEO, "Semuke"}, &Entity::isIdleSemuke });
	idleHandlers.insert( { {CHARACTER_TYPE_JAM, "NeoHochihu"}, &Entity::isIdleNeoHochihu });
	idleHandlers.insert( { {CHARACTER_TYPE_ANSWER, "Ami_Hold"}, &Entity::isIdleAmi_Hold });
	idleHandlers.insert( { {CHARACTER_TYPE_ANSWER, "Ami_Move"}, &Entity::isIdleAmi_Move });
	
	uintptr_t tensionModsCall = sigscanOffset(
		"GuiltyGearXrd.exe",
		"0f af fe b8 1f 85 eb 51 f7 ef c1 fa 05 8b ca c1 e9 1f 03 ca 01 8b 34 d1 02 00",
		NULL, "tensionModsCall");
	if (tensionModsCall) {
		getExtraTensionModifier = (getExtraTensionModifier_t)followRelativeCall(tensionModsCall - 0x1b);
		logwrap(fprintf(logfile, "getExtraTensionModifier: %p\n", getExtraTensionModifier));
	}
	
	return !error;
}

Entity::Entity(const char* ent) : ent(ent) { }

int Entity::posX() const {
	return entityManager.getPosX(ent);
}

int Entity::posY() const {
	return entityManager.getPosY(ent);
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

	const auto otg = (*(unsigned int*)(ent + 0x4D24) & 0x800000) != 0;
	const auto invulnFrames = *(int*)(ent + 0x9A0);
	logOnce(fprintf(logfile, "invulnFrames: %x\n", invulnFrames));
	const auto invulnFlags = *(char*)(ent + 0x238);
	logOnce(fprintf(logfile, "invulnFlags: %x\n", invulnFlags));
	state->strikeInvuln = invulnFrames > 0 || (invulnFlags & 16) || (invulnFlags & 64) || state->doingAThrow || state->isGettingThrown;
	logOnce(fprintf(logfile, "strikeInvuln: %u\n", (int)state->strikeInvuln));
	state->throwInvuln = (invulnFlags & 32) || (invulnFlags & 64) || otg || state->inHitstunBlockstun;
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

bool Entity::isIdle() const {
	IsIdleHashEntry e;
	e.charType = characterType();
	e.animName = animationName();
	auto found = idleHandlers.find(e);
	if (found == idleHandlers.end()) {
		return enableNormals();
	}
	return (this->*found->second)();
}

// Chipp wall cling idle/moving up/down
bool Entity::isIdleHaritsukiKeep() const {
	return enableWhiffCancels();
}
// Faust pogo
bool Entity::isIdleSouten() const {
	return enableWhiffCancels();
}
// Faust pogo helicopter
bool Entity::isIdleSouten8() const {
	return !enableGatlings();
}
// Axl Haitaka stance
bool Entity::isIdleDaiRensen() const {
	return enableWhiffCancels();
}
// Elphelt Ms. Confille (rifle)
bool Entity::isIdleRifle() const {
	return enableWhiffCancels()
		&& (forceDisableFlags() & 0x2) == 0;  // 0x2 is the force disable flag for Rifle_Fire
}
// Leo backturn idle
bool Entity::isIdleSemuke() const {
	return true;
}
// Jam parry
bool Entity::isIdleNeoHochihu() const {
	return false;
}
// Answer scroll cling idle
bool Entity::isIdleAmi_Hold() const {
	return enableWhiffCancels();
}
// Answer s.D
bool Entity::isIdleAmi_Move() const {
	return enableWhiffCancels()
		&& hitstop() == 0;
}

int hashString(const char* str, int startingHash) {
	for (const char* c = str; *c != '\0'; ++c) {
		startingHash = startingHash * 0x89 + *c;
	}
	return startingHash;
}

void Entity::getWakeupTimings(CharacterType charType, WakeupTimings* output) {
	switch (charType) {
		case CHARACTER_TYPE_SOL:		*output = { 25, 21 }; break;
		case CHARACTER_TYPE_KY:			*output = { 23, 21 }; break;
		case CHARACTER_TYPE_MAY:		*output = { 25, 22 }; break;
		case CHARACTER_TYPE_MILLIA:		*output = { 25, 23 }; break;
		case CHARACTER_TYPE_ZATO:		*output = { 25, 22 }; break;
		case CHARACTER_TYPE_POTEMKIN:	*output = { 24, 22 }; break;
		case CHARACTER_TYPE_CHIPP:		*output = { 30, 24 }; break;
		case CHARACTER_TYPE_FAUST:		*output = { 25, 29 }; break;
		case CHARACTER_TYPE_AXL:		*output = { 25, 21 }; break;
		case CHARACTER_TYPE_VENOM:		*output = { 21, 26 }; break;
		case CHARACTER_TYPE_SLAYER:		*output = { 26, 20 }; break;
		case CHARACTER_TYPE_INO:		*output = { 24, 20 }; break;
		case CHARACTER_TYPE_BEDMAN:		*output = { 24, 30 }; break;
		case CHARACTER_TYPE_RAMLETHAL:	*output = { 25, 23 }; break;
		case CHARACTER_TYPE_SIN:		*output = { 30, 21 }; break;
		case CHARACTER_TYPE_ELPHELT:	*output = { 27, 27 }; break;
		case CHARACTER_TYPE_LEO:		*output = { 28, 26 }; break;
		case CHARACTER_TYPE_JOHNNY:		*output = { 25, 24 }; break;
		case CHARACTER_TYPE_JACKO:		*output = { 25, 23 }; break;
		case CHARACTER_TYPE_JAM:		*output = { 26, 25 }; break;
		case CHARACTER_TYPE_HAEHYUN:	*output = { 22, 27 }; break;
		case CHARACTER_TYPE_RAVEN:		*output = { 25, 24 }; break;
		case CHARACTER_TYPE_DIZZY:		*output = { 25, 24 }; break;
		case CHARACTER_TYPE_BAIKEN:		*output = { 25, 21 }; break;
		case CHARACTER_TYPE_ANSWER:		*output = { 25, 25 }; break;
	}
	*output = { 0, 0 };
}

void Entity::getWakeupTimings(WakeupTimings* output) {
	getWakeupTimings(characterType(), output);
}

int Entity::calculateGuts() {
	int maxHP = maxHp();
	int HP = hp();
	int gutsIndex = 0;
	if (HP <= maxHP * 10 / 100) {
		gutsIndex = 5;
	} else if (HP <= maxHP * 20 / 100) {
		gutsIndex = 4;
	} else if (HP <= maxHP * 30 / 100) {
		gutsIndex = 3;
	} else if (HP <= maxHP * 40 / 100) {
		gutsIndex = 2;
	} else if (HP <= maxHP * 50 / 100) {
		gutsIndex = 1;
	}
	return gutsTable[gutsRating()][gutsIndex];
}

int EntityManager::calculateExtraTensionGainModifier(void* pawn) {
	if (!getExtraTensionModifier) return 100;
	return getExtraTensionModifier(pawn, 3);
}

void EntityManager::calculateTensionGainModifier(
		int distance,
		int negativePenaltyTimer,
		int tensionPulse,
		int* distanceModifier,
		int* tensionPenaltyModifier,
		int* tensionPulseModifier) {
	if (distance < 0) distance = -distance;
	
	if (distance < 875000) {
		*distanceModifier = 100;
	} else if (distance >= 1312500) {
		*distanceModifier = 60;
	} else {
		*distanceModifier = 80;
	}
	
	if (negativePenaltyTimer) {
		*tensionPenaltyModifier = 20;
	} else {
		*tensionPenaltyModifier = 100;
	}
	
	if (tensionPulse < -12500) {
		*tensionPulseModifier = 25;
	} else if (tensionPulse < -7500) {
		*tensionPulseModifier = 50;
	} else if (tensionPulse < -3750) {
		*tensionPulseModifier = 75;
	} else if (tensionPulse < -1250) {
		*tensionPulseModifier = 90;
	} else if (tensionPulse < 1250) {
		*tensionPulseModifier = 100;
	} else if (tensionPulse < 5000) {
		*tensionPulseModifier = 125;
	} else {
		*tensionPulseModifier = 150;
	}
}

void EntityManager::calculateTensionPulsePenaltyGainModifier(
		int distance,
		int tensionPulse,
		int* distanceModifier,
		int* tensionPulseModifier) {
	if (distance < 0) distance = -distance;
	
	if (distance < 437500) {
		*distanceModifier = 50;
	} else if (distance >= 875000) {
		*distanceModifier = 150;
	} else {
		*distanceModifier = 100;
	}
	
	if (tensionPulse < -17500) {
		*tensionPulseModifier = 350;
	} else if (tensionPulse < -12500) {
		*tensionPulseModifier = 250;
	} else if (tensionPulse < -7500) {
		*tensionPulseModifier = 200;
	} else if (tensionPulse < -5000) {
		*tensionPulseModifier = 150;
	} else if (tensionPulse < -2500) {
		*tensionPulseModifier = 125;
	} else {
		*tensionPulseModifier = 100;
	}
	
}

char EntityManager::calculateTensionPulsePenaltySeverity(int tensionPulsePenalty) {
	if (tensionPulsePenalty >= 1080) {
		return 2;
	} else if (tensionPulsePenalty > 360) {
		return 1;
	} else {
		return 0;
	}
}

char EntityManager::calculateCornerPenaltySeverity(int cornerPenalty) {
	if (cornerPenalty >= 300) {
		return 2;
	} else if (cornerPenalty > 120) {
		return 1;
	} else {
		return 0;
	}
}

int EntityManager::calculateReceivedComboCountTensionGainModifier(bool inPain, int comboCount) {
	if (!inPain) return 400;
	return (comboCount + 17) * 100 / 16 * 4;
}

int EntityManager::calculateDealtComboCountTensionGainModifier(bool inPain, int comboCount) {
	if (!inPain) return 100;
	int n = comboCount + 8;
	if (n > 30) n = 30;
	else if (n < 8) n = 8;
	return (32 - n) * 100 / 32;
}
