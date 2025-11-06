#include "pch.h"
#include "Entity.h"
#include "memoryFunctions.h"
#include "logging.h"
#include "EntityList.h"
#include <string>
#include <intrin.h>
#include "Game.h"

EntityManager entityManager;
extern char** aswEngine;

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
		GUILTY_GEAR_XRD_EXE,
		"85 C9 75 35 8B 8E",
		{-9},
		&error, "getPosX");

	// ghidra sig: 
	getPosY = (getPos_t)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"75 0A 6A 08 E8",
		{ -0xB },
		&error, "getPosY");

	uintptr_t getPushboxCoordsMiddle = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 46 48 8b 50 58 6a 05",
		&error, "getPushboxCoordsMiddle");
	if (getPushboxCoordsMiddle) {
		getPushboxCoords = (getPushboxCoords_t)sigscanBackwards16ByteAligned(getPushboxCoordsMiddle, "83 ec", 0xF0);
	}
	if (!getPushboxCoords) {
		error = true;
		logwrap(fprintf(logfile, "Failed to find getPushboxCoords\n"));
	}
	
	uintptr_t tensionModsCall = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
	bool isFullInvul = (wasFullInvul ? *wasFullInvul : fullInvul())
		|| inHitstunThisFrame()  // original game code checks for inHitstun this or next frame, but we check only this frame for clarity
		&& receivedAttack()->invulnTime != 0;
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

	state->inHitstunBlockstun = !jitabataLoop()  // these are from FUN_00deca70, called from FUN_00f6bcd0
		&& (throwProtection() > 0
		|| inHitstunThisFrame()  // game's original code checks hitstun this or next frame, but we check for hitstun on this frame only,
		                         // because it is way too confusing to see opponent's throw invul on the very frame they get thrown
		|| blockstun() > 0);

	state->posX = posX();
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
	state->counterhit = counterHitState()
		&& !state->strikeInvuln
		&& !state->isASummon;
	logOnce(fprintf(logfile, "counterhit: %u\n", (int)state->counterhit));
}

bool Entity::isGettingThrown() const {
	const unsigned int flagsField = *(unsigned int*)(ent + 0x23C);
	return *(unsigned int*)(ent + 0x43C) != 0
		&& (flagsField & 0x800)
		//&& (flagsField & 0x40000);  // determineHitType does not check for this, 0x800 is enough
		&& inHitstunThisFrame(); // ignore the frame when the throw first connected
}

