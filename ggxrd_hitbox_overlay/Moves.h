#pragma once
#include "characterTypes.h"
#include <unordered_map>
#include "Entity.h"

struct PlayerInfo;

using sectionSeparator_t = bool(*)(Entity ent);
using isIdle_t = bool(*)(const PlayerInfo& ent);
bool isIdle_default(const PlayerInfo& player);
bool canBlock_default(const PlayerInfo& player);

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
	MoveInfo(bool combineWithPreviousMove = false,
		bool usePlusSignInCombination = false,
		const char* displayName = nullptr,
		sectionSeparator_t sectionSeparator = nullptr,
		int considerIdleInSeparatedSectionAfterThisManyFrames = 0,
		bool preservesNewSection = false,
		isIdle_t isIdle = nullptr,
		isIdle_t canBlock = nullptr);
};

class Moves {
public:
	void onDllMain();
	const MoveInfo& getInfo(CharacterType charType, const char* name, bool isEffect);
	MoveInfo defaultMove{
		false, false, nullptr
	};
private:
	struct MyKey {
		CharacterType charType = (CharacterType)-1;
		const char* name = nullptr;
		bool isEffect = false;
	};
	inline static int hashString(const char* str, int startingHash = 0) {
		for (const char* c = str; *c != '\0'; ++c) {
			startingHash = startingHash * 0x89 + *c;
		}
		return startingHash;
	}
	struct MyHashFunction {
		inline std::size_t operator()(const MyKey& k) const {
			return hashString(k.name);
		}
	};
	struct MyCompareFunction {
		inline bool operator()(const MyKey& k, const MyKey& other) const {
			return k.charType == other.charType && strcmp(k.name, other.name) == 0;
		}
	};
	std::unordered_map<MyKey, MoveInfo, MyHashFunction, MyCompareFunction> map;
	
};

extern Moves moves;
