#pragma once
#include "characterTypes.h"
#include <unordered_map>
#include "Entity.h"
#include <vector>

struct PlayerInfo;

using sectionSeparator_t = bool(*)(PlayerInfo& ent);
using sectionSeparatorProjectile_t = bool(*)(Entity ent);
using isIdle_t = bool(*)(PlayerInfo& ent);
using isDangerous_t = bool(*)(Entity ent);
using selectFramebarName_t = const char*(*)(Entity ent);
using zatoHoldLevel_t = DWORD(*)(PlayerInfo& ent);
using selectDisplayName_t = const char*(*)(PlayerInfo& ent);

bool isIdle_default(PlayerInfo& player);
bool canBlock_default(PlayerInfo& player);
bool isDangerous_default(Entity ent);

class ForceAddedWhiffCancel {
public:
	const char* name;
	int getMoveIndex(Entity ent);
	void clearCachedValues();
private:
	int moveIndexPerPlayer[2];
};

struct MoveInfoProperty {
	DWORD type;
	union {
		bool boolValue;
		const char* strValue;
		sectionSeparator_t sectionSeparatorValue;
		sectionSeparatorProjectile_t sectionSeparatorProjectileValue;
		int intValue;
		isIdle_t isIdleValue;
		isDangerous_t isDangerousValue;
		selectFramebarName_t selectFramebarNameValue;
		selectDisplayName_t selectDisplayNameValue;
		zatoHoldLevel_t zatoHoldLevelValue;
	} u;
};

struct MoveInfoStored {
	union {
		int startInd;
		MoveInfoProperty* startPtr;
	};
	int count = 0;
};

