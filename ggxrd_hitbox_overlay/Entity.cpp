#include "pch.h"
#include "Entity.h"
#include "memoryFunctions.h"
#include "logging.h"
#include "EntityList.h"
#include <string>

EntityManager entityManager;

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

static int normalComboProrationTable[] { 256, 200, 152, 112, 80, 48, 32, 16, 8, 8, 8 };
static int overdriveComboProrationTable[] { 256, 176, 128, 96, 80, 48, 40, 32, 24, 16, 16 };

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

int Entity::posX() const {
	return entityManager.getPosX(ent);
}

int Entity::posY() const {
	return entityManager.getPosY(ent);
}

void Entity::getState(EntityState* state, bool* wasSuperArmorEnabled, bool* wasFullInvul) const {
	state->flagsField = *(unsigned int*)(ent + 0x23C);
	const unsigned int throwFlagsField = *(unsigned int*)(ent + 0x9A4);  // this is super armor flags
	Entity presumablyThrownEntity = *(Entity*)(ent + 0x43c);
	
	// Blitz Shield rejection changes super armor enabled and full invul flags at the end of a logic tick
	bool isFullInvul = wasFullInvul ? *wasFullInvul : fullInvul();
	bool isSuperArmor = wasSuperArmorEnabled ? *wasSuperArmorEnabled : superArmorEnabled();
	
	state->doingAThrow = isPawn()  // isPawn check for Dizzy bubble pop
	&& (
		presumablyThrownEntity
		&& (*(DWORD*)(presumablyThrownEntity + 0x23c) & 0x800) != 0
		&& (*(DWORD*)(ent + 0x23c) & 0x1000) != 0
		&& !(
			// needed for Baiken Metsudo Kushodo
			*dealtAttack()->attackLockAction != '\0'
			&& strcmp(animationName(), dealtAttack()->attackLockAction) != 0
		)
		
		|| isSuperArmor
		&& superArmorType() == SUPER_ARMOR_DODGE
		&& !superArmorForReflect()  // needed for Dizzy mirror super
		&& (throwFlagsField & 0x036F3E43) == 0x036F3E43
	);
	
	logOnce(fprintf(logfile, "doingAThrow: %d\n", (int)state->doingAThrow));
	state->isGettingThrown = isGettingThrown();
	logOnce(fprintf(logfile, "isGettingThrown: %d\n", (int)state->isGettingThrown));

	state->inHitstunBlockstun = throwProtection();

	state->posY = posY();

	const auto otg = isOtg();
	const auto strikeIF = strikeInvulnFrames();
	const auto throwIF = throwInvulnFrames();
	logOnce(fprintf(logfile, "invulnFrames: %d, %d\n", strikeIF, throwIF));
	logOnce(fprintf(logfile, "invulnFlags: %x\n", *(DWORD*)(ent + 0x238)));
	state->strikeInvuln = strikeIF > 0
		|| strikeInvul()
		|| isFullInvul
		|| state->doingAThrow
		|| state->isGettingThrown
		|| clashHitstop();
	logOnce(fprintf(logfile, "strikeInvuln: %u\n", (int)state->strikeInvuln));
	state->throwInvuln = throwIF > 0
		|| throwInvul()
		|| isFullInvul
		//|| otg  // this turned out to not be the case, because OTG Blue Burst can be thrown
		|| state->inHitstunBlockstun
		|| clashHitstop()
		|| (
			damageToAir()
			|| setOnCmnActDownBoundEntry()
		) && !(state->posY != 0 || speedY() != 0);
	logOnce(fprintf(logfile, "throwInvuln: %u\n", (int)state->throwInvuln));
	state->superArmorActive = isSuperArmor
		&& !(
			isFullInvul
			|| strikeInvul()
			|| strikeIF > 0
			|| state->doingAThrow
			|| state->isGettingThrown
		)
		|| superArmorForReflect()
		&& (
			// you just get hit by the projectile if you try to reflect without armor/invul
			isSuperArmor
			|| strikeInvul()
			|| strikeIF > 0
			// full invul just makes you ignore interactions, you don't even get to the reflect part
		);
	state->charType = characterType();
	state->isASummon = state->charType == -1;
	state->ownerCharType = (CharacterType)(-1);
	state->team = team();
	if (state->isASummon && entityList.count >= 2) {
		state->ownerCharType = entityList.slots[state->team].characterType();
	}
	logOnce(fprintf(logfile, "isASummon: %u\n", (int)state->isASummon));
	state->counterhit = (*(int*)(ent + 0x234) & 256) != 0  // Thanks to WorseThanYou for finding this
		&& !state->strikeInvuln
		&& !state->isASummon;
	logOnce(fprintf(logfile, "counterhit: %u\n", (int)state->counterhit));
}

