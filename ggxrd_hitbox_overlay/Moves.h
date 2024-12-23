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

enum MoveInfoPropertyType {
	MoveInfoPropertyType_combineWithPreviousMove,
	// This is needed for Johnny becoming able to use Mist Finer from his MistFinerLoop animation.
	// We want to write X+possibly infinite frames as long as you're holding, where X is the startup of
	// the availability of Mist Finer attack.
	MoveInfoPropertyType_usePlusSignInCombination,
	MoveInfoPropertyType_displayName,
	MoveInfoPropertyType_slangName,
	// A section is what I call separating frames with a + sign in the startup, recovery or total display.
	// This is useful for some moves that can be held or charged, because if you treat the part of the
	// animation that starts after you release the button as separate and show it with a "frames from before
	// that part" + "frames after that part", then you will be able to tell what the startup of the move is
	// after you release the button
	MoveInfoPropertyType_sectionSeparator,
	MoveInfoPropertyType_sectionSeparatorProjectile,
	// If PlayerInfo::inNewMoveSection == true for this many frames, the player is considered 'idle' in all respects.
	MoveInfoPropertyType_considerIdleInSeparatedSectionAfterThisManyFrames,
	// This is needed for Johnny walking back and forth in Mist Finer because he can start walking
	// as soon as he can do Mist Finer attack, but we still want the new section because we treat
	// walking same way as standing in Mist Finer
	MoveInfoPropertyType_preservesNewSection,
	MoveInfoPropertyType_isIdle,
	MoveInfoPropertyType_canBlock,
	MoveInfoPropertyType_isDangerous,
	MoveInfoPropertyType_framebarId,
	MoveInfoPropertyType_framebarName,
	MoveInfoPropertyType_framebarNameUncombined,
	MoveInfoPropertyType_framebarSlangNameUncombined,
	MoveInfoPropertyType_framebarSlangName,
	MoveInfoPropertyType_framebarNameFull,
	MoveInfoPropertyType_framebarNameSelector,
	MoveInfoPropertyType_framebarSlangNameSelector,
	MoveInfoPropertyType_isInVariableStartupSection,
	MoveInfoPropertyType_canStopHolding,
	MoveInfoPropertyType_aSectionBeforeVariableStartup,
	MoveInfoPropertyType_considerNewSectionAsBeingInVariableStartup,
	MoveInfoPropertyType_considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot,
	MoveInfoPropertyType_considerVariableStartupAsStanceForFramebar,
	MoveInfoPropertyType_canBeUnableToBlockIndefinitelyOrForVeryLongTime,
	MoveInfoPropertyType_isRecoveryHasGatlings,
	MoveInfoPropertyType_isRecoveryCanAct,
	MoveInfoPropertyType_canFaultlessDefend,
	MoveInfoPropertyType_nameIncludesInputs,
	MoveInfoPropertyType_ignoresHitstop,
	MoveInfoPropertyType_frontLegInvul,
	MoveInfoPropertyType_forceAddWhiffCancelsStart,
	MoveInfoPropertyType_forceAddWhiffCancelsCount,
	MoveInfoPropertyType_isRecoveryCanReload,
	MoveInfoPropertyType_onlyAddForceWhiffCancelsOnFirstFrameOfSprite,
	MoveInfoPropertyType_zatoHoldLevel,
	MoveInfoPropertyType_conditionForAddingWhiffCancels,
	MoveInfoPropertyType_caresAboutWall,
	MoveInfoPropertyType_faustPogo,
	MoveInfoPropertyType_displayNameIfIdle,
	MoveInfoPropertyType_displayNameIfIdleSlang,
	MoveInfoPropertyType_butForFramebarDontCombineWithPreviousMove,
	MoveInfoPropertyType_replacementInputs,
	MoveInfoPropertyType_replacementBufferTime,
	MoveInfoPropertyType_whiffCancelsNote,
	MoveInfoPropertyType_secondaryStartup,
	MoveInfoPropertyType_forceLandingRecovery,
	MoveInfoPropertyType_isGrab,
	MoveInfoPropertyType_partOfStance
};