#define MOVE_INFO_PROPERTY_TABLE \
	MOVE_INFO_EXEC(bool, boolValue, combineWithPreviousMove, false) \
	/* This is needed for Johnny becoming able to use Mist Finer from his MistFinerLoop animation.
	 We want to write X+possibly infinite frames as long as you're holding, where X is the startup of
	 the availability of Mist Finer attack.
*/	MOVE_INFO_EXEC(bool, boolValue, usePlusSignInCombination, false) \
	MOVE_INFO_EXEC(const char*, strValue, displayName, nullptr) \
	MOVE_INFO_EXEC(const char*, strValue, slangName, nullptr) \
	/* A section is what I call separating frames with a + sign in the startup, recovery or total display.
	This is useful for some moves that can be held or charged, because if you treat the part of the
	animation that starts after you release the button as separate and show it with a "frames from before
	that part" + "frames after that part", then you will be able to tell what the startup of the move is
	after you release the button
*/	MOVE_INFO_EXEC(sectionSeparator_t, sectionSeparatorValue, sectionSeparator, nullptr) \
	MOVE_INFO_EXEC(sectionSeparatorProjectile_t, sectionSeparatorProjectileValue, sectionSeparatorProjectile, nullptr) \
	/* If PlayerInfo::inNewMoveSection == true for this many frames, the player is considered 'idle' in all respects.
*/	MOVE_INFO_EXEC(int, intValue, considerIdleInSeparatedSectionAfterThisManyFrames, 0) \
	/* This is needed for Johnny walking back and forth in Mist Finer because he can start walking
	as soon as he can do Mist Finer attack, but we still want the new section because we treat
	walking same way as standing in Mist Finer
*/	MOVE_INFO_EXEC(bool, boolValue, preservesNewSection, false) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, isIdle, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, canBlock, nullptr) \
	MOVE_INFO_EXEC(isDangerous_t, isDangerousValue, isDangerous, nullptr) \
	MOVE_INFO_EXEC(int, intValue, framebarId, -1) \
	MOVE_INFO_EXEC(const char*, strValue, framebarName, nullptr) \
	MOVE_INFO_EXEC(const char*, strValue, framebarNameUncombined, nullptr) \
	MOVE_INFO_EXEC(const char*, strValue, framebarSlangNameUncombined, nullptr) \
	MOVE_INFO_EXEC(const char*, strValue, framebarSlangName, nullptr) \
	MOVE_INFO_EXEC(const char*, strValue, framebarNameFull, nullptr) \
	MOVE_INFO_EXEC(selectFramebarName_t, selectFramebarNameValue, framebarNameSelector, nullptr) \
	MOVE_INFO_EXEC(selectFramebarName_t, selectFramebarNameValue, framebarSlangNameSelector, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, isInVariableStartupSection, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, canStopHolding, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, aSectionBeforeVariableStartup, nullptr) \
	MOVE_INFO_EXEC(bool, boolValue, considerNewSectionAsBeingInVariableStartup, false) \
	MOVE_INFO_EXEC(bool, boolValue, considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot, false) \
	MOVE_INFO_EXEC(bool, boolValue, considerVariableStartupAsStanceForFramebar, false) \
	MOVE_INFO_EXEC(bool, boolValue, canBeUnableToBlockIndefinitelyOrForVeryLongTime, false) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, isRecoveryHasGatlings, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, isRecoveryCanAct, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, canFaultlessDefend, nullptr) \
	MOVE_INFO_EXEC(bool, boolValue, nameIncludesInputs, false) \
	MOVE_INFO_EXEC(bool, boolValue, ignoresHitstop, false) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, frontLegInvul, nullptr) \
	MOVE_INFO_EXEC(int, intValue, forceAddWhiffCancelsStart, 0) \
	MOVE_INFO_EXEC(int, intValue, forceAddWhiffCancelsCount, 0) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, isRecoveryCanReload, nullptr) \
	MOVE_INFO_EXEC(const char*, strValue, onlyAddForceWhiffCancelsOnFirstFrameOfSprite, nullptr) \
	MOVE_INFO_EXEC(zatoHoldLevel_t, zatoHoldLevelValue, zatoHoldLevel, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, conditionForAddingWhiffCancels, nullptr) \
	MOVE_INFO_EXEC(bool, boolValue, caresAboutWall, false) \
	MOVE_INFO_EXEC(bool, boolValue, faustPogo, false) \
	MOVE_INFO_EXEC(bool, boolValue, butForFramebarDontCombineWithPreviousMove, false) \
	MOVE_INFO_EXEC(const char*, strValue, replacementInputs, nullptr) \
	MOVE_INFO_EXEC(int, intValue, replacementBufferTime, 0) \
	MOVE_INFO_EXEC(const char*, strValue, whiffCancelsNote, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, secondaryStartup, nullptr) \
	MOVE_INFO_EXEC(bool, boolValue, forceLandingRecovery, false) \
	MOVE_INFO_EXEC(bool, boolValue, isGrab, false) \
	MOVE_INFO_EXEC(bool, boolValue, partOfStance, false) \
	MOVE_INFO_EXEC(bool, boolValue, dontSkipSuper, false) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, iKnowExactlyWhenTheRecoveryOfThisMoveIs, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, forceSuperHitAnyway, nullptr) \
	MOVE_INFO_EXEC(bool, boolValue, drawProjectileOriginPoint, false) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, canYrcProjectile, nullptr) \
	MOVE_INFO_EXEC(selectDisplayName_t, selectDisplayNameValue, displayNameSelector, nullptr) \
	MOVE_INFO_EXEC(selectDisplayName_t, selectDisplayNameValue, displaySlangNameSelector, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, createdProjectile, nullptr) \
	MOVE_INFO_EXEC(isIdle_t, isIdleValue, powerup, nullptr) \
	MOVE_INFO_EXEC(bool, boolValue, isEddie, false) \
	MOVE_INFO_EXEC(bool, boolValue, dontSkipGrab, false)

struct MoveInfo {
	CharacterType charType;
	const char* name;
	bool isEffect;
	
	#define MOVE_INFO_EXEC(type, prop, name, defaultValue) type name = defaultValue;
	MOVE_INFO_PROPERTY_TABLE
	#undef MOVE_INFO_EXEC
	
