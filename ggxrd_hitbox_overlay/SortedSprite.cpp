#include "pch.h"
#include "SortedSprite.h"
#include <stdio.h>
#include "Game.h"
#include "EntityList.h"
#include "EndScene.h"
#include "Game.h"
#include <stdexcept>
#include "LayerIterator.h"

static char reprbuf[128];

int __cdecl CompareSortedSprites(void const* spriteLeft, void const* spriteRight) {
	SortedSprite* spriteLeftC = (SortedSprite*)spriteLeft;
	SortedSprite* spriteRightC = (SortedSprite*)spriteRight;
	const char* nameLeft = spriteLeftC->newName[0] ? spriteLeftC->newName : spriteLeftC->name;
	const char* nameRight = spriteRightC->newName[0] ? spriteRightC->newName : spriteRightC->name;
	return strcmp(nameLeft, nameRight);
}

void SortedSprite::resizeLayers(size_t sizeToSet) {
	if (sizeToSet == layersSize) return;
	if (!sizeToSet) {
		layersSize = 0;
		return;
	}
	if (layers) {
		layersSize = sizeToSet;
		if (layersCapacity < sizeToSet) {
			if (layersCapacity * 2 < sizeToSet) {
				layersCapacity = sizeToSet;
			} else {
				layersCapacity *= 2;
			}
			EditedHitbox* newMem = (EditedHitbox*)realloc(layers, layersCapacity * sizeof(EditedHitbox));
			if (!newMem) throw std::bad_alloc();
			layers = newMem;
		}
	} else {
		layersSize = sizeToSet;
		layersCapacity = sizeToSet;
		layers = (EditedHitbox*)calloc(sizeToSet, sizeof(EditedHitbox));
	}
}

// we don't run this on destructor, because SortedSprite sits in a std::vector that we call insert and other things on,
// and gets moved from it and to it through a local variable,
// and we're very afraid at one of these points it might fire off a destructor,
// while the idea was to just copy the """vector""" in all of those cases and explicitly trigger a release
void SortedSprite::freeMemory() {
	if (newJonbin) free(newJonbin);
	if (layers) free(layers);
}

void SortedSprite::resizeJonbin(FPACSecondaryData* mainData, size_t newSize) {
	if (newJonbinSize == newSize) return;
	if (!newSize) {
		newJonbinSize = 0;
		return;
	}
	
	BYTE* oldData = newJonbin;
	BYTE* oldDataEnd = oldData + newJonbinSize;
	
	if (newJonbinCapacity < newSize) {
		newJonbinSize = newSize;
		if (newJonbinCapacity * 2 < newSize) {
			newJonbinCapacity = newSize;
		} else {
			newJonbinCapacity *= 2;
		}
		if (newJonbin) {
			BYTE* mallocMayReturnNull = (BYTE*)realloc(newJonbin, newJonbinCapacity);
			if (!mallocMayReturnNull) throw std::bad_alloc();
			newJonbin = mallocMayReturnNull;
		} else {
			newJonbin = (BYTE*)malloc(newJonbinCapacity);
			if (!newJonbin) throw std::bad_alloc();
		}
	} else {
		newJonbinSize = newSize;
	}
	
	if (newJonbin != oldData && oldData && oldData != oldDataEnd) {
		BYTE* newData = newJonbin;
		mainData->relocateEntityJonbins(oldData, oldDataEnd, newData);
		int disp = newData - oldData;
		
		if (mainData->Collision->TopData->size0x50()) {
			FPACLookupElement0x50* elem = (FPACLookupElement0x50*)name;
			elem->offset += disp;
		} else {
			FPACLookupElement0x30* elem = (FPACLookupElement0x30*)name;
			elem->offset += disp;
		}
		if (layers) {
			EditedHitbox* layer = layers;
			for (int i = 0; i < (int)layersSize; ++i) {
				layer->ptr = (Hitbox*)(newData + ((BYTE*)layer->ptr - oldData));
				++layer;
			}
		}
	}
}

const char* SortedSprite::repr() const {
	const char* userName = newName[0] ? newName : name;
	if (!added && !deleted) {
		return userName;
	}
	sprintf_s(reprbuf, "%s%s%s", userName,
		added ? " (Added)" : "",
		deleted ? " (Deleted)" : "");
	return reprbuf;
}

void FPACSecondaryData::clear() {
	parsed = false;
	jonbinToLookupUE3.clear();
	for (SortedSprite& sortedSprite : sortedSprites) {
		sortedSprite.freeMemory();
	}
	sortedSprites.clear();
	oldNameToSortedSpriteIndex.clear();
	newHashMap.clear();
	newSpriteCounter = 0;
	Collision = nullptr;
}

void FPACSecondaryData::generateNewSpriteName(char (&outNameBuf)[32], DWORD* hash, DWORD* insertionIndex) {
	
	FPAC* fpac = Collision->TopData;
	
	static const char new_sprite[] { "new_sprite" };
	
	bool thereIsCollision = false;
	do {
		++newSpriteCounter;
		
		int printedCharsCount = sprintf_s(outNameBuf, "%s%.3d", new_sprite, newSpriteCounter);
		int remainingSize = sizeof outNameBuf - printedCharsCount;
		if (remainingSize > 0) {
			memset(outNameBuf + printedCharsCount, '\0', remainingSize);
		}
		
		*hash = Entity::hashStringLowercase(outNameBuf);
		*insertionIndex = fpac->findInsertionIndex(*hash);
		thereIsCollision = *insertionIndex == 0xFFFFFFFF || newHashMap.find(*hash) != newHashMap.end();
	} while (thereIsCollision);
}

