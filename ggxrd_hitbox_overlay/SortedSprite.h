#pragma once
#include "pch.h"
#include "EditedHitbox.h"
#include "Entity.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>

struct SortedSprite {
	char* name;
	bool deleted = false;
	bool added = false;
	char newName[32] { 0 };
	// this was an std::vector<BYTE> newJonbin previously
	// then we got a crash on DLL_PROCESS_DETACH during std::vector destructor from ~SortedSprite
	// caused by 'SortedSprite backup = *currentSpriteElement;' in UI.cpp when renaming a sprite, probably (idk)
	// we need a copyable vector that shares storage
	// we need vector to be dumb
	// std one is way too smart. It's causing problems
	BYTE* newJonbin = nullptr;
	size_t newJonbinSize = 0;
	size_t newJonbinCapacity = 0;
	
	EditedHitbox* layers = nullptr;
	size_t layersSize = 0;
	size_t layersCapacity = 0;
	void resizeLayers(size_t sizeToSet);
	// if you can get 'struct FPACSecondaryData*' to be the type of mainData here and get it to compile, please let me know how you did it
	inline void addToJonbin(struct FPACSecondaryData* mainData, DWORD sizeToAdd) { resizeJonbin(mainData, newJonbinSize + sizeToAdd); }
	void resizeJonbin(struct FPACSecondaryData* mainData, size_t newSize);
	const char* repr() const;
	void freeMemory();
};

struct SourceFpacProvier {
	virtual void rewind() = 0;
	virtual bool getNext() = 0;
	// of current entry
	virtual DWORD getHash() const = 0;
	virtual const char (&getName() const)[32] = 0;
	virtual int getSize() const = 0;
	virtual BYTE* getJonbin() const = 0;
	virtual void forgetJonbin() = 0;  // , it's mine now
};

struct FPACSecondaryData {
	// Lookup table must always sit in UE3-owned memory at TopData+0x20.
	
	bool parsed = false;
	int bbscrIndexInAswEng = -1;
	REDAssetCollision* Collision = nullptr;
	// extra space to store jonbins that don't fit into the UE3 memory
	
	// maps a jonbin to its lookup entry.
	// Pair.first points to a jonbin located in UE3-owned memory.
	// Pair.second points to its lookup entry.
	std::map<void*, void*> jonbinToLookupUE3;
	std::vector<SortedSprite> sortedSprites;  // each char* points to a lookup entry in UE3-owned memory. Sorted in ascending alphabetic order
	// map original, unmodified name's hash to an index in sortedSprites
	std::unordered_map<DWORD, DWORD> oldNameToSortedSpriteIndex;
	// since renames and deletions just get remembered for later, we need to verify newly entered names somehow,
	// that their hash does not collide with any other (there is a small possibility of that).
	// This set will only contain hashes that would remain after deletions and renames.
	std::unordered_set<DWORD> newHashMap;
	int newSpriteCounter = 0;
	void clear();
	// call this after appRealloc'ing fpac. Does not adjust rawSize
	void onFPACRelocate(FPAC* oldLoc, FPAC* newLoc, DWORD oldSize, DWORD newSize);
	// updates the following:
	// Entity::fpac() - for when you appRealloc the whole UE3 memory
	// Entity::hitboxes()->jonbinPtr
	// Entity::hitboxes()->names
	// Entity::hitboxes()->ptrRawAfterShort1
	// Entity::hitboxes()->ptrLookup
	// Entity::hitboxes()->data
	void relocateEntityJonbins(const BYTE* oldData, const BYTE* oldDataEnd, BYTE* newData);
	// may trash sortedSprites, both maps, relocate entire FPAC.
	// Increments fpac->count
	void allocateNewLookupEntry();
	int sortedSpritesFind(const char* str);
	void parseAllSprites();
	void generateNewSpriteName(char (&outNameBuf)[32], DWORD* hash, DWORD* insertionIndex);
	void detach();
	template<typename T>
	void detachPiece(BYTE* newData);
	// Sprite must not be in use by other entities
	void deleteSprite(SortedSprite& spriteToDelete);
	~FPACSecondaryData();
	void hitboxEditorDeleteOperationMoveIndices(BYTE* jonbin, int hitboxCount, int startInd, int endInd);
	DWORD findSortedSpriteInsertionIndex(const char* str) const;
	template<typename T, bool canStealJonbins>
	void replaceFpac(SourceFpacProvier* provider);
};