	inline MoveInfo() : isIdle(isIdle_default), canBlock(canBlock_default) { }
	MoveInfo(const MoveInfoStored& info);
	inline MoveInfo(CharacterType charType, const char* name, bool isEffect = false) : charType(charType), name(name), isEffect(isEffect) { }
	const char* getFramebarName(Entity ent) const;
	void addForceAddWhiffCancel(const char* name);
	ForceAddedWhiffCancel* getForceAddWhiffCancel(int index) const;
	inline const char* getDisplayName(PlayerInfo& ent) const { return displayNameSelector ? displayNameSelector(ent) : displayName; }
	inline const char* getDisplayNameSlang(PlayerInfo& ent) const { return displaySlangNameSelector ? displaySlangNameSelector(ent) : slangName; }
};
class Moves {
public:
	bool onDllMain();
	bool getInfo(MoveInfo& returnValue, CharacterType charType, const char* moveName, const char* stateName, bool isEffect);
	bool getInfo(MoveInfo& returnValue, CharacterType charType, const char* name, bool isEffect);
	void onAswEngineDestroyed();
	MoveInfo defaultMove{ };
	unsigned short* bbscrInstructionSizes = nullptr;
	inline BYTE* skipInstruction(BYTE* in) const;
	enum InstructionType {
		instr_endState = 1,
		instr_sprite = 2,
		instr_spriteEnd = 3,
		instr_ifOperation = 6,
		instr_setMarker = 11,
		instr_exitState = 18,
		instr_clearUpon = 23,
		instr_storeValue = 46,
		instr_createObjectWithArg = 445,
		instr_createObject = 446,
		instr_deleteMoveForceDisableFlag = 1603,
		instr_exPointFReset = 2161,
	};
	inline InstructionType instructionType(BYTE* in) const;
	BYTE* findSetMarker(BYTE* in, const char* name) const;
	BYTE* findNextMarker(BYTE* in, const char** name) const;
	BYTE* findCreateObj(BYTE* in, const char* name) const;
	BYTE* findSprite(BYTE* in, const char* name) const;
	inline BYTE* findSpriteNull(BYTE* in) const { return findSprite(in, "null"); }
	BYTE* findSpriteNonNull(BYTE* in) const;
	int armorDanceEndOffset = 0;  // in number of bytes
	int armorDance2EndOffset = 0;
	int saishingeki_SaishintuikaOffset = 0;
	int saishingeki_SaishintuikaEndOffset = 0;
	int sinRtl_end_air_offset[2] = { 0 };
	int zanseiRougaRecoveryOffset = 0;
	int hououshouHitOffset = 0;
	int stunEdgeMahojinDistX = 0;
	int stunEdgeMahojinDistY = 0;
	int chargedStunEdgeMahojinDistX = 0;
	int chargedStunEdgeMahojinDistY = 0;
	int sacredEdgeMahojinDistX = 0;
	int sacredEdgeMahojinDistY = 0;
	int spChargedStunEdgeKowareSpriteDuration = 0;
	int stunEdgeDeleteSpriteSum = 0;
	int laserFishCreateLaserOffset = 0;
	int ky5DDustEffectShot_firstSpriteAfter_Offset = 0;
	int mayPBallJumpConnectOffset = 0;
	int mayKBallJumpConnectOffset = 0;
	int mayPBallJumpConnectRange = 0;
	int mayKBallJumpConnectRange = 0;
	struct MayIrukasanRidingObjectFrames {
		int offset = 0;
		int frames = 0;
	};
	struct MayIrukasanRidingObjectInfo {
		int offset = 0;
		int totalFrames = 0;
		std::vector<MayIrukasanRidingObjectFrames> frames;
		inline void clear() { offset = 0; totalFrames = 0; frames.clear(); }
	};
	MayIrukasanRidingObjectInfo mayIrukasanRidingObjectYokoA { 0 };
	MayIrukasanRidingObjectInfo mayIrukasanRidingObjectYokoB { 0 };
	MayIrukasanRidingObjectInfo mayIrukasanRidingObjectTateA { 0 };
	MayIrukasanRidingObjectInfo mayIrukasanRidingObjectTateB { 0 };
	int may6H_6DHoldOffset = 0;
	int may6H_6DHoldAttackOffset = 0;
	enum TriBool : char {
		TRIBOOL_DUNNO,
		TRIBOOL_FALSE,
		TRIBOOL_TRUE
	};
	TriBool milliaIsRev2 = TRIBOOL_DUNNO;
	int faust5DExPointX = -1;
	int faust5DExPointY = -1;
	int venomQvClearUponAfterExitOffset = 0;
	int venomBishopCreateOffset = 0;
	std::vector<ForceAddedWhiffCancel> forceAddWhiffCancels;
private:
	struct MyKey {
		CharacterType charType = (CharacterType)-1;
		const char* name = nullptr;
		bool isEffect = false;
	};
	static int hashString(const char* str, int startingHash = 0);
	struct MyHashFunction {
		inline std::size_t operator()(const MyKey& k) const {
			return hashString(k.name);
		}
	};
	struct MyCompareFunction {
		inline bool operator()(const MyKey& k, const MyKey& other) const {
			return k.charType == other.charType && strcmp(k.name, other.name) == 0 && k.isEffect == other.isEffect;
		}
	};
	std::unordered_map<MyKey, MoveInfoStored, MyHashFunction, MyCompareFunction> map;
	void addMove(const MoveInfo& move);
	#ifdef _DEBUG
	std::vector<MyKey> repeatingMoves;
	#endif
};

extern Moves moves;