template<typename T>
static void onFPACRelocateImpl(std::map<void*, void*>& mapUE3, int count, DWORD maxOffset, int disp, SortedSprite* ptr, FPAC* newLoc, BYTE* newLocEnd, FPAC* oldLoc, BYTE* oldLocEnd) {
	for (int i = 0; i < count; ++i) {
		ptr->name += disp;
		T* lookupEntry = (T*)ptr->name;
		if (lookupEntry->offset > maxOffset) {
			lookupEntry->offset -= disp;
		}
		BYTE* jonbin = (BYTE*)newLoc + newLoc->headerSize + lookupEntry->offset;
		if (jonbin >= (BYTE*)newLoc && jonbin < newLocEnd) {
			mapUE3[jonbin] = ptr->name;
		}
		if (ptr->layers) {
			EditedHitbox* layer = ptr->layers;
			for (int layerIndex = 0; layerIndex < (int)ptr->layersSize; ++layerIndex) {
				if ((BYTE*)layer->ptr >= (BYTE*)oldLoc && (BYTE*)layer->ptr < oldLocEnd) {
					layer->ptr = (Hitbox*)((BYTE*)newLoc + ((BYTE*)layer->ptr - (BYTE*)oldLoc));
				}
				++layer;
			}
		}
		++ptr;
	}
}
	
// call this after appRealloc'ing fpac. Does not adjust rawSize
void FPACSecondaryData::onFPACRelocate(FPAC* oldLoc, FPAC* newLoc, DWORD oldSize, DWORD newSize) {
	if (oldLoc == newLoc) return;
	if (Collision->TopData == oldLoc) {
		Collision->TopData = newLoc;
		Collision->DataSize = newSize;
		
		BYTE* oldLocEnd = (BYTE*)oldLoc + oldSize;
		
		relocateEntityJonbins((BYTE*)oldLoc, oldLocEnd, (BYTE*)newLoc);
		
		jonbinToLookupUE3.clear();
		
		int disp = (BYTE*)newLoc - (BYTE*)oldLoc;
		
		BYTE* newLocEnd = (BYTE*)newLoc + newLoc->rawSize;
		
		int count = (int)sortedSprites.size();
		SortedSprite* ptr = sortedSprites.data();
		
		DWORD maxOffset = newLoc->rawSize - newLoc->headerSize;
		
		if (newLoc->size0x50()) {
			onFPACRelocateImpl<FPACLookupElement0x50>(jonbinToLookupUE3, count, maxOffset, disp, ptr, newLoc, newLocEnd, oldLoc, oldLocEnd);
		} else {
			onFPACRelocateImpl<FPACLookupElement0x30>(jonbinToLookupUE3, count, maxOffset, disp, ptr, newLoc, newLocEnd, oldLoc, oldLocEnd);
		}
	}
}

void FPACSecondaryData::parseAllSprites() {
	
	if (parsed) return;
	
	Collision = ((REDPawn_Player**)(*aswEngine + aswEnginePawnsOffset))[bbscrIndexInAswEng]->Collision();
	
	FPAC* fpac = Collision->TopData;
	
	static std::vector<SortedSprite>* g_sortedSprites;
	static std::map<void*, void*>* g_mapUE3;
	static BYTE* g_UE3Start;
	static BYTE* g_UE3End;
	static std::unordered_set<DWORD>* g_newHashMap;
	static DWORD g_hashOffset;
	
	struct MyNameEnumerator {
		static bool callback(char* name, BYTE* jonbin) {
			g_sortedSprites->push_back({ name, false });
			if (jonbin >= g_UE3Start && jonbin < g_UE3End) {
				(*g_mapUE3)[(void*)jonbin] = (void*)name;
			}
			g_newHashMap->insert(*(DWORD*)(name + g_hashOffset));
			return true;
		}
	};
	
	sortedSprites.reserve(fpac->count);
	g_sortedSprites = &sortedSprites;
		
	g_mapUE3 = &jonbinToLookupUE3;
	g_UE3Start = (BYTE*)fpac;
	g_UE3End = (BYTE*)fpac + fpac->rawSize;
	
	g_newHashMap = &newHashMap;
	g_hashOffset = fpac->size0x50() ? offsetof(FPACLookupElement0x50, hash) : offsetof(FPACLookupElement0x30, hash);
	
	fpac->enumNames(MyNameEnumerator::callback);
	
	qsort(sortedSprites.data(), sortedSprites.size(), sizeof SortedSprite, CompareSortedSprites);
	
	size_t sortedSpritesCount = sortedSprites.size();
	for (DWORD i = 0; i < sortedSpritesCount; ++i) {
		DWORD hash = Entity::hashStringLowercase(sortedSprites[i].name);
		oldNameToSortedSpriteIndex[hash] = i;
	}
	
	parsed = true;
	
}