struct MoveInfoProperty {
	MoveInfoPropertyType type;
	union {
		bool boolValue;
		const char* strValue;
		sectionSeparator_t sectionSeparatorValue;
		sectionSeparatorProjectile_t sectionSeparatorProjectileValue;
		int intValue;
		isIdle_t isIdleValue;
		isDangerous_t isDangerousValue;
		selectFramebarName_t selectFramebarNameValue;
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

struct MoveInfo {
	bool combineWithPreviousMove = false;
	bool usePlusSignInCombination = false;
	const char* displayName = nullptr;
	const char* slangName = nullptr;
	sectionSeparator_t sectionSeparator = nullptr;
	sectionSeparatorProjectile_t sectionSeparatorProjectile = nullptr;
	int considerIdleInSeparatedSectionAfterThisManyFrames = 0;
	bool preservesNewSection = false;
	isIdle_t isIdle = nullptr;
	isIdle_t canBlock = nullptr;
	isDangerous_t isDangerous = nullptr;
	int framebarId = -1;
	const char* framebarName = nullptr;
	const char* framebarNameUncombined = nullptr;
	const char* framebarSlangNameUncombined = nullptr;
	const char* framebarSlangName = nullptr;
	const char* framebarNameFull = nullptr;
	selectFramebarName_t framebarNameSelector = nullptr;
	selectFramebarName_t framebarSlangNameSelector = nullptr;
	isIdle_t isInVariableStartupSection = nullptr;
	isIdle_t canStopHolding = nullptr;
	isIdle_t aSectionBeforeVariableStartup = nullptr;
	bool considerNewSectionAsBeingInVariableStartup = false;
	bool considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = false;
	bool considerVariableStartupAsStanceForFramebar = false;
	bool canBeUnableToBlockIndefinitelyOrForVeryLongTime = false;
	isIdle_t isRecoveryHasGatlings = nullptr;
	isIdle_t isRecoveryCanAct = nullptr;
	isIdle_t canFaultlessDefend = nullptr;
	bool nameIncludesInputs = false;
	bool ignoresHitstop = false;
	isIdle_t frontLegInvul = nullptr;
	int forceAddWhiffCancelsStart = 0;
	int forceAddWhiffCancelsCount = 0;
	isIdle_t isRecoveryCanReload = nullptr;
	const char* onlyAddForceWhiffCancelsOnFirstFrameOfSprite = nullptr;
	zatoHoldLevel_t zatoHoldLevel = nullptr;
	isIdle_t conditionForAddingWhiffCancels = nullptr;
	bool caresAboutWall = false;
	bool faustPogo = false;
	const char* displayNameIfIdle = nullptr;
	const char* displayNameIfIdleSlang = nullptr;
	bool butForFramebarDontCombineWithPreviousMove = false;
	const char* replacementInputs = nullptr;
	int replacementBufferTime = 0;
	const char* whiffCancelsNote = nullptr;
	isIdle_t secondaryStartup = nullptr;
	bool forceLandingRecovery = false;
	bool isGrab = false;
	bool partOfStance = false;
	
	CharacterType charType;
	const char* name;
	bool isEffect;
	inline MoveInfo() { }
	MoveInfo(const MoveInfoStored& info);
	inline MoveInfo(CharacterType charType, const char* name, bool isEffect = false) : charType(charType), name(name), isEffect(isEffect) { }
	const char* getFramebarName(Entity ent) const;
	void addForceAddWhiffCancel(const char* name);
	ForceAddedWhiffCancel* getForceAddWhiffCancel(int index) const;
	inline const char* getDisplayName(bool isIdle) const { return isIdle && displayNameIfIdle ? displayNameIfIdle : displayName; }
	inline const char* getDisplayNameSlang(bool isIdle) const { return isIdle && displayNameIfIdleSlang ? displayNameIfIdleSlang : slangName; }
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
		instr_setMarker = 11,
		instr_createObject = 446,
	};
	inline InstructionType instructionType(BYTE* in) const;
	BYTE* findSetMarker(BYTE* in, const char* name) const;
	BYTE* findNextMarker(BYTE* in, const char** name) const;
	BYTE* findCreateObj(BYTE* in, const char* name) const;
	BYTE* findSpriteNull(BYTE* in) const;
	BYTE* findSpriteNonNull(BYTE* in) const;
	int armorDanceEndOffset = 0;  // in number of bytes
	int armorDance2EndOffset = 0;
	int saishingeki_SaishintuikaOffset = 0;
	int saishingeki_SaishintuikaEndOffset = 0;
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
