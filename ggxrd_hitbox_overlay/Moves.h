#pragma once
#include "characterTypes.h"
#include <unordered_map>
#include "Entity.h"

struct PlayerInfo;

using sectionSeparator_t = bool(*)(Entity ent);
using isIdle_t = bool(*)(const PlayerInfo& ent);
using isDangerous_t = bool(*)(Entity ent);
using selectFramebarName_t = const char*(*)(Entity ent);

bool isIdle_default(const PlayerInfo& player);
bool canBlock_default(const PlayerInfo& player);
bool isDangerous_default(Entity ent);

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
	// If PlayerInfo::inNewMoveSection == true for this many frames, the player is considered 'idle' in all respects.
	int considerIdleInSeparatedSectionAfterThisManyFrames = 0;
	// This is needed for Johnny walking back and forth in Mist Finer because he can start walking
	// as soon as he can do Mist Finer attack, but we still want the new section because we treat
	// walking same way as standing in Mist Finer
	bool preservesNewSection = false;
	isIdle_t isIdle = isIdle_default;
	isIdle_t canBlock = canBlock_default;
	isDangerous_t isDangerous = isDangerous_default;
	int framebarId = -1;
	const char* framebarName = nullptr;
	const char* framebarNameFull = nullptr;
	selectFramebarName_t framebarNameSelector = nullptr;
	isDangerous_t isInArbitraryStartupSection = nullptr;
	bool considerNewSectionAsBeingInArbitraryStartup = false;
	bool considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = false;
	bool considerArbitraryStartupAsStanceForFramebar = false;
	bool canBeUnableToBlockIndefinitelyOrForVeryLongTime = false;
	isDangerous_t isRecoveryHasGatlings = nullptr;
	isDangerous_t isRecoveryCanAct = nullptr;
	isDangerous_t canFaultlessDefend = nullptr;
	const char* getFramebarName(Entity ent) const;
	MoveInfo(bool combineWithPreviousMove = false,
		bool usePlusSignInCombination = false,
		const char* displayName = nullptr,
		sectionSeparator_t sectionSeparator = nullptr,
		int considerIdleInSeparatedSectionAfterThisManyFrames = 0,
		bool preservesNewSection = false,
		isIdle_t isIdle = nullptr,
		isIdle_t canBlock = nullptr,
		isDangerous_t isDangerous = isDangerous_default,
		int framebarId = -1,
		const char* framebarName = nullptr,
		const char* framebarNameFull = nullptr,
		selectFramebarName_t framebarNameSelector = nullptr,
		isDangerous_t isInArbitraryStartupSection = nullptr,
		bool considerNewSectionAsBeingInArbitraryStartup = false,
		bool considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = false,
		bool considerArbitraryStartupAsStanceForFramebar = false,
		bool canBeUnableToBlockIndefinitelyOrForVeryLongTime = false,
		isDangerous_t isRecoveryHasGatlings = nullptr,
		isDangerous_t isRecoveryCanAct = nullptr,
		isDangerous_t canFaultlessDefend = nullptr);
};

class Moves {
public:
	bool onDllMain();
	const MoveInfo& getInfo(CharacterType charType, const char* name, bool isEffect);
	MoveInfo defaultMove{
		false, false, nullptr
	};
	unsigned short* bbscrInstructionSizes = nullptr;
	inline BYTE* gotoNextInstruction(BYTE* in) const;
	enum InstructionType {
		instr_endState = 1,
		instr_sprite = 2,
		instr_setMarker = 11,
		instr_createObject = 446,
	};
	inline InstructionType instructionType(BYTE* in) const;
	BYTE* findSetMarker(BYTE* in, const char* name) const;
	BYTE* findCreateObj(BYTE* in, const char* name) const;
	BYTE* findSpriteNull(BYTE* in) const;
	BYTE* findSpriteNonNull(BYTE* in) const;
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
		bool combineWithPreviousMove = false;
		bool usePlusSignInCombination = false;
		const char* displayName = nullptr;
		sectionSeparator_t sectionSeparator = nullptr;
		int considerIdleInSeparatedSectionAfterThisManyFrames = 0;
		bool preservesNewSection = false;
		isIdle_t isIdle = nullptr;
		isIdle_t canBlock = nullptr;
		isDangerous_t isDangerous = isDangerous_default;
		int framebarId = -1;
		const char* framebarName = nullptr;
		const char* framebarNameFull = nullptr;
		selectFramebarName_t framebarNameSelector = nullptr;
		isDangerous_t isInArbitraryStartupSection = nullptr;
		bool considerNewSectionAsBeingInArbitraryStartup = false;
		bool considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot = false;
		bool considerArbitraryStartupAsStanceForFramebar = false;
		bool canBeUnableToBlockIndefinitelyOrForVeryLongTime = false;
		isDangerous_t isRecoveryHasGatlings = nullptr;
		isDangerous_t isRecoveryCanAct = nullptr;
		isDangerous_t canFaultlessDefend = nullptr;
		inline AddedMove() { }
		inline AddedMove(CharacterType charType, const char* name, bool isEffect = false) : charType(charType), name(name), isEffect(isEffect) { }
	};
	void addMove(const AddedMove& move);
};

extern Moves moves;