// updates the following:
// Entity::fpac()
// Entity::hitboxes()->jonbinPtr
// Entity::hitboxes()->names[]
// Entity::hitboxes()->ptrRawAfterShort1
// Entity::hitboxes()->ptrLookup
// Entity::hitboxes()->data
void FPACSecondaryData::relocateEntityJonbins(const BYTE* oldData, const BYTE* oldDataEnd, BYTE* newData) {
	int disp = newData - oldData;
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.fpac() == (const FPAC*)oldData) {
			ent.fpac() = (FPAC*)newData;
		}
		HitboxHolder* hitboxes = ent.hitboxes();
		if (hitboxes->jonbinPtr >= oldData && hitboxes->jonbinPtr < oldDataEnd) {
			hitboxes->jonbinPtr += disp;
		}
		for (int nameInd = 0; nameInd < hitboxes->nameCount; ++nameInd) {
			if ((BYTE*)hitboxes->names[nameInd] >= oldData && (BYTE*)hitboxes->names[nameInd] < oldDataEnd) {
				hitboxes->names[nameInd] += disp;
			}
		}
		if (hitboxes->ptrRawAfterShort1 >= oldData && hitboxes->ptrRawAfterShort1 < oldDataEnd) {
			hitboxes->ptrRawAfterShort1 += disp;
		}
		for (int hitboxTypeInd = 0; hitboxTypeInd < 17; ++hitboxTypeInd) {
			BYTE* ptr = (BYTE*)hitboxes->data[hitboxTypeInd];
			if (ptr >= oldData && ptr < oldDataEnd) {
				hitboxes->data[hitboxTypeInd] = (Hitbox*)(ptr + disp);
			}
		}
		if (hitboxes->ptrLookup >= oldData && hitboxes->ptrLookup < oldDataEnd) {
			hitboxes->ptrLookup += disp;
		}
	}
}

int FPACSecondaryData::sortedSpritesFind(const char* str) {
	if (sortedSprites.empty()) return -1;
	int start = 0;
	int end = (int)sortedSprites.size() - 1;
	while (true) {
		int mid = (start + end) >> 1;
		const SortedSprite& arMember = sortedSprites[mid];
		const char* membName = arMember.newName[0] ? arMember.newName : arMember.name;
		int cmpResult = strcmp(str, membName);
		if (cmpResult < 0) {
			end = mid - 1;
			if (mid == start) {
				return -1;
			}
		} else if (cmpResult > 0) {
			start = mid + 1;
			if (mid == end) {
				return -1;
			}
		} else if (cmpResult == 0) {
			return mid;
		}
	}
}

template<typename T>
void allocateNewLookupEntryPiece(std::map<void*, void*>::iterator* mapDeletionIteratorEnd,
		BYTE** prevJonbinEnd,
		DWORD* sum,
		FPACSecondaryData* mainData
) {
	FPAC* fpac = mainData->Collision->TopData;
	auto itEnd = mainData->jonbinToLookupUE3.end();
	for (auto it = mainData->jonbinToLookupUE3.begin(); it != itEnd; ++it) {
		// jonbin data
		BYTE* jonbinPtr = (BYTE*)it->first;
		T* lookupEntry = (T*)it->second;
		
		DWORD slack = jonbinPtr - *prevJonbinEnd;
		*sum += slack;
		if (*sum >= sizeof (T)) break;
		
		DWORD hash = lookupEntry->hash;
		auto foundIndex = mainData->oldNameToSortedSpriteIndex.find(hash);
		DWORD sortedSpriteIndex = foundIndex->second;
		SortedSprite& sortedSprite = mainData->sortedSprites[sortedSpriteIndex];
		
		sortedSprite.resizeJonbin(mainData, lookupEntry->size);
		memcpy(sortedSprite.newJonbin, jonbinPtr, lookupEntry->size);
		
		lookupEntry->offset = sortedSprite.newJonbin - ((BYTE*)fpac + fpac->headerSize);
		
		mainData->relocateEntityJonbins(jonbinPtr, jonbinPtr + lookupEntry->size, sortedSprite.newJonbin);
		
		*prevJonbinEnd = jonbinPtr + lookupEntry->size;
		
		*mapDeletionIteratorEnd = it;
		++(*mapDeletionIteratorEnd);
		
		*sum += lookupEntry->size;
		if (*sum >= sizeof (T)) break;
	}
}

// may change all of sortedSprites, the whole UE3 map, relocate entire FPAC.
// Increments fpac->count
void FPACSecondaryData::allocateNewLookupEntry() {
	
	FPAC* fpac = Collision->TopData;
	
	DWORD headerElementSize = fpac->elementSize();
	DWORD freeHeaderSpace;
	if (fpac->count * headerElementSize > fpac->headerSize - 0x20) {
		freeHeaderSpace = 0;
	} else {
		freeHeaderSpace = fpac->headerSize - 0x20 - fpac->count * headerElementSize;
	}
	BYTE* lookupEnd = fpac->lookupEnd();
	
	if (freeHeaderSpace >= headerElementSize) {
		++fpac->count;
		return;
	}
	
	// ensure UE3 map is ready for use
	parseAllSprites();
	
	DWORD sum = 0;
	// are there jonbins still left in the UE3-allocated memory? Jonbin is the actual hitbox data, not a lookup table entry
	if (!jonbinToLookupUE3.empty()) {
		// rawSize points to past the last UE3-owned jonbin
		
		BYTE* prevJonbinEnd = fpac->lookupEnd();
		std::map<void*, void*>::iterator mapDeletionIteratorEnd = jonbinToLookupUE3.begin();
		
		if (fpac->size0x50()) {
			allocateNewLookupEntryPiece<FPACLookupElement0x50>(
				&mapDeletionIteratorEnd,
				&prevJonbinEnd,
				&sum,
				this);
		} else {
			allocateNewLookupEntryPiece<FPACLookupElement0x30>(
				&mapDeletionIteratorEnd,
				&prevJonbinEnd,
				&sum,
				this);
		}
		
		jonbinToLookupUE3.erase(jonbinToLookupUE3.begin(), mapDeletionIteratorEnd);
		
	}
	// else rawSize points to past the last lookup entry
	
	DWORD overallSlack = Collision->DataSize - fpac->rawSize;
	sum += overallSlack;
	
	if (sum < headerElementSize) {
		// resize time
		DWORD newSize = Collision->DataSize + 100 * headerElementSize;
		if (!appRealloc) game.sigscanFNamesAndAppRealloc();
		void* newMem = appRealloc(fpac, newSize, 8);
		if (!newMem) throw std::bad_alloc();
		onFPACRelocate(fpac, (FPAC*)newMem, Collision->DataSize, newSize);
		fpac = (FPAC*)newMem;
	}
	
	++fpac->count;
	if (jonbinToLookupUE3.empty()) {
		fpac->rawSize = fpac->lookupEnd() - (BYTE*)fpac;
	}
	return;
}