void Entity::pushboxDimensions(int* left, int* top, int* right, int* bottom) const {
	entityManager.getPushboxCoords((const void*)ent, left, top, right, bottom);
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

// risc is [-12800;+12800]
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

static BYTE* findInBBScrHashmap(const char* name, const BBScrInfo* bbscrInfo, const BBScrHashtable* table) {
	
	const DWORD hash = Entity::hashStringLowercase(name);
	
	int start = 0;
	int middle;
	int end = table->currentSize - 1;
	int found = -1;
	do {
		middle = (start + end) >> 1;
		DWORD currentHash = table->ptr[middle].hashValue;
		if (currentHash == hash) {
			found = table->ptr[middle].addressInCommands;
			break;
		} else if (currentHash < hash) {
			start = middle + 1;
		} else {
			end = middle - 1;
		}
	} while (start <= end);
	
	if (found == -1) return nullptr;
	return bbscrInfo->afterJumptable + found;
}

BYTE* Entity::findStateStart(const char* name) const {
	return findInBBScrHashmap(name, bbscrInfo(), bbscrInfo()->stateMap);
}

BYTE* Entity::findSubroutineStart(const char* name) const {
	return findInBBScrHashmap(name, bbscrInfo(), bbscrInfo()->subroutineMap);
}

int Entity::getCenterOffsetY() const {
	if (!isPawn()) return 0;
	if (crouching()) return 90000;
	if (lying()) return 40000;
	return 200000;
}

const AddedMoveData* Entity::findAddedMove(const char* name) const {
	const CmnActHashtable* hashtable = addedMovesHashtable();
	for (
		unsigned short index = hashtable->hashMap[hashString(name) % _countof(hashtable->hashMap)];
		index != 0xFFFF;
		index = hashtable->next[index]
	) {
		if (strcmp(hashtable->strings[index], name) == 0) return movesBase() + index;
	}
	return nullptr;
}

DWORD Entity::hashStringLowercase(const char* str) {
	DWORD hash = 0;
	for (const char* c = str; *c != '\0'; ++c) {
		char cVal = *c;
		if (cVal >= 'A' && cVal <= 'Z') cVal = 'a' + cVal - 'A';
		hash = hash * 0x89 + cVal;
	}
	return hash;
}

DWORD Entity::hashString(const char* str) {
	DWORD hash = 0;
	for (const char* c = str; *c != '\0'; ++c) {
		hash = hash * 0x89 + *c;
	}
	return hash;
}

int FPAC::calcJonbSize(BYTE* dataPtr) {
	if (memcmp(dataPtr, "JONB", 4) != 0) return 0xFF773311;
	short nameCount = *(short*)(dataPtr + 4);
	dataPtr += 4 + 2 + 32 * nameCount;
	short typeCount = *(BYTE*)dataPtr - 3;
	++dataPtr;
	short short1 = *(short*)dataPtr;
	dataPtr += 2;
	short short2 = *(short*)dataPtr;
	dataPtr += 2;
	short short3 = *(short*)dataPtr;
	dataPtr += 2;
	short totalCount = 0;
	for (int i = 0; i < typeCount; ++i) {
		totalCount += *(short*)dataPtr;
		dataPtr += 2;
	}
	return 4  // "JONB"
		+ 2  // name count
		+ 32 * nameCount
		+ 1  // hitbox type count + 3
		+ 2  // short1
		+ 2  // short2
		+ 2  // short3
		+ 2 * typeCount  // count of hitboxes, for each type
		+ short1 * size1
		+ short2 * size2
		+ short3 * size3
		+ (sizeof(Hitbox)) * totalCount;
}

void FPAC::enumNames(bool (*callback)(char* name, BYTE* jonbin)) {
	// flag2 we only know how to find name by hash and get that entry's data
	if (useHash()) {
		if (flag2()) {
			return;
			for (DWORD i = 0; i < count; ++i) {
				//oops idk where the name is callback(data.table0x10.elements[i].
			}
		} else if (size0x50()) {
			for (DWORD i = 0; i < count; ++i) {
				FPACLookupElement0x50& elem = data.table0x50.elements[i];
				if (!callback(elem.spriteName, (BYTE*)this + headerSize + elem.offset)) return;
			}
		} else {
			for (DWORD i = 0; i < count; ++i) {
				FPACLookupElement0x30& elem = data.table0x30.elements[i];
				if (!callback(elem.spriteName, (BYTE*)this + headerSize + elem.offset)) return;
			}
		}
	} else if (flag2()) {
		return;
	} else {
		DWORD uVar2 = sizeOfSingleElement + 0xc;  // idk
		// this pads the uVar2 up to the nearest 16-byte boundary
		uVar2 = (uVar2 - (uVar2 & 0xf)) + 0x10;
		
		for (DWORD i = 0; i < count; ++i) {
			if (
				!callback(
					(char*)&data + uVar2 * i,
					(BYTE*)this + headerSize + *(int*)(
						(BYTE*)&data + uVar2 * i + sizeOfSingleElement + 4
					)
				)
			) {
				return;
			}
		}
	}
	return;
}

REDAnimNodeSequence* REDPawn::getFirstAnimSeq() {
	for (int i = 0; i < MeshControlNum(); ++i) {
		REDAnimNodeSequence* result = MeshControls()[i].AnimSeq;
		if (result) {
			return result;
		}
	}
	return nullptr;
}

template<typename T>
static inline DWORD findInsertionIndex(T* array, int count, DWORD hash) {
	int start = 0;
	int end = (int)count - 1;
	while (true) {
		int mid = (start + end) >> 1;
		DWORD curHash = array[mid].hash;
		if (hash == curHash) {
			return 0xFFFFFFFF;
		} else if (hash > curHash) {
			start = mid + 1;
			if (mid == end) {
				return (DWORD)start;
			}
		} else {
			end = mid - 1;
			if (mid == start) {
				return (DWORD)start;
			}
		}
	}
}

DWORD FPAC::findInsertionIndex(DWORD hash) {
	if (!count) return 0;
	if (size0x50()) {
		return ::findInsertionIndex(data.table0x50.elements, count, hash);
	} else {
		return ::findInsertionIndex(data.table0x30.elements, count, hash);
	}
}

template<typename T>
static BYTE* findLookupEntry(T* array, int count, DWORD hash) {
	int start = 0;
	int end = (int)count - 1;
	while (true) {
		int mid = (start + end) >> 1;
		DWORD curHash = array[mid].hash;
		if (hash == curHash) {
			return (BYTE*)&array[mid];
		} else if (hash > curHash) {
			start = mid + 1;
			if (mid == end) {
				return nullptr;
			}
		} else {
			end = mid - 1;
			if (mid == start) {
				return nullptr;
			}
		}
	}
}

BYTE* FPAC::findLookupEntry(DWORD hash) {
	if (!count) return nullptr;
	if (size0x50()) {
		return ::findLookupEntry(data.table0x50.elements, count, hash);
	} else {
		return ::findLookupEntry(data.table0x30.elements, count, hash);
	}
}

BYTE* FPAC::findLookupEntry(const char* str) {
	return findLookupEntry(Entity::hashStringLowercase(str));
}

template<typename T>
void FPACInsertAtImpl(T* array, DWORD index, DWORD count, const T& newElement){
	if (!count) {
		array[0] = newElement;
		return;
	}
	for (DWORD i = count - 1; i >= index; --i) {
		array[i + 1] = array[i];
		++array[i + 1].index;
	}
	array[index] = newElement;
}

void FPACLookupTable0x30::insertAt(DWORD index, DWORD count, const FPACLookupElement0x30& newElement) {
	FPACInsertAtImpl(elements, index, count, newElement);
}

void FPACLookupTable0x50::insertAt(DWORD index, DWORD count, const FPACLookupElement0x50& newElement) {
	FPACInsertAtImpl(elements, index, count, newElement);
}

REDAssetCollision* Entity::getCollision() const {
	REDPawn_Player** ar = (REDPawn_Player**)(*aswEngine + aswEnginePawnsOffset);
	return ar[bbscrIndexInAswEng()]->Collision();
}

static BYTE* sizeOfJonbinAtOffsetTarget;
static BYTE* sizeOfJonbinAtOffsetEntryOffsetPtr;
static DWORD sizeOfJonbinAtOffsetSize;

template<typename T>
static bool sizeOfJonbinAtOffsetImpl(char* name, BYTE* jonbin) {
	if (jonbin == sizeOfJonbinAtOffsetTarget) {
		sizeOfJonbinAtOffsetSize = ((T*)name)->size;
		sizeOfJonbinAtOffsetEntryOffsetPtr = (BYTE*)name + offsetof(T, offset);
		return false;
	}
	return true;
}

DWORD FPAC::sizeOfJonbinAtOffset(DWORD offset, DWORD* offsetPtrOffset) {
	sizeOfJonbinAtOffsetSize = 0xFFFFFFFF;
	sizeOfJonbinAtOffsetTarget = (BYTE*)this + headerSize + offset;
	if (size0x50()) {
		enumNames(sizeOfJonbinAtOffsetImpl<FPACLookupElement0x50>);
	} else {
		enumNames(sizeOfJonbinAtOffsetImpl<FPACLookupElement0x30>);
	}
	*offsetPtrOffset = sizeOfJonbinAtOffsetEntryOffsetPtr - (BYTE*)this;
	return sizeOfJonbinAtOffsetSize;
}

int Entity::getEffectIndex() const {
	return (
		ent - (*aswEngine + 0x4 + 0x460)  // an in-place array of 75 Effects is here
	) / 0x4d10;  // size of 1 Effect;
}

BYTE* FPAC::lookupEnd() const {
	return (BYTE*)this + 0x20 + count * elementSize();
}

FName FName::nullFName { 0, 0 };

Hitbox* HitboxHolder::hitboxesStart() const {
	return hitboxesStart(jonbinPtr);
}

Hitbox* HitboxHolder::hitboxesStart(BYTE* jonbinPtr) {
	BYTE* ptr = jonbinPtr;
	ptr += 4;  // skip "JONB"
	short nameCount = *(short*)ptr;
	ptr += 2;
	ptr += nameCount * 32;
	BYTE numTypes = *ptr - 3;
	++ptr;
	short short1 = *(short*)ptr;
	ptr += 2;
	short short2 = *(short*)ptr;
	ptr += 2;
	short short3 = *(short*)ptr;
	ptr += 2;
	return (Hitbox*)(
		ptr + numTypes * 2 + short1 * FPAC::size1 + short2 * FPAC::size2 + short3 * FPAC::size3
	);
}

short* HitboxHolder::hitboxCounts(BYTE* jonbinPtr) {
	BYTE* ptr = jonbinPtr;
	ptr += 4;  // skip "JONB"
	short nameCount = *(short*)ptr;
	return (short*)(ptr + 2 + nameCount * 32 + 1 + 2 + 2 + 2);
}

BYTE HitboxHolder::numTypes(BYTE* jonbinPtr) {
	BYTE* ptr = jonbinPtr;
	ptr += 4;  // skip "JONB"
	short nameCount = *(short*)ptr;
	ptr += 2 + nameCount * 32;
	return *ptr - 3;
}

int HitboxHolder::hitboxCount() const {
	int result = 0;
	for (int i = 0; i < 17; ++i) {
		result += count[i];
	}
	return result;
}

void HitboxHolder::parse(BYTE* jonbinPtr) {
	this->jonbinPtr = jonbinPtr;
	
	BYTE* ptr = jonbinPtr;
	ptr += 4;  // skip "JONB"
	nameCount = *(short*)ptr;
	ptr += 2;
	for (int i = 0; i < nameCount; ++i) {
		names[i] = (char*)ptr;
		ptr += 32;
	}
	BYTE numTypes = *ptr - 3;
	++ptr;
	short short1 = *(short*)ptr;
	ptr += 2;
	short2 = *(short*)ptr;
	ptr += 2;
	short short3 = *(short*)ptr;
	ptr += 2;
	
	for (BYTE type = 0; type < numTypes; ++type) {
		count[type] = *(short*)ptr;
		ptr += 2;
	}
	
	if (numTypes < 17) {
		memset(count + numTypes, 0, 2 * (17 - numTypes));
	}
	
	ptr += short1 * FPAC::size1;
	ptrRawAfterShort1 = ptr;
	
	ptr += short2 * FPAC::size2 + short3 * FPAC::size3;
	
	for (BYTE type = 0; type < numTypes; ++type) {
		data[type] = (Hitbox*)ptr;
		ptr += count[type] * sizeof (Hitbox);
	}
	
}

static int wideStrIntoStrbuf(const wchar_t* widePtr, char* buf, size_t bufSize) {
	char* ptr = buf;
	if (!bufSize) return 0;
	--bufSize;  // reserve the last character for null
	for (; *widePtr != L'\0' && bufSize > 0; ++widePtr) {
		char whoahwhoahholdontherepal = *(char*)widePtr;
		if (whoahwhoahholdontherepal < 0 || whoahwhoahholdontherepal > 126 || *((char*)widePtr + 1) != 0) {
			break;  // Worse Than You got me paranoid of malcoded UTF as an attack vector
		}
		*ptr = *(char*)widePtr;
		++ptr;
		--bufSize;
	}
	*ptr = '\0';
	return ptr - buf;
}

char* FName::print(char* buf, size_t size) const {
	if (!low) {
		if (size) {
			buf[0] = '\0';
		}
		return nullptr;
	}
	bool isWide;
	const char* data = game.readFName(low, &isWide);
	if (!high) {
		if (!isWide) {
			strcpy_s(buf, size, data);
		} else {
			wideStrIntoStrbuf((const wchar_t*)data, buf, size);
		}
	} else {
		if (!isWide) {
			sprintf_s(buf, size, "%s%d", data, high - 1);
		} else {
			int count = wideStrIntoStrbuf((const wchar_t*)data, buf, size);
			// 12, because 11 is the longest int you can print with %d, plus null character
			if (count + 12 < (int)size) {
				sprintf_s(buf + count, size - (size_t)count, "%d", high - 1);
			}
		}
	}
	return buf;
}

template<typename Key, typename Value>
int TMap<Key,Value>::find(Key* key) const {
	int HashSize = Pairs.HashSize;
	if (HashSize != 0) {
		DWORD mask = (DWORD)HashSize - 1;
		int* HashData;
		if (Pairs.Hash.Data) {
			HashData = Pairs.Hash.Data;
		} else {
			HashData = Pairs.Hash.InlineData;
		}
		typedef TSet<TMap<Key,Value>::FPair>::FElement unconfuseVisualStudio;  // if you write this in-place, the code won't compile, because ElementData: identifier not found. What the hell is happening inside the compiler or C++whatever-number specification??
		unconfuseVisualStudio* ElementData = Pairs.Elements.Data.Data;
		for (int HashNext = HashData[mask & hash(key)]; HashNext != -1; HashNext = ElementData[HashNext].HashNextId) {
			if (ElementData[HashNext].Value.Key == key) {
				return HashNext;
			}
		}
	}
	return -1;
}

UAnimSequence* UAnimSet::find(FName name) const {
	for (int i = 0; i < Sequences.ArrayNum; ++i) {
		UAnimSequence* element = Sequences.Data[i];
		if (element->SequenceName == name) {
			return element;
		}
	}
	return nullptr;
}

UAnimSequence* USkeletalMeshComponent::find(FName name) const {
	for (int i = AnimSets.ArrayNum - 1; i >= 0; --i) {
		const UAnimSet* set = AnimSets.Data[i];
		if (!set) continue;
		UAnimSequence* found = set->find(name);
		if (found) return found;
	}
	return nullptr;
}

int REDPawn::getMaxFrameOfAnimSequence(FName name) {
	int numControls = MeshControlNum();
	const MeshControl* meshControl = MeshControls();
	for (int i = 0; i < numControls; ++i) {
		UAnimSequence* animSeq = meshControl->find(name);
		if (animSeq) {
			BYTE* vtable = *(BYTE**)meshControl->AnimSeq;  // read vtable
			typedef float (__thiscall * GetAnimPlaybackLength_t) (UAnimNodeSequence* thisArg);
			GetAnimPlaybackLength_t GetAnimPlaybackLength = *(GetAnimPlaybackLength_t*)(vtable + 0x1e4);
			UAnimSequence* currentSeq = meshControl->AnimSeq->AnimSeq;
			float playbackLength = GetAnimPlaybackLength(meshControl->AnimSeq);
			playbackLength = playbackLength * currentSeq->RateScale() / currentSeq->SequenceLength()
				* animSeq->SequenceLength() / animSeq->RateScale();
			return -1 - (int)(
				(
					playbackLength * 60.F + 0.04166667F
				) * -0.2F
			);
		}
		++meshControl;
	}
	return 0;
}