bool Entity::hasUpon(int index) const {
	int arrayIndex = index >> 5;
	int bitIndex = index & 31;
	int bitMask = 1 << bitIndex;
	return (*(DWORD*)(ent + 0xa0c + arrayIndex * 4) & bitMask) != 0;
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

void Entity::getWakeupTimings(CharacterType charType, WakeupTimings* output) {
	switch (charType) {
		case CHARACTER_TYPE_SOL:		*output = { 25, 21 }; return;
		case CHARACTER_TYPE_KY:			*output = { 23, 21 }; return;
		case CHARACTER_TYPE_MAY:		*output = { 25, 22 }; return;
		case CHARACTER_TYPE_MILLIA:		*output = { 25, 23 }; return;
		case CHARACTER_TYPE_ZATO:		*output = { 25, 22 }; return;
		case CHARACTER_TYPE_POTEMKIN:	*output = { 24, 22 }; return;
		case CHARACTER_TYPE_CHIPP:		*output = { 30, 24 }; return;
		case CHARACTER_TYPE_FAUST:		*output = { 25, 29 }; return;
		case CHARACTER_TYPE_AXL:		*output = { 25, 21 }; return;
		case CHARACTER_TYPE_VENOM:		*output = { 21, 26 }; return;
		case CHARACTER_TYPE_SLAYER:		*output = { 26, 20 }; return;
		case CHARACTER_TYPE_INO:		*output = { 24, 20 }; return;
		case CHARACTER_TYPE_BEDMAN:		*output = { 24, 30 }; return;
		case CHARACTER_TYPE_RAMLETHAL:	*output = { 25, 23 }; return;
		case CHARACTER_TYPE_SIN:		*output = { 30, 21 }; return;
		case CHARACTER_TYPE_ELPHELT:	*output = { 27, 27 }; return;
		case CHARACTER_TYPE_LEO:		*output = { 28, 26 }; return;
		case CHARACTER_TYPE_JOHNNY:		*output = { 25, 24 }; return;
		case CHARACTER_TYPE_JACKO:		*output = { 25, 23 }; return;
		case CHARACTER_TYPE_JAM:		*output = { 26, 25 }; return;
		case CHARACTER_TYPE_HAEHYUN:	*output = { 22, 27 }; return;
		case CHARACTER_TYPE_RAVEN:		*output = { 25, 24 }; return;
		case CHARACTER_TYPE_DIZZY:		*output = { 25, 24 }; return;
		case CHARACTER_TYPE_BAIKEN:		*output = { 25, 21 }; return;
		case CHARACTER_TYPE_ANSWER:		*output = { 25, 25 }; return;
	}
	*output = { 0, 0 };
}

void Entity::getWakeupTimings(WakeupTimings* output) const {
	getWakeupTimings(characterType(), output);
}

int Entity::calculateGuts(int* gutsLevel) const {
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
	if (gutsLevel) *gutsLevel = gutsIndex;
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

int EntityManager::calculateReceivedComboCountTensionGainModifier(bool inHitstun, int comboCount) {
	if (!inHitstun) return 100;
	return (comboCount + 17) * 100 / 16;
}

int EntityManager::calculateDealtComboCountTensionGainModifier(bool inHitstun, int comboCount) {
	if (!inHitstun) return 100;
	int n = comboCount + 8;
	if (n > 30) n = 30;
	else if (n < 8) n = 8;
	return (32 - n) * 100 / 32;
}

void EntityManager::calculatePushback(
		int attackLevel,
		int comboTimer,
		bool dontUseComboTimer,
		bool ascending,
		int y,
		int pushbackModifier,
		int airPushbackModifier,
		bool inHitstunOrInHitstunNextFrame,
		int pushbackModifierOnHitstun,
		int* basePushback,
		int* attackPushbackModifier,
		int* hitstunPushbackModifier,
		int* comboTimerPushbackModifier) {
	if (dontUseComboTimer) {
		comboTimer = 0;
	}
	
	static int pushbacksOnHit[]         { 1300, 1400, 1500, 1750, 2000, 2400, 3000 };
	static int pushbacksOnGroundBlock[] { 1250, 1375, 1500, 1750, 2000, 2400, 3000 };
	static int pushbacksOnAirBlock[]    {  800,  850,  900,  950, 1000, 2400, 3000 };
	
	if (basePushback) {
		if (inHitstunOrInHitstunNextFrame) {
			*basePushback = pushbacksOnHit[attackLevel];
		} else if (!ascending && y <= 0) {
			*basePushback = pushbacksOnGroundBlock[attackLevel];
		} else {
			*basePushback = pushbacksOnAirBlock[attackLevel];
		}
	}
	if (attackPushbackModifier) {
		*attackPushbackModifier = pushbackModifier;
		if ((ascending || y > 0) && airPushbackModifier != 0) {
			*attackPushbackModifier = airPushbackModifier;
		}
	}
	if (hitstunPushbackModifier) {
		*hitstunPushbackModifier = 100;
		if (inHitstunOrInHitstunNextFrame) {
			*hitstunPushbackModifier = pushbackModifierOnHitstun;
		}
	}
	if (comboTimerPushbackModifier) {
		if (comboTimer < 60) {
			*comboTimerPushbackModifier = 100;
		} else if (comboTimer < 120) {
			*comboTimerPushbackModifier = 106;
		} else if (comboTimer < 180) {
			*comboTimerPushbackModifier = 114;
		} else if (comboTimer < 240) {
			*comboTimerPushbackModifier = 124;
		} else if (comboTimer < 300) {
			*comboTimerPushbackModifier = 136;
		} else if (comboTimer < 360) {
			*comboTimerPushbackModifier = 150;
		} else if (comboTimer < 420) {
			*comboTimerPushbackModifier = 166;
		} else if (comboTimer < 480) {
			*comboTimerPushbackModifier = 184;
		} else {
			*comboTimerPushbackModifier = 200;
		}
	}
}

void EntityManager::calculateSpeedYProration(
		int comboCount,
		int weight,
		bool ignoreWeight,
		bool disableComboProtation,
		int* weightModifier,
		int* comboCountModifier) {
	int n = comboCount - 5;
	if (!disableComboProtation) {
		if (n < 0) {
			n = 0;
		} else if (n > 30) {
			n = 30;
		}
	} else {
		n = 0;
	}
	if (comboCountModifier) {
		*comboCountModifier = (60 - n) * 100 / 60;
	}
	if (weightModifier) {
		if (!ignoreWeight) {
			*weightModifier = weight;
		} else {
			*weightModifier = 100;
		}
	}
}

void EntityManager::calculateHitstunProration(
		bool noHitstunScaling,
		bool isInAir,
		int comboTimer,
		int* hitstunProration) {
	if (noHitstunScaling) {
		*hitstunProration = 100;
		return;
	}
	if (!isInAir) {
		if (comboTimer >= 1080) {
			*hitstunProration = 50;
		} else {
			*hitstunProration = 100;
		}
	} else if (comboTimer >= 1080) {
		*hitstunProration = 10;
	} else if (comboTimer >= 840) {
		*hitstunProration = 60;
	} else if (comboTimer >= 600) {
		*hitstunProration = 70;
	} else if (comboTimer >= 420) {
		*hitstunProration = 80;
	} else if (comboTimer >= 300) {
		*hitstunProration = 90;
	} else if (comboTimer >= 180) {
		*hitstunProration = 95;
	} else {
		*hitstunProration = 100;
	}
}

int EntityManager::calculateComboProration(int risc, AttackType attackType) {
	risc = risc / 100;
	if (risc >= 0) return 256;
	int* prorationTable;
	if ((int)attackType < 3) {
		prorationTable = normalComboProrationTable;
	} else {
		prorationTable = overdriveComboProrationTable;
	}
	risc = -risc - 1;
	int ind = risc >> 4;
	return (
			(prorationTable[ind] << 4) - (prorationTable[ind] - prorationTable[ind + 1]) * (risc & 15)
		) >> 4;
}

bool AddedMoveData::hasCondition(MoveCondition condition) const {
	int index = condition >> 5;
	return (conditions[index] & (1 << (condition & 31))) != 0;
}

BYTE* Entity::findFunctionStart(const char* name) const {
	DWORD hash = 0;
	for (const char* c = name; *c != '\0'; ++c) {
		char cVal = *c;
		if (cVal >= 'A' && cVal <= 'Z') cVal = 'a' + cVal - 'A';
		hash = hash * 0x89 + cVal;
	}
	
	int start = 0;
	int middle;
	const BBScrHashtable& table = *bbscrInfo()->functionStarts;
	int end = table.currentSize - 1;
	int found = -1;
	do {
		middle = (start + end) >> 1;
		DWORD currentHash = table.ptr[middle].hashValue;
		if (currentHash == hash) {
			found = table.ptr[middle].addressInCommands;
			break;
		} else if (currentHash < hash) {
			start = middle + 1;
		} else {
			end = middle - 1;
		}
	} while (start <= end);
	
	if (found == -1) return nullptr;
	return bbscrInfo()->afterJumptable + found;
}

int Entity::getCenterOffsetY() const {
	if (!isPawn()) return 0;
	if (crouching()) return 90000;
	if (lying()) return 40000;
	return 200000;
}