template<typename T>
void FPACSecondaryData::detachPiece(BYTE* newData) {
	FPAC* fpac = Collision->TopData;
	for (SortedSprite& sortedSprite : sortedSprites) {
		if (sortedSprite.newJonbinSize) {
			relocateEntityJonbins(sortedSprite.newJonbin, sortedSprite.newJonbin + sortedSprite.newJonbinSize, newData);
			T* lookupEntry = (T*)sortedSprite.name;
			memcpy(newData, (BYTE*)fpac + fpac->headerSize + lookupEntry->offset, lookupEntry->size);
			lookupEntry->offset = newData - ((BYTE*)fpac + fpac->headerSize);
			newData += lookupEntry->size;
			
			sortedSprite.newJonbinSize = 0;
			sortedSprite.newJonbinCapacity = 0;
			free(sortedSprite.newJonbin);
			sortedSprite.newJonbin = nullptr;
		}
	}
}

// goal: relocate jonbins from SortedSprite::newJonbin back to the UE3 memory, i.e. rebuild whole FPAC
void FPACSecondaryData::detach() {
	if (!Collision) return;
	FPAC* fpac = Collision->TopData;
	DWORD spaceNeeded = 0;
	for (const SortedSprite& sortedSprite : sortedSprites) {
		spaceNeeded += sortedSprite.newJonbinSize;
	}
	BYTE* newData;
	DWORD ue3SpaceRemaining = Collision->DataSize - fpac->rawSize;
	if (spaceNeeded > ue3SpaceRemaining) {
		DWORD oldSize = fpac->rawSize;
		DWORD newSize = oldSize + spaceNeeded;
		Collision->DataSize = newSize;
		if (!appRealloc) game.sigscanFNamesAndAppRealloc();
		void* newMem = appRealloc(fpac, newSize, 8);
		if (!newMem) throw std::bad_alloc();
		onFPACRelocate(fpac, (FPAC*)newMem, oldSize, newSize);
		fpac = (FPAC*)newMem;
	}
	
	newData = (BYTE*)fpac + fpac->rawSize;
	
	if (fpac->size0x50()) {
		detachPiece<FPACLookupElement0x50>(newData);
	} else {
		detachPiece<FPACLookupElement0x30>(newData);
	}
	
	fpac->rawSize += spaceNeeded;
	
}

// sprite must not be in use by any entities or those entities must be switched off now or later to another sprite
void FPACSecondaryData::deleteSprite(SortedSprite& spriteToDelete) {
	FPAC* fpac = Collision->TopData;
	DWORD headerElementSize = fpac->elementSize();
	if (spriteToDelete.added) {
		
		DWORD oldJonbinOffset;
		DWORD oldJonbinSize;
		BYTE* oldJonbin;
		
		// delete from lookup
		
		DWORD modifHash;
		const char* modifName = spriteToDelete.newName[0] ? spriteToDelete.newName : spriteToDelete.name;
		modifHash = Entity::hashStringLowercase(modifName);
		
		DWORD oldHash;
		if (fpac->size0x50()) {
			FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)spriteToDelete.name;
			oldHash = lookupEntry->hash;
			oldJonbinOffset = lookupEntry->offset;
			oldJonbinSize = lookupEntry->size;
		} else {
			FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)spriteToDelete.name;
			oldHash = lookupEntry->hash;
			oldJonbinOffset = lookupEntry->offset;
			oldJonbinSize = lookupEntry->size;
		}
		
		oldJonbin = (BYTE*)fpac + fpac->headerSize + oldJonbinOffset;
		
		DWORD oldSortedSpriteIndex = &spriteToDelete - sortedSprites.data();
		
		BYTE* lookupStart = (BYTE*)spriteToDelete.name + headerElementSize;
		BYTE* lookupEnd = fpac->lookupEnd();
		if (lookupStart != lookupEnd) {
			relocateEntityJonbins(lookupStart, lookupEnd, lookupStart - headerElementSize);
			BYTE* memmoveEnd = lookupEnd - headerElementSize;
			memmove(spriteToDelete.name, spriteToDelete.name + headerElementSize, (char*)lookupEnd - spriteToDelete.name - headerElementSize);
			
			if (fpac->size0x50()) {
				for (
					FPACLookupElement0x50* lookupPtr = (FPACLookupElement0x50*)spriteToDelete.name;
					lookupPtr < (FPACLookupElement0x50*)memmoveEnd;
					++lookupPtr
				) {
					--lookupPtr->index;
				}
			} else {
				for (
					FPACLookupElement0x30* lookupPtr = (FPACLookupElement0x30*)spriteToDelete.name;
					lookupPtr < (FPACLookupElement0x30*)memmoveEnd;
					++lookupPtr
				) {
					--lookupPtr->index;
				}
			}
			
			auto itEnd = jonbinToLookupUE3.end();
			for (auto it = jonbinToLookupUE3.begin(); it != itEnd; ++it) {
				if (it->second >= lookupStart) {
					it->second = (BYTE*)it->second - headerElementSize;
				}
			}
			
			for (SortedSprite& sortedSprite : sortedSprites) {
				if ((BYTE*)sortedSprite.name >= lookupStart) {
					sortedSprite.name -= headerElementSize;
				}
			}
		
		}
		if (fpac->rawSize == 0x20 + fpac->count * headerElementSize) {
			fpac->rawSize -= headerElementSize;
		}
		--fpac->count;
		
		spriteToDelete.freeMemory();
		sortedSprites.erase(sortedSprites.begin() + oldSortedSpriteIndex);
		
		newHashMap.erase(modifHash);
		oldNameToSortedSpriteIndex.erase(oldHash);
		
		auto itEndHash = oldNameToSortedSpriteIndex.end();
		for (auto it = oldNameToSortedSpriteIndex.begin(); it != itEndHash; ++it) {
			if (it->second >= oldSortedSpriteIndex) {
				--it->second;
			}
		}
		return;
	}
	
	const char* nameUse = spriteToDelete.newName[0] ? spriteToDelete.newName : spriteToDelete.name;
	DWORD hash = Entity::hashStringLowercase(nameUse);
	
	if (spriteToDelete.deleted) {
		newHashMap.insert(hash);
		spriteToDelete.deleted = false;
	} else {
		newHashMap.erase(hash);
		spriteToDelete.deleted = true;
	}
	
}

