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

struct MoveInfo {
	// This is needed for Bandit Revolver turning into Bandit Bringer
	bool combineWithPreviousMove = false;
	// This is needed for Johnny becoming able to use Mist Finer from his MistFinerLoop animation.
	// We want to write X+possibly infinite frames as long as you're holding, where X is the startup of
	// the availability of Mist Finer attack.
	bool usePlusSignInCombination = false;
	const char* displayName = nullptr;
	// A section is what I call separating frames with a + sign in the startup, recovery or total display.
	// This is useful for some moves that can be held or charged, because if you treat the part of the
	// animation that starts after you release the button as separate and show it with a "frames from before
	// that part" + "frames after that part", then you will be able to tell what the startup of the move is
	// after you release the button
	sectionSeparator_t sectionSeparator = nullptr;
	sectionSeparatorProjectile_t sectionSeparatorProjectile = nullptr;
	// If PlayerInfo::inNewMoveSection == true for this many frames, the player is considered 'idle' in all respects.
	int considerIdleInSeparatedSectionAfterThisManyFrames = 0;
	// This is needed for Johnny walking back and forth in Mist Finer because he can start walking
	// as soon as he can do Mist Finer attack, but we still want the new section because we treat
	// walking same way as standing in Mist Finer
	bool preservesNewSection = false;
	isIdle_t isIdle = nullptr;
	isIdle_t canBlock = nullptr;
	isDangerous_t isDangerous = isDangerous_default;
	int framebarId = -1;
	const char* framebarName = nullptr;
	const char* framebarNameFull = nullptr;
	selectFramebarName_t framebarNameSelector = nullptr;
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
	bool butForFramebarDontCombineWithPreviousMove = false;
	const char* replacementInputs = nullptr;
	int replacementBufferTime = 0;
	const char* whiffCancelsNote = nullptr;
	isIdle_t secondaryStartup = nullptr;
	const char* getFramebarName(Entity ent) const;
	void addForceAddWhiffCancel(const char* name);
	ForceAddedWhiffCancel* getForceAddWhiffCancel(int index) const;
	inline const char* getDisplayName(bool isIdle) const { return isIdle && displayNameIfIdle ? displayNameIfIdle : displayName; }
	MoveInfo(bool combineWithPreviousMove = false,
		bool usePlusSignInCombination = false,
		const char* displayName = nullptr,
		sectionSeparator_t sectionSeparator = nullptr,
		sectionSeparatorProjectile_t sectionSeparatorProjectile = nullptr,
		int considerIdleInSeparatedSectionAfterThisManyFrames = 0,
		bool preservesNewSection = false,
		isIdle_t isIdle = nullptr,
		isIdle_t canBlock = nullptr,
		isDangerous_t isDangerous = isDangerous_default,
		int framebarId = -1,
		const char* framebarName = nullptr,
		const char* framebarNameFull = nullptr,
		selectFramebarName_t framebarNameSelector = nullptr,
		isIdle_t isInVariableStartupSection = nullptr,
		isIdle_t canStopHolding = nullptr,
		isIdle_t aSectionBeforeVariableStartup = nullptr,
		bool considerNewSectionAsBeingInVariableStartup = false,
		bool considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = false,
		bool considerVariableStartupAsStanceForFramebar = false,
		bool canBeUnableToBlockIndefinitelyOrForVeryLongTime = false,
		isIdle_t isRecoveryHasGatlings = nullptr,
		isIdle_t isRecoveryCanAct = nullptr,
		isIdle_t canFaultlessDefend = nullptr,
		bool nameIncludesInputs = false,
		bool ignoresHitstop = false,
		isIdle_t frontLegInvul = nullptr,
		int forceAddWhiffCancelsStart = 0,
		int forceAddWhiffCancelsCount = 0,
		isIdle_t isRecoveryCanReload = nullptr,
		const char* onlyAddForceWhiffCancelsOnFirstFrameOfSprite = nullptr,
		zatoHoldLevel_t zatoHoldLevel = nullptr,
		isIdle_t conditionForAddingWhiffCancels = nullptr,
		bool caresAboutWall = false,
		bool faustPogo = false,
		const char* displayNameIfIdle = nullptr,
		bool butForFramebarDontCombineWithPreviousMove = false,
		const char* replacementInputs = nullptr,
		int replacementBufferTime = 0,
		const char* whiffCancelsNote = nullptr,
		isIdle_t secondaryStartup = nullptr);
};

class Moves {
public:
	bool onDllMain();
	const MoveInfo& getInfo(CharacterType charType, const char* moveName, const char* stateName, bool isEffect);
	const MoveInfo& getInfo(CharacterType charType, const char* name, bool isEffect);
	void onAswEngineDestroyed();
	MoveInfo defaultMove{
		false, false, nullptr
	};
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
	std::unordered_map<MyKey, MoveInfo, MyHashFunction, MyCompareFunction> map;
	struct AddedMove : MoveInfo {
		CharacterType charType;
		const char* name;
		bool isEffect;
		inline AddedMove() { }
		inline AddedMove(CharacterType charType, const char* name, bool isEffect = false) : charType(charType), name(name), isEffect(isEffect) { }
	};
	void addMove(const AddedMove& move);
	#ifdef _DEBUG
	std::vector<MyKey> repeatingMoves;
	#endif
};

extern Moves moves;