void FPACSecondaryData::hitboxEditorDeleteOperationMoveIndices(BYTE* jonbin, int hitboxCount, int startInd, int endInd) {
	
	BYTE numTypes = HitboxHolder::numTypes(jonbin);
	
	short* counts = HitboxHolder::hitboxCounts(jonbin);
	
	Hitbox* hitboxesStart = HitboxHolder::hitboxesStart(jonbin);
	
	if (endInd != hitboxCount) {
		memmove(hitboxesStart + startInd, hitboxesStart + endInd, sizeof (Hitbox) * (hitboxCount - endInd));
	}
	
	Hitbox* newHitboxStarts[17] { nullptr };
	short newCounts[17];
	
	int currentIndexOld = 0;
	int currentIndexNew = 0;
	for (BYTE type = 0; type < numTypes; ++type) {
		int countOld = counts[type];
		int countNew = countOld;
		int currentIndexOldEnd = currentIndexOld + countOld;
		if (currentIndexOldEnd > startInd && currentIndexOld < endInd) {
			int hitboxLeft = max(currentIndexOld, startInd);
			int hitboxRight = min(currentIndexOldEnd, endInd);
			short hitboxesToDelete = (short)(hitboxRight - hitboxLeft);
			countNew -= hitboxesToDelete;
			counts[type] = countNew;  // modify JONBIN data
		}
		newHitboxStarts[type] = hitboxesStart + currentIndexNew;
		newCounts[type] = countNew;
		currentIndexNew += countNew;
		currentIndexOld += countOld;
	}
	
	if (numTypes < 17) {
		memset(newCounts + numTypes, 0, 2 * (17 - numTypes));
	}
	
	for (int entityIndex = 0; entityIndex < entityList.count; ++entityIndex) {
		Entity ent = entityList.list[entityIndex];
		HitboxHolder* hitboxHolder = ent.hitboxes();
		if (hitboxHolder->jonbinPtr == jonbin) {
			for (int hitboxTypeInd = 0; hitboxTypeInd < 17; ++hitboxTypeInd) {
				hitboxHolder->data[hitboxTypeInd] = newHitboxStarts[hitboxTypeInd];
				hitboxHolder->count[hitboxTypeInd] = newCounts[hitboxTypeInd];
			}
		}
	}
}

FPACSecondaryData::~FPACSecondaryData() {
	for (SortedSprite& sortedSprite : sortedSprites) {
		sortedSprite.freeMemory();
	}
}

DWORD FPACSecondaryData::findSortedSpriteInsertionIndex(const char* str) const {
	const std::vector<SortedSprite>& ar = sortedSprites;
	if (ar.empty()) return 0;
	int start = 0;
	int end = (int)ar.size() - 1;
	while (true) {
		int mid = (start + end) >> 1;
		const SortedSprite& arMember = ar[mid];
		const char* arPtr = arMember.newName[0] ? arMember.newName : arMember.name;
		int cmpResult = strcmp(str, arPtr);
		if (cmpResult == 0) {
			return (DWORD)mid;
		} else if (cmpResult < 0) {
			end = mid - 1;
			if (mid == start) {
				return (DWORD)start;
			}
		} else {
			start = mid + 1;
			if (mid == end) {
				return (DWORD)start;
			}
		}
	}
}

template<typename T, bool canStealJonbins>
void FPACSecondaryData::replaceFpac(SourceFpacProvier* provider) {
	
	newSpriteCounter = 0;
	
	// we can use this to fix all damage done to sorted sprites later without nuking layers
	struct ReplaceFpacCachedValue {
		DWORD oldHash;
		SortedSprite* sortedSprite;
		ReplaceFpacCachedValue() { }
	};
	std::unordered_map<DWORD /* new name's hash */, ReplaceFpacCachedValue> newHashToSortedSprite;
	newHashToSortedSprite.reserve(sortedSprites.size());
	
	int numOfEntriesToDelete = 0;
	int numOfEntriesThatStay = 0;
	
	for (SortedSprite& sortedSprite : sortedSprites) {
		if (sortedSprite.added) ++numOfEntriesToDelete;
		else ++numOfEntriesThatStay;
		sortedSprite.deleted = true;
		DWORD hash;
		if (sortedSprite.newName[0]) {
			hash = Entity::hashStringLowercase(sortedSprite.newName);
		} else {
			hash = ((T*)sortedSprite.name)->hash;
		}
		ReplaceFpacCachedValue& newVal = newHashToSortedSprite[hash];
		newVal.sortedSprite = &sortedSprite;
		newVal.oldHash = ((T*)sortedSprite.name)->hash;
	}
	
	FPAC* fpac = Collision->TopData;
	
	struct NewSprite {
		char oldName[32];
		char newName[32];
		DWORD hash;
		DWORD insertionIndex;
		BYTE* jonbin;
		DWORD size;
		T* lookup;
	};
	std::vector<NewSprite> newSprites;
	
	provider->rewind();
	while (provider->getNext()) {
		DWORD hash = provider->getHash();
		auto found = newHashToSortedSprite.find(hash);
		if (found == newHashToSortedSprite.end()) {
			newSprites.emplace_back();
			NewSprite& newSprite = newSprites.back();
			generateNewSpriteName(newSprite.oldName, &newSprite.hash, &newSprite.insertionIndex);
			newHashMap.insert(newSprite.hash);
			if (canStealJonbins) {
				newSprite.jonbin = provider->getJonbin();
				newSprite.size = provider->getSize();
				provider->forgetJonbin();
			} else {
				DWORD size = provider->getSize();
				newSprite.jonbin = (BYTE*)malloc(size);
				if (!newSprite.jonbin) {
					throw std::bad_alloc();
				}
				memcpy(newSprite.jonbin, provider->getJonbin(), size);
				newSprite.size = size;
			}
			memcpy(newSprite.newName, provider->getName(), 32);
		} else {
			SortedSprite* sortedSprite = found->second.sortedSprite;
			if (sortedSprite->deleted) {
				sortedSprite->deleted = false;
				if (sortedSprite->added) {
					--numOfEntriesToDelete;
					++numOfEntriesThatStay;
				}
			}
			BYTE* providerJonbin = provider->getJonbin();
			DWORD providerSize = provider->getSize();
			BYTE* oldJonbin = ((T*)sortedSprite->name)->offset + (BYTE*)fpac + fpac->headerSize;
			
			HitboxHolder oldHitboxes;
			oldHitboxes.parse(oldJonbin);
			Hitbox* oldStart = oldHitboxes.hitboxesStart();
			
			HitboxHolder providerHitboxes;
			providerHitboxes.parse(providerJonbin);
			
			bool dataChanged = false;
			{
				BYTE numTypes = HitboxHolder::numTypes(oldJonbin);
				if (!(
					numTypes == HitboxHolder::numTypes(oldJonbin)
					&& providerHitboxes.nameCount == oldHitboxes.nameCount
					&& (providerHitboxes.nameCount == 0
						? true
						: strcmp(providerHitboxes.names[0], oldHitboxes.names[0]) == 0)
					&& memcmp(HitboxHolder::hitboxCounts(providerJonbin), HitboxHolder::hitboxCounts(oldJonbin), (numTypes - 3) * 2) == 0
					&& memcmp(HitboxHolder::hitboxesStart(providerJonbin), HitboxHolder::hitboxesStart(oldJonbin), oldHitboxes.hitboxCount() * sizeof (Hitbox)) == 0
				)) {
					dataChanged = true;
				}
			}
			
			if (dataChanged) {
				if (sortedSprite->layers) {
					free(sortedSprite->layers);
					sortedSprite->layers = nullptr;
					sortedSprite->layersSize = 0;
					sortedSprite->layersCapacity = 0;
				}
				if (sortedSprite->newJonbin || providerSize > ((T*)sortedSprite->name)->size) {
					sortedSprite->resizeJonbin(this, providerSize);
					memcpy(sortedSprite->newJonbin, providerJonbin, providerSize);
				} else {
					memcpy(oldJonbin, providerJonbin, providerSize);
				}
				((T*)sortedSprite->name)->size = providerSize;
			}
		}
	}
	
	int newNumEntriesTotal = numOfEntriesThatStay + newSprites.size();
	
	DWORD requiredSize = 0x20 + sizeof (T) * newNumEntriesTotal;
	if (Collision->DataSize < requiredSize) {
		FPAC* oldLoc = Collision->TopData;
		DWORD oldSize = Collision->DataSize;
		DWORD newSize = requiredSize + 100 * sizeof (T);
		if (!appRealloc) game.sigscanFNamesAndAppRealloc();
		FPAC* newFpac = (FPAC*)appRealloc(oldLoc, newSize, 8);
		if (!newFpac) throw std::bad_alloc();
		Collision->TopData = newFpac;
		Collision->DataSize = requiredSize;
		onFPACRelocate(oldLoc, Collision->TopData, oldSize, newSize);
		fpac = Collision->TopData;
	}
	
	auto deleteEnd = jonbinToLookupUE3.begin();
	auto itEnd = jonbinToLookupUE3.end();
	BYTE* end = (BYTE*)fpac + requiredSize;
	for (auto it = deleteEnd; it != itEnd; ++it) {
		BYTE* jonbin = (BYTE*)it->first;
		T* lookup = (T*)it->second;
		if (jonbin < end) {
			++deleteEnd;
			auto found = oldNameToSortedSpriteIndex.find(lookup->hash);
			if (found != oldNameToSortedSpriteIndex.end()) {
				SortedSprite& sortedSprite = sortedSprites[found->second];
				sortedSprite.resizeJonbin(this, lookup->size);
				memcpy(sortedSprite.newJonbin, jonbin, lookup->size);
				lookup->offset = sortedSprite.newJonbin - ((BYTE*)fpac + fpac->headerSize);
				// we'll relocate entity jonbins at the very end. Actually not, we'll just call spriteImpl on all of them
				if (sortedSprite.layers) {
					int disp = sortedSprite.newJonbin - jonbin;
					EditedHitbox* layer = sortedSprite.layers;
					for (int layerIndex = 0; layerIndex < (int)sortedSprite.layersSize; ++layerIndex) {
						layer->ptr += disp;
						++layer;
					}
				}
			}
		} else {
			break;
		}
	}
	jonbinToLookupUE3.erase(jonbinToLookupUE3.begin(), deleteEnd);
	DWORD oldCount = fpac->count;
	fpac->count = newNumEntriesTotal;
	if (jonbinToLookupUE3.empty()) {
		fpac->rawSize = 0x20 + fpac->count * sizeof (T);
	}
	
	T* lookupStart = (T*)((BYTE*)fpac + 0x20);
	T* sourceLookup = lookupStart;
	T* lookupEnd = (T*)((BYTE*)fpac + 0x20 + oldCount * sizeof (T));
	int count = (int)fpac->count;
	T* deletionStart = nullptr;
	std::vector<Entity> entitiesThatNeedToMentallyHeal;
	entityList.populate();
	for (int sourceLookupIndex = 0; sourceLookupIndex < count + 1; ++sourceLookupIndex) {
		bool needsDeletion = false;
		if (sourceLookupIndex < count) {
			auto found = oldNameToSortedSpriteIndex.find(sourceLookup->hash);
			if (found != oldNameToSortedSpriteIndex.end()) {
				SortedSprite& sortedSprite = sortedSprites[found->second];
				if (sortedSprite.deleted && sortedSprite.added) {
					needsDeletion = true;
					for (int i = 0; i < entityList.count; ++i) {
						Entity ent = entityList.list[i];
						if (ent.hitboxes()->jonbinPtr == sortedSprite.newJonbin) {
							entitiesThatNeedToMentallyHeal.push_back(ent);
						}
					}
					#if _DEBUG
					if (!sortedSprite.newJonbin) throw std::logic_error("Am I going insane??");
					#endif
					free(sortedSprite.newJonbin);
					sortedSprite.newJonbinSize = 0;
					sortedSprite.newJonbinCapacity = 0;
					sortedSprite.newJonbin = nullptr;
					if (sortedSprite.layers) {
						free(sortedSprite.layers);
						sortedSprite.layersSize = 0;
						sortedSprite.layersCapacity = 0;
						sortedSprite.layers = nullptr;
					}
				}
			}
		}
		if (needsDeletion) {
			if (!deletionStart) {
				deletionStart = sourceLookup;
			}
		} else if (deletionStart) {
			T* replacementLookup;
			if (sourceLookup < lookupEnd) {
				memmove(deletionStart, sourceLookup, (BYTE*)lookupEnd - (BYTE*)sourceLookup);
				replacementLookup = deletionStart;
				sourceLookup = deletionStart;
				lookupEnd -= (sourceLookup - deletionStart);
			} else if (deletionStart > lookupStart) {
				replacementLookup = deletionStart - 1;
			} else {
				replacementLookup = nullptr;
			}
			
			char replacementSprite[32] { "null" };
			if (replacementLookup) {
				memcpy(replacementSprite, replacementLookup->spriteName, 32);
			}
			if (endScene.spriteImpl) {
				for (Entity ent : entitiesThatNeedToMentallyHeal) {
					endScene.spriteImpl((void*)ent.ent, replacementSprite, 1);
					game.allowTickForActor(ent.pawnWorld());
				}
			}
			entitiesThatNeedToMentallyHeal.clear();
			deletionStart = nullptr;
		}
		++sourceLookup;
	}
	
	struct MySort {
		static int __cdecl sort(const void* Ptr1, const void* Ptr2) {
			const NewSprite* sprite1 = (const NewSprite*)Ptr1;
			const NewSprite* sprite2 = (const NewSprite*)Ptr2;
			if (sprite1->insertionIndex == sprite2->insertionIndex) {
				DWORD hash1 = sprite1->hash;
				DWORD hash2 = sprite2->hash;
				if (hash1 == hash2) return 0;
				else if (hash1 < hash2) return -1;
				else return 1;
			} else {
				return sprite1->insertionIndex - sprite2->insertionIndex;
			}
		}
	};
	
	qsort(newSprites.data(), newSprites.size(), sizeof (NewSprite), MySort::sort);
	
	DWORD prevInsertionIndex = (DWORD)-1;
	DWORD indexOffset = 0;
	oldCount = fpac->count - newSprites.size();
	DWORD spritesToInsert = 0;
	DWORD prevSpriteIndex = (DWORD)-1;
	
	size_t newSpriteCount = newSprites.size();
	for (size_t newSpriteIndex = 0; newSpriteIndex < newSpriteCount + 1; ++newSpriteIndex) {
		DWORD currentInsertionIndex;
		if (newSpriteIndex < newSpriteCount) {
			currentInsertionIndex = newSprites[newSpriteIndex].insertionIndex;
		} else {
			currentInsertionIndex = (DWORD)-1;
		}
		if (currentInsertionIndex != prevInsertionIndex) {
			if (prevInsertionIndex != (DWORD)-1) {
				T* newLookup = lookupStart + prevInsertionIndex + indexOffset;
				if (prevInsertionIndex != oldCount) {
					memmove(lookupStart + prevInsertionIndex + indexOffset + spritesToInsert,
						newLookup,
						(oldCount - prevInsertionIndex) * sizeof (T));
				}
				NewSprite* newSprite = &newSprites[prevSpriteIndex];
				for (int i = 0; i < (int)spritesToInsert; ++i) {
					memcpy(newLookup->spriteName, newSprite->oldName, 32);
					newLookup->hash = newSprite->hash;
					newLookup->offset = newSprite->jonbin - ((BYTE*)fpac + fpac->headerSize);
					newSprite->lookup = newLookup;
					++newLookup;
					++newSprite;
				}
				indexOffset += spritesToInsert;
			}
			
			spritesToInsert = 1;
			prevInsertionIndex = currentInsertionIndex;
			prevSpriteIndex = newSpriteIndex;
		} else {
			++spritesToInsert;
		}
	}
	
	DWORD maxOffset = fpac->rawSize;
	sourceLookup = lookupStart;
	for (int i = 0; i < (int)fpac->count; ++i) {
		sourceLookup->index = i;
		if (sourceLookup->offset < maxOffset) {
			jonbinToLookupUE3[(BYTE*)fpac + fpac->headerSize + sourceLookup->offset] = sourceLookup;
		}
		++sourceLookup;
	}
	
	auto newHashToSortedSpriteEnd = newHashToSortedSprite.end();
	for (auto it = newHashToSortedSprite.begin(); it != newHashToSortedSpriteEnd; ++it) {
		DWORD oldHash = it->second.oldHash;
		SortedSprite* sortedSprite = it->second.sortedSprite;
		if (!(sortedSprite->added && sortedSprite->deleted)) {
			sortedSprite->name = (char*)fpac->findLookupEntry(oldHash);
		}
	}
	
	SortedSprite* spriteDeleteStart = nullptr;
	SortedSprite* allSpritesEnd = sortedSprites.data() + sortedSprites.size();
	SortedSprite* sortedSpriteIter = sortedSprites.data();
	for (; sortedSpriteIter <= allSpritesEnd; ++sortedSpriteIter) {
		bool needDelete;
		if (sortedSpriteIter < allSpritesEnd) {
			needDelete = sortedSpriteIter->added && sortedSpriteIter->deleted;
		} else {
			needDelete = false;
		}
		if (needDelete) {
			if (!spriteDeleteStart) spriteDeleteStart = sortedSpriteIter;
		} else if (spriteDeleteStart) {
			if (sortedSpriteIter < allSpritesEnd) {
				memmove(spriteDeleteStart, sortedSpriteIter, (BYTE*)allSpritesEnd - (BYTE*)sortedSpriteIter);
			}
			allSpritesEnd -= (sortedSpriteIter - spriteDeleteStart);
			sortedSpriteIter = spriteDeleteStart;
			spriteDeleteStart = nullptr;
		}
	}
	
	sortedSprites.resize(sortedSprites.size() - numOfEntriesToDelete);
	
	for (NewSprite& newSprite : newSprites) {
		DWORD index = findSortedSpriteInsertionIndex(newSprite.newName);
		SortedSprite& sortedSprite = *sortedSprites.insert(sortedSprites.begin() + index, {
			(char*)newSprite.lookup,
			false,
			true
		});
		sortedSprite.newJonbin = newSprite.jonbin;
		sortedSprite.newJonbinSize = newSprite.size;
		sortedSprite.newJonbinCapacity = newSprite.size;
		memcpy(sortedSprite.newName, newSprite.newName, 32);
	}
	
	newHashMap.clear();  // we put generated (old) names into this, now it's ruined and must be redone from scratch
	oldNameToSortedSpriteIndex.clear();
	SortedSprite* sortedSpritesStart = sortedSprites.data();
	for (SortedSprite& sortedSprite : sortedSprites) {
		oldNameToSortedSpriteIndex[((T*)sortedSprite.name)->hash] = &sortedSprite - sortedSpritesStart;
		newHashMap.insert(
			Entity::hashStringLowercase(
				sortedSprite.newName[0]
					? sortedSprite.newName
					: sortedSprite.name
			)
		);
	}
	
	if (endScene.spriteImpl) {
		char spriteName[32];
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			int entBBScrIndexInAswEng;
			if (ent.bbscrIndexInAswEng() == 1
					&& entityList.slots[0].characterType() == entityList.slots[1].characterType()) {
				entBBScrIndexInAswEng = 0;
			} else {
				entBBScrIndexInAswEng = ent.bbscrIndexInAswEng();
			}
			if (entBBScrIndexInAswEng == bbscrIndexInAswEng) {
				memcpy(spriteName, ent.spriteName(), 32);
				endScene.spriteImpl((void*)ent.ent, spriteName, 1);
				game.allowTickForActor(ent.pawnWorld());
			}
		}
	}
	
}

template void FPACSecondaryData::replaceFpac<FPACLookupElement0x50, true>(SourceFpacProvier* provider);
template void FPACSecondaryData::replaceFpac<FPACLookupElement0x30, true>(SourceFpacProvier* provider);
template void FPACSecondaryData::replaceFpac<FPACLookupElement0x50, false>(SourceFpacProvier* provider);
template void FPACSecondaryData::replaceFpac<FPACLookupElement0x30, false>(SourceFpacProvier* provider);

void SortedSprite::convertToLayers(FPACSecondaryData* mainData) {
	if (layers) return;
	LayerIteratorIgnoreLayers layerIterator(mainData->Collision->TopData, this);
	int count = layerIterator.count();
	resizeLayers(count);
	EditedHitbox* layer = layers;
	while (layerIterator.getNext()) {
		layerIterator.copyTo(layer);
		++layer;
	}
}
