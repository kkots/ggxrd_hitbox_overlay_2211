#include "pch.h"
#include "UndoOperations.h"
#include "EntityList.h"
#include "Game.h"
#include "UI.h"
#include "EndScene.h"
#include "GifMode.h"

const char* undoOperationName[UNDO_OPERATION_TYPE_LAST] {
	"I fucked something up",
	"Delete hitboxes",
	"Delete hitboxes",
	"Add sprite",
	"Delete sprite",
	"Rename sprite",
	"Set anim sequence and/or frame",
	"Reorder layers",
	"Reorder layers",
	"Add box",
	"Move and/or resize box(es)"
};

static int CompareInt(const void* Ptr1, const void* Ptr2) {
	return *(int*)Ptr1 - *(int*)Ptr2;
}

void UndoOperationBase::restoreView() {
	if (!gifMode.editHitboxes) return;
	refresh();
	Entity newEntity = nullptr;
	if (isPawn) {
		newEntity = backupEntity;
	} else if (strcmp(spriteName, "null") == 0) {
		
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			if (ent.bbscrIndexInAswEng() == bbscrIndexInAswEng
					&& strcmp(ent.spriteName(), "null") == 0) {
				newEntity = ent;
				break;
			}
		}
		
	} else {
		for (int i = 2; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			if (ent.bbscrIndexInAswEng() == bbscrIndexInAswEng
					&& ent.hitboxes()->jonbinPtr == jonbin) {
				newEntity = ent;
				break;
			}
		}
		
		if (!newEntity) {
			
			for (int i = 2; i < entityList.count; ++i) {
				Entity ent = entityList.list[i];
				if (ent.bbscrIndexInAswEng() == bbscrIndexInAswEng
						&& ent.bbscrCurrentFunc() == bbscrFunc) {
					newEntity = ent;
					break;
				}
			}
			
		}
	}
	
	if (!newEntity) return;
	
	if (newEntity != gifMode.editHitboxesEntity) {
		ui.editHitboxesChangeView(newEntity);  // this function also checks newEntity != gifMode.editHitboxesEntity. Let's all check newEntity != gifMode.editHitboxesEntity. The more checks the better
		// this will also center the view on that entity
	}
	
	bool needUpdateModel = false;
	if (strcmp(newEntity.spriteName(), spriteName) != 0) {
		needUpdateModel = true;
	} else {
		REDPawn* pawnWorld = newEntity.pawnWorld();
		if (pawnWorld) {
			const REDAnimNodeSequence* animSeq = pawnWorld->getFirstAnimSeq();
			const FName* currentAnimSeqName = &animSeq->AnimSeqName;
			if (*currentAnimSeqName != animSeqName || animSeq->CurrentFrame != currentFrame) {
				needUpdateModel = true;
			}
		}
	}
	
	if (endScene.spriteImpl && needUpdateModel) {
		endScene.spriteImpl((void*)newEntity.ent, spriteName, true);
		game.allowTickForActor(newEntity.pawnWorld());
	}
	
	memcpy(&settings.hitboxList, &hitboxList, sizeof hitboxList);
	
	ui.setSelectedHitboxes(selectedHitboxes);
}

int UndoOperationBase::find(const std::vector<int>& array, int elem) {
	int size = (int)array.size();
	for (int i = 0; i < size; ++i) {
		if (array[i] == elem) return i;
	}
	return -1;
}

void UndoOperationBase::refresh() const {
	DWORD hash = Entity::hashStringLowercase(spriteName);
	auto found = secondaryData->oldNameToSortedSpriteIndex.find(hash);
	if (found == secondaryData->oldNameToSortedSpriteIndex.end()) {
		sortedSprite = nullptr;
		jonbin = nullptr;
		return;
	}
	sortedSprite = &secondaryData->sortedSprites[found->second];
	DWORD offset;
	if (fpac->size0x50()) {
		offset = ((FPACLookupElement0x50*)sortedSprite->name)->offset;
	} else {
		offset = ((FPACLookupElement0x30*)sortedSprite->name)->offset;
	}
	jonbin = (BYTE*)fpac + fpac->headerSize + offset;
}

void DeleteLayersOperation::fill() {
	fillFromEnt(Entity{gifMode.editHitboxesEntity}, UNDO_OPERATION_TYPE_DELETE_HITBOXES);
	boxesToDelete = ui.selectedHitboxes;
}

bool DeleteLayersOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	
	if (!jonbin) {
		ui.showErrorDlg("Can't delete layers on the 'null' sprite.", true);
		return false;
	}
	
	ThreadUnsafeSharedPtr<UndoDeleteLayersOperation> newOp = new ThreadUnsafeSharedResource<UndoDeleteLayersOperation>(*(UndoOperationBase*)this);
	*oppositeOperation = newOp;
	
	if (fpac->size0x50()) {
		FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
		newOp->oldJonbinSize = lookupEntry->size;
	} else {
		FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
		newOp->oldJonbinSize = lookupEntry->size;
	}
	
	std::vector<int> realIndices;
	HitboxHolder hitboxes;
	hitboxes.parse(jonbin);
	
	int hitboxCount = hitboxes.hitboxCount();
	
	Hitbox* hitboxesStart = hitboxes.hitboxesStart();
	
	for (int hitboxType = 0; hitboxType < 17; ++hitboxType) {
		newOp->oldCounts[hitboxType] = hitboxes.count[hitboxType];
		newOp->oldHitboxesData[hitboxType] = hitboxes.data[hitboxType] - hitboxesStart;
	}
	
	newOp->oldHitboxes.resize(hitboxCount);
	memcpy(newOp->oldHitboxes.data(), hitboxesStart, hitboxCount * sizeof (Hitbox));
		
	EditedHitbox* layer;
	
	if (!sortedSprite->layers) {
		// always include pushbox
		sortedSprite->resizeLayers(hitboxCount + 1  // add pushbox
			);
		
		layer = sortedSprite->layers;
		
		LayerIterator layerIterator(jonbin);
		while (layerIterator.getNext()) {
			layerIterator.copyTo(layer);
			++layer;
		}
	}
	
	newOp->oldLayers.resize(hitboxCount + 1);
	memcpy(newOp->oldLayers.data(), sortedSprite->layers, (hitboxCount + 1) * sizeof (EditedHitbox));
	EditedHitbox* oldLayersElement = newOp->oldLayers.data();
	for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
		oldLayersElement->ptr = (Hitbox*)(
			oldLayersElement->ptr - hitboxesStart
		);
		++oldLayersElement;
	}
	
	newOp->oldSelection = boxesToDelete;
	
	layer = sortedSprite->layers;
	for (int i = 0; i < (int)sortedSprite->layersSize; ++i) {
		if (!layer->isPushbox && find(boxesToDelete, layer->originalIndex) != -1) {
			realIndices.push_back(layer->ptr - hitboxesStart);
		}
		++layer;
	}
	
	Entity editEntity;
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
		editEntity = gifMode.editHitboxesEntity;
		if (editEntity.hitboxes()->jonbinPtr == jonbin) {
			ui.selectedHitboxes.clear();
		}
	}
	
	qsort(realIndices.data(), realIndices.size(), 4, CompareInt);
	
	layer = sortedSprite->layers;
	for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
		if (!layer->isPushbox) {
			int thisLayerRealIndex = layer->ptr - hitboxesStart;
			bool thisLayerActuallyGone = false;
			for (int realIndex : realIndices) {
				if (realIndex < thisLayerRealIndex) {
					--layer->ptr;
				} else {
					if (realIndex == thisLayerRealIndex) {
						thisLayerActuallyGone = true;
					}
					break;
				}
			}
			if (thisLayerActuallyGone) {
				if (layerIndex != (int)sortedSprite->layersSize - 1) {
					memmove(layer, layer + 1, ((int)sortedSprite->layersSize - layerIndex - 1) * sizeof (EditedHitbox));
				}
				--sortedSprite->layersSize;
				--layer;
				--layerIndex;
			}
		}
		++layer;
	}
	
	int prevRealIndex = -1;
	int realIndexStart = -1;
	int indexShift = 0;
	for (int realIndex : realIndices) {
		if (realIndex - prevRealIndex > 1 && realIndexStart != -1) {
			secondaryData->hitboxEditorDeleteOperationMoveIndices(jonbin,
				hitboxCount,
				realIndexStart - indexShift,
				prevRealIndex + 1 - indexShift);
			int numBoxesDeleted = prevRealIndex + 1 - realIndexStart;
			indexShift += numBoxesDeleted;
			realIndexStart = -1;
			hitboxCount -= numBoxesDeleted;
		}
		if (realIndexStart == -1) {
			realIndexStart = realIndex;
		}
		prevRealIndex = realIndex;
	}
	secondaryData->hitboxEditorDeleteOperationMoveIndices(jonbin,
		hitboxCount,
		realIndexStart - indexShift,
		prevRealIndex + 1 - indexShift);
	
	DWORD deletedSize = realIndices.size() * sizeof (Hitbox);
	if (sortedSprite->newJonbin) {
		sortedSprite->resizeJonbin(secondaryData, sortedSprite->newJonbinSize - deletedSize);
	}
	if (fpac->size0x50()) {
		FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
		lookupEntry->size -= deletedSize;
	} else {
		FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
		lookupEntry->size -= deletedSize;
	}
	
	return true;
}

UndoDeleteLayersOperation::UndoDeleteLayersOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

UndoOperationBase::UndoOperationBase(const UndoOperationBase& other) {
	type = other.type;
	backupEntity = other.backupEntity;
	bbscrIndexInAswEng = other.bbscrIndexInAswEng;
	isPawn = other.isPawn;
	memcpy(spriteName, other.spriteName, sizeof spriteName);
	selectedHitboxes = other.selectedHitboxes;
	secondaryData = other.secondaryData;
	fpac = secondaryData->Collision->TopData;
	bbscrFunc = other.bbscrFunc;
	animSeqName = other.animSeqName;
	currentFrame = other.currentFrame;
	memcpy(&hitboxList, &other.hitboxList, sizeof hitboxList);
	extern unsigned int getUE3EngineTick();
	engineTick = getUE3EngineTick();
}

UndoOperationBase::UndoOperationBase(Entity ent) {
	fillFromEnt(ent, UNDO_OPERATION_TYPE_YOU_FUCKED_UP);
}

void UndoOperationBase::fillFromEnt(Entity ent, UndoOperationType type) {
	this->type = type;
	backupEntity = ent;
	bbscrIndexInAswEng = ent.bbscrIndexInAswEng();
	isPawn = ent.isPawn();
	selectedHitboxes = ui.selectedHitboxes;
	secondaryData = &ui.hitboxEditorFPACSecondaryData[bbscrIndexInAswEng];
	fpac = secondaryData->Collision->TopData;
	bbscrFunc = ent.bbscrCurrentFunc();
	animSeqName.low = 0;
	animSeqName.high = 0;
	currentFrame = 0;
	memcpy(spriteName, ent.spriteName(), 32);
	memcpy(&hitboxList, &settings.hitboxList, sizeof hitboxList);
	REDPawn* pawnWorld = ent.pawnWorld();
	if (pawnWorld) {
		REDAnimNodeSequence* animSeq = pawnWorld->getFirstAnimSeq();
		if (animSeq) {
			animSeqName = animSeq->AnimSeqName;
			currentFrame = animSeq->CurrentFrame;
		}
	}
	extern unsigned int getUE3EngineTick();
	engineTick = getUE3EngineTick();
}

bool UndoDeleteLayersOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	
	if (!jonbin) {
		ui.showErrorDlg("Can't undo deletion of layers on the 'null' sprite.", true);
		return false;
	}
	
	ThreadUnsafeSharedPtr<DeleteLayersOperation> newOp = new ThreadUnsafeSharedResource<DeleteLayersOperation>(*(UndoOperationBase*)this);
	newOp->boxesToDelete = oldSelection;
	*oppositeOperation = std::move(newOp);
	
	if (sortedSprite->newJonbin) {
		sortedSprite->resizeJonbin(secondaryData, oldJonbinSize);
		// assume memory did not change location
	}
	
	if (fpac->size0x50()) {
		FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
		lookupEntry->size = oldJonbinSize;
	} else {
		FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
		lookupEntry->size = oldJonbinSize;
	}
	
	HitboxHolder hitboxes;
	hitboxes.parse(jonbin);
	
	Hitbox* hitboxesStart = hitboxes.hitboxesStart();
	
	memcpy(hitboxesStart, oldHitboxes.data(), oldHitboxes.size() * sizeof (Hitbox));
	
	int oldCountsInt[17];
	Hitbox* oldHitboxesDataPtrs[17];
	for (int hitboxType = 0; hitboxType < 17; ++hitboxType) {
		oldCountsInt[hitboxType] = oldCounts[hitboxType];
		oldHitboxesDataPtrs[hitboxType] = hitboxesStart + oldHitboxesData[hitboxType];
	}
	
	BYTE numTypes = HitboxHolder::numTypes(jonbin);
	short* countPtr = HitboxHolder::hitboxCounts(jonbin);
	for (BYTE hitboxType = 0; hitboxType < numTypes; ++hitboxType) {
		*countPtr = oldCounts[hitboxType];
		++countPtr;
	}
	
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		HitboxHolder* entHitboxes = ent.hitboxes();
		if (entHitboxes->jonbinPtr == jonbin) {
			memcpy(entHitboxes->count, oldCountsInt, sizeof oldCountsInt);
			memcpy(entHitboxes->data, oldHitboxesDataPtrs, sizeof oldHitboxesDataPtrs);
		}
	}
	
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity
			&& Entity{gifMode.editHitboxesEntity}.hitboxes()->jonbinPtr == jonbin) {
		ui.setSelectedHitboxes(oldSelection);
	}
	
	sortedSprite->resizeLayers(oldLayers.size());
	memcpy(sortedSprite->layers, oldLayers.data(), oldLayers.size() * sizeof (EditedHitbox));
	EditedHitbox* layer = sortedSprite->layers;
	for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
		layer->ptr = (Hitbox*)(
			(int)layer->ptr + hitboxesStart
		);
		++layer;
	}
	
	return true;
}

DeleteLayersOperation::DeleteLayersOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

DeleteLayersOperation::DeleteLayersOperation() : UndoOperationBase() { }

AddSpriteOperation::AddSpriteOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

AddSpriteOperation::AddSpriteOperation() : UndoOperationBase() { }

void AddSpriteOperation::fill() {
	fillFromEnt(Entity{gifMode.editHitboxesEntity}, UNDO_OPERATION_TYPE_ADD_SPRITE);
	secondaryData->parseAllSprites();
	secondaryData->generateNewSpriteName(newSpriteName, &newSpriteHash, &insertionIndex);
	memcpy(oldSpriteName, newSpriteName, 32);
	oldSpriteHash = newSpriteHash;
}

template<typename T>
void fillNewLookupEntry(T* newEntry, DWORD offset, const char* name, DWORD hash, DWORD index, DWORD size) {
	newEntry->index = index;
	memcpy(newEntry->spriteName, name, 32);
	newEntry->offset = offset;
	newEntry->size = size;
	newEntry->hash = hash;
}

bool AddSpriteOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	secondaryData->newHashMap.insert(newSpriteHash);
	
	char oldSpriteToSelectAfterDelete[32];
	oldSpriteToSelectAfterDelete[0] = '\0';
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
		Entity editEntity{gifMode.editHitboxesEntity};
		if (editEntity.hitboxes()->jonbinPtr == jonbin) {
			memcpy(oldSpriteToSelectAfterDelete, editEntity.spriteName(), 32);
		}
	}
	
	int fpacCountFromBeforeAllocateNewLookupEntry = fpac->count;
	secondaryData->allocateNewLookupEntry();
	// after adding a lookup entry fpac may relocate and sorted sprites array and map may completely change
	
	DWORD neededSpaceForNewEntry;
	if (sortedSprite) {
		neededSpaceForNewEntry = fpac->size0x50()
			? ((FPACLookupElement0x50*)sortedSprite->name)->size
			: ((FPACLookupElement0x30*)sortedSprite->name)->size;
	} else {  // null sprite
		neededSpaceForNewEntry = 4  // JONB
			+ 2  // name count
			+ 1  // type count
			+ 2 * 0x14;  // hitbox count
	}
	
	if (insertionIndex != fpacCountFromBeforeAllocateNewLookupEntry) {
		
		DWORD headerElementSize = fpac->elementSize();
		BYTE* lookupRelocStart = (BYTE*)fpac + 0x20 + insertionIndex * headerElementSize;
		BYTE* lookupRelocEnd = (BYTE*)fpac + 0x20 + fpacCountFromBeforeAllocateNewLookupEntry * headerElementSize;
		
		secondaryData->relocateEntityJonbins(lookupRelocStart, lookupRelocEnd, lookupRelocStart + headerElementSize);
		
		int sortedSpritesCount = (int)secondaryData->sortedSprites.size();
		SortedSprite* ptrPtr = secondaryData->sortedSprites.data();
		for (int i = 0; i < sortedSpritesCount; ++i) {
			SortedSprite& ptr = *ptrPtr;
			if ((BYTE*)ptr.name >= lookupRelocStart) {
				ptr.name += headerElementSize;
			}
			++ptrPtr;
		}
		
		auto itEnd = secondaryData->jonbinToLookupUE3.end();
		for (auto it = secondaryData->jonbinToLookupUE3.begin(); it != itEnd; ++it) {
			if (it->second >= lookupRelocStart) {
				it->second = (BYTE*)it->second + headerElementSize;
			}
		}
		
		if (fpac->size0x50()) {
			for (int i = fpacCountFromBeforeAllocateNewLookupEntry - 1; i >= (int)insertionIndex; --i) {
				fpac->data.table0x50.elements[i + 1] = fpac->data.table0x50.elements[i];
				++fpac->data.table0x50.elements[i + 1].index;
			}
		} else {
			for (int i = fpacCountFromBeforeAllocateNewLookupEntry - 1; i >= (int)insertionIndex; --i) {
				fpac->data.table0x30.elements[i + 1] = fpac->data.table0x30.elements[i];
				++fpac->data.table0x30.elements[i + 1].index;
			}
		}
	}
	
	BYTE* lookupEntry;
	if (fpac->size0x50()) {
		lookupEntry = (BYTE*)fpac + 0x20 + 0x50 * insertionIndex;
		fillNewLookupEntry((FPACLookupElement0x50*)lookupEntry,
			0, oldSpriteName, oldSpriteHash, insertionIndex, neededSpaceForNewEntry);
	} else {
		lookupEntry = (BYTE*)fpac + 0x20 + 0x30 * insertionIndex;
		fillNewLookupEntry((FPACLookupElement0x30*)lookupEntry,
			0, oldSpriteName, oldSpriteHash, insertionIndex, neededSpaceForNewEntry);
	}
	
	DWORD insertionIndexSprites = secondaryData->findSortedSpriteInsertionIndex(newSpriteName);
	secondaryData->sortedSprites.insert(secondaryData->sortedSprites.begin() + insertionIndexSprites, { (char*)lookupEntry, false, true });
	SortedSprite& newSpriteElem = secondaryData->sortedSprites[insertionIndexSprites];
	
	const char* newName;
	if (strcmp(newSpriteName, oldSpriteName) != 0) {
		memcpy(newSpriteElem.newName, newSpriteName, 32);
		newName = newSpriteName;
	} else {
		memset(newSpriteElem.newName, 0, 32);
		newName = newSpriteElem.name;
	}
	
	newSpriteElem.resizeJonbin(secondaryData, neededSpaceForNewEntry);
	BYTE* newJonbin = newSpriteElem.newJonbin;
	DWORD jonbinOffset = (DWORD)(newJonbin - ((BYTE*)fpac + fpac->headerSize));
	
	if (fpac->size0x50()) {
		((FPACLookupElement0x50*)lookupEntry)->offset = jonbinOffset;
	} else {
		((FPACLookupElement0x30*)lookupEntry)->offset = jonbinOffset;
	}
	
	auto itEnd = secondaryData->oldNameToSortedSpriteIndex.end();
	for (auto it = secondaryData->oldNameToSortedSpriteIndex.begin(); it != itEnd; ++it) {
		if (it->second >= insertionIndexSprites) {
			++it->second;
		}
	}
	
	secondaryData->oldNameToSortedSpriteIndex[oldSpriteHash] = insertionIndexSprites;
	
	if (jonbin) {
		memcpy(newJonbin, jonbin, neededSpaceForNewEntry);
	} else {
		memcpy(newJonbin, "JONB", 4);
		BYTE* ptr = newJonbin + 4;
		*(short*)ptr = 0;  // name count
		ptr += 2;
		*ptr = 0x14;  // type count
		++ptr;
		for (int i = 0; i < 0x14; ++i) {
			*(short*)ptr = 0;
			ptr += 2;
		}
	}
	
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity
			&& Entity{gifMode.editHitboxesEntity}.hitboxes()->jonbinPtr == jonbin) {
		Entity editEntity{gifMode.editHitboxesEntity};
		endScene.spriteImpl((void*)editEntity.ent, oldSpriteName, true);
		game.allowTickForActor(editEntity.pawnWorld());
		ui.onNewSprite();
	}
	
	ThreadUnsafeSharedPtr<DeleteSpriteOperation> newOp = new ThreadUnsafeSharedResource<DeleteSpriteOperation>(*(UndoOperationBase*)this);
	memcpy(newOp->spriteName, oldSpriteName, 32);
	newOp->fpac = secondaryData->Collision->TopData;
	*oppositeOperation = newOp;
	memcpy(newOp->newSpriteName, newName, 32);
	memcpy(newOp->oldSpriteName, oldSpriteName, 32);
	memcpy(newOp->oldSpriteToSelectAfterDelete, oldSpriteToSelectAfterDelete, 32);
	
	return true;
}

DeleteSpriteOperation::DeleteSpriteOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

DeleteSpriteOperation::DeleteSpriteOperation() : UndoOperationBase() { }

void DeleteSpriteOperation::fill() {
	Entity editEntity{gifMode.editHitboxesEntity};
	fillFromEnt(editEntity, UNDO_OPERATION_TYPE_DELETE_SPRITE);
	memcpy(oldSpriteName, editEntity.spriteName(), 32);
	refresh();
	if (sortedSprite) {
		memcpy(newSpriteName, sortedSprite->newName[0] ? sortedSprite->newName : sortedSprite->name, 32);
	}
}

bool DeleteSpriteOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	if (!sortedSprite) {
		ui.showErrorDlg("Can't delete the 'null' sprite.", true);
		return false;
	}
	DWORD oldIndex;
	if (fpac->size0x50()) {
		FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
		oldIndex = lookupEntry - (FPACLookupElement0x50*)((BYTE*)fpac + 0x20);
	} else {
		FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
		oldIndex = lookupEntry - (FPACLookupElement0x30*)((BYTE*)fpac + 0x20);
	}
	if (!sortedSprite) {
		ui.showErrorDlg("Can't delete the 'null' sprite.", true);
		return false;
	}
	if (!sortedSprite->added) {
		DWORD newHash = Entity::hashStringLowercase(newSpriteName);
		if (sortedSprite->deleted) {
			auto found = secondaryData->newHashMap.find(newHash);
			if (found != secondaryData->newHashMap.end()) {
				const char* conflictingSpriteName = "<couldn't locate conflicting sprite>";
				for (const SortedSprite& seek : secondaryData->sortedSprites) {
					const char* nameUse = seek.newName[0] ? seek.newName : seek.name;
					if (Entity::hashStringLowercase(nameUse) == newHash) {
						conflictingSpriteName = nameUse;
						break;
					}
				}
				char msg[1024];
				sprintf_s(msg, "Can't undelete sprite '%s', because its hash clashes with another sprite name ('%s')."
					" Please resolve the conflict by renaming the other sprite.",
					newSpriteName,
					conflictingSpriteName);
				ui.showErrorDlg(msg, true);
				return false;
			}
			sortedSprite->deleted = false;
			secondaryData->newHashMap.insert(newHash);
		} else {
			sortedSprite->deleted = true;
			secondaryData->newHashMap.erase(newHash);
		}
		ThreadUnsafeSharedPtr<DeleteSpriteOperation> newOp = new ThreadUnsafeSharedResource<DeleteSpriteOperation>(*(UndoOperationBase*)this);
		*oppositeOperation = newOp;
		memcpy(newOp->newSpriteName, newSpriteName, 32);
		memcpy(newOp->oldSpriteName, oldSpriteName, 32);
		return true;
	}
	BYTE* oldjonbin = jonbin;
	DWORD replacementSpriteIndex;
	if (oldSpriteToSelectAfterDelete[0] == '\0') {
		DWORD currentSpriteIndex = sortedSprite - secondaryData->sortedSprites.data();
		if (currentSpriteIndex == secondaryData->sortedSprites.size() - 1) {
			replacementSpriteIndex = currentSpriteIndex - 1;
		} else {
			replacementSpriteIndex = currentSpriteIndex;
		}
	}
	// it's going to actually be gone
	secondaryData->deleteSprite(*sortedSprite);
	const char* selectedSpriteName;
	if (oldSpriteToSelectAfterDelete[0] == '\0') {
		SortedSprite* selectedSprite = &secondaryData->sortedSprites[replacementSpriteIndex];
		selectedSpriteName = selectedSprite->name;
	} else {
		selectedSpriteName = oldSpriteToSelectAfterDelete;
	}
	if (endScene.spriteImpl) {
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			if (ent.hitboxes()->jonbinPtr == oldjonbin) {
				endScene.spriteImpl((void*)ent.ent, selectedSpriteName, true);
				game.allowTickForActor(ent.pawnWorld());
			}
		}
	}
	ui.onNewSprite();
	ThreadUnsafeSharedPtr<AddSpriteOperation> newOp = new ThreadUnsafeSharedResource<AddSpriteOperation>(*(UndoOperationBase*)this);
	memcpy(newOp->spriteName, selectedSpriteName, 32);
	*oppositeOperation = newOp;
	memcpy(newOp->oldSpriteName, oldSpriteName, 32);
	memcpy(newOp->newSpriteName, newSpriteName, 32);
	newOp->insertionIndex = oldIndex;
	newOp->newSpriteHash = Entity::hashStringLowercase(newSpriteName);
	newOp->oldSpriteHash = Entity::hashStringLowercase(oldSpriteName);
	return true;
}

RenameSpriteOperation::RenameSpriteOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

RenameSpriteOperation::RenameSpriteOperation() : UndoOperationBase() { }

void RenameSpriteOperation::fill(const char(&newSpriteName)[32]) {
	fillFromEnt(gifMode.editHitboxesEntity, UNDO_OPERATION_TYPE_RENAME_SPRITE);
	memcpy(this->newSpriteName, newSpriteName, 32);
}

bool RenameSpriteOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	if (!sortedSprite) {
		ui.showErrorDlg("Can't rename the 'null' sprite.", true);
		return false;
	}
	ThreadUnsafeSharedPtr<RenameSpriteOperation> newOp = new ThreadUnsafeSharedResource<RenameSpriteOperation>(*(UndoOperationBase*)this);
	*oppositeOperation = newOp;
	const char* oldName = sortedSprite->newName[0] ? sortedSprite->newName : sortedSprite->name;
	memcpy(newOp->newSpriteName, oldName, 32);
	
	DWORD oldHash = Entity::hashStringLowercase(oldName);
	DWORD hash = Entity::hashStringLowercase(newSpriteName);
	
	std::unordered_set<DWORD>& set = secondaryData->newHashMap;
	
	if (oldHash != hash && set.find(hash) != set.end()) {
		ui.showErrorDlg("Can't rename sprite due to name (hash) conflict. Please pick a different name.", true);
		return false;
	}
	if (oldHash != hash) {
		secondaryData->newHashMap.erase(oldHash);
		secondaryData->newHashMap.insert(hash);
	}
	
	memcpy(sortedSprite->newName, newSpriteName, 32);
	DWORD oldIndex = sortedSprite - secondaryData->sortedSprites.data();
	DWORD newInsertionIndex = secondaryData->findSortedSpriteInsertionIndex(newSpriteName);
	if (newInsertionIndex == oldIndex || newInsertionIndex == oldIndex + 1) {
		return true;
	}
	SortedSprite backup = *sortedSprite;  // I guess this is where std::vector (newJonbin and layers used to be vectors) went haywire and caused a crash on DLL_PROCESS_DETACH
	if (newInsertionIndex > oldIndex + 1) {
		DWORD elementsToMove = newInsertionIndex - oldIndex - 1;
		memmove(sortedSprite, sortedSprite + 1, elementsToMove * sizeof (SortedSprite));
		auto itEnd = secondaryData->oldNameToSortedSpriteIndex.end();
		for (auto it = secondaryData->oldNameToSortedSpriteIndex.begin(); it != itEnd; ++it) {
			if (it->second > oldIndex && it->second < newInsertionIndex) {
				--it->second;
			}
		}
		--newInsertionIndex;
	} else if (newInsertionIndex < oldIndex) {
		DWORD elementsToMove = oldIndex - newInsertionIndex;
		memmove(sortedSprite - elementsToMove + 1, sortedSprite - elementsToMove, elementsToMove * sizeof (SortedSprite));
		auto itEnd = secondaryData->oldNameToSortedSpriteIndex.end();
		for (auto it = secondaryData->oldNameToSortedSpriteIndex.begin(); it != itEnd; ++it) {
			if (it->second >= newInsertionIndex && it->second < oldIndex) {
				++it->second;
			}
		}
	}
	secondaryData->sortedSprites[newInsertionIndex] = backup;
	sortedSprite = &secondaryData->sortedSprites[newInsertionIndex];
	secondaryData->oldNameToSortedSpriteIndex[Entity::hashStringLowercase(sortedSprite->name)] = newInsertionIndex;
	
	return true;
}

void SetAnimOperation::fill(FName newAnimSeqName, int frame) {
	Entity editEntity{gifMode.editHitboxesEntity};
	fillFromEnt(editEntity, UNDO_OPERATION_TYPE_SET_ANIM);
	this->newAnimSeqName = newAnimSeqName;
	this->frame = frame;
	determineFrameFromEntity(editEntity, &oldAnimSeqName, &oldFrame, &oldMaxFrame);
	maxFrame = 0;
	REDPawn* pawnWorld = editEntity.pawnWorld();
	if (pawnWorld) {
		maxFrame = pawnWorld->getMaxFrameOfAnimSequence(newAnimSeqName);
	}
}

SetAnimOperation::SetAnimOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

SetAnimOperation::SetAnimOperation() : UndoOperationBase() { }

bool SetAnimOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	
	if (!sortedSprite || !jonbin) {
		ui.showErrorDlg("Can't change anim sequence or frame on a 'null' sprite.", true);
		return false;
	}
	
	HitboxHolder hitboxesLocal;
	hitboxesLocal.parse(jonbin);
	
	if (!newAnimSeqName.low && !newAnimSeqName.high) {
		ui.showErrorDlg("Can't set anim sequence or frame to nothing.", true);
		return false;
	}
	
	if (!hitboxesLocal.nameCount && (animSeqName.low || animSeqName.high)) {
		bool hadNewJonbin = sortedSprite->newJonbin;
		DWORD oldJonbinSize;
		if (fpac->size0x50()) {
			FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
			oldJonbinSize = lookupEntry->size;
			lookupEntry->size += 32;
		} else {
			FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
			oldJonbinSize = lookupEntry->size;
			lookupEntry->size += 32;
		}
		BYTE* oldJonbin = jonbin;
		Hitbox* oldHitboxesStart = hitboxesLocal.hitboxesStart();
		DWORD newJonbinSize = oldJonbinSize + 32;
		sortedSprite->resizeJonbin(secondaryData, newJonbinSize);
		BYTE* newJonbin = sortedSprite->newJonbin;
		if (!hadNewJonbin) {
			memcpy(newJonbin + 4  // JONB
				+ 2  // name count
				+ 32,  // name
				oldJonbin,
				oldJonbinSize - 4  // JONB
					 - 2);  // name count
			memcpy(newJonbin, "JONB", 4);
			*(short*)(newJonbin + 4) = 1;  // name count
		} else {
			memmove(newJonbin + 4  // JONB
				+ 2  // name count
				+ 32,
				newJonbin + 4  // JONB
				+ 2,  // name count
				oldJonbinSize - 4 - 2);
		}
		secondaryData->relocateEntityJonbins(oldJonbin, oldJonbin + 4  // JONB
			+ 2,  // name count
			newJonbin);
		secondaryData->relocateEntityJonbins(oldJonbin + 4  // JONB
			+ 2,  // name count
			oldJonbin + oldJonbinSize,
			newJonbin + 4 + 2 + 32);
		Hitbox* newHitboxesStart = (Hitbox*)(
			((BYTE*)oldHitboxesStart - oldJonbin) + 32 + newJonbin
		);
		if (sortedSprite->layers) {
			EditedHitbox* layer = sortedSprite->layers;
			for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
				layer->ptr = (Hitbox*)(
					(layer->ptr - oldHitboxesStart)
					+ newHitboxesStart
				);
				++layer;
			}
		}
		if (!hadNewJonbin) {
			secondaryData->jonbinToLookupUE3.erase(oldJonbin);
			DWORD newJonbinOffset = newJonbin - ((BYTE*)fpac + fpac->headerSize);
			if (fpac->size0x50()) {
				FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
				lookupEntry->offset = newJonbinOffset;
			} else {
				FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
				lookupEntry->offset = newJonbinOffset;
			}
		}
		// we called relocateEntityJonbins before and that relocated the data, jonbinPtr, ptrRawAfterShort1, names (although it was empty at that time)
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			HitboxHolder* entHitboxes = ent.hitboxes();
			if (entHitboxes->jonbinPtr == newJonbin) {
				++entHitboxes->nameCount;
				entHitboxes->names[0] = (char*)(
					newJonbin + 4  // JONB
					+ 2  // name count
				);
				// hitboxes()->data got relocated by relocateEntityJonbins, same with hitboxes()->jonbinPtr
			}
		}
		hitboxesLocal.parse(newJonbin);
	} else if (hitboxesLocal.nameCount && !newAnimSeqName.low && !newAnimSeqName.high) {
		DWORD oldJonbinSize;
		if (fpac->size0x50()) {
			FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
			oldJonbinSize = lookupEntry->size;
			lookupEntry->size -= 32 * hitboxesLocal.nameCount;
		} else {
			FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
			oldJonbinSize = lookupEntry->size;
			lookupEntry->size -= 32 * hitboxesLocal.nameCount;
		}
		DWORD newJonbinSize = oldJonbinSize - 32 * hitboxesLocal.nameCount;
		if (sortedSprite->newJonbin) {
			sortedSprite->resizeJonbin(secondaryData, newJonbinSize);
		}
		memmove(jonbin + 4  // JONB
			+ 2,  // name count
			jonbin + 4  // JONB
			+ 2  // name count
			+ 32 * hitboxesLocal.nameCount,  // names
			oldJonbinSize - 4 - 2 - 32 * hitboxesLocal.nameCount);
		*(short*)(jonbin + 4) = 0;
		secondaryData->relocateEntityJonbins(jonbin + 4  // JONB
			+ 2  // name count
			+ 32 * hitboxesLocal.nameCount,  // names
			jonbin + oldJonbinSize,
			jonbin + 4 + 2);
		if (sortedSprite->layers) {
			EditedHitbox* layer = sortedSprite->layers;
			for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
				layer->ptr = (Hitbox*)(
					(BYTE*)layer->ptr - 32 * hitboxesLocal.nameCount
				);
				++layer;
			}
		}
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			HitboxHolder* entHitboxes = ent.hitboxes();
			if (entHitboxes->jonbinPtr == jonbin) {
				entHitboxes->nameCount = 0;
			}
		}
		hitboxesLocal.parse(jonbin);
	}
	
	if (oppositeOperation) {
		ThreadUnsafeSharedPtr<SetAnimOperation> newOp = new ThreadUnsafeSharedResource<SetAnimOperation>(*(UndoOperationBase*)this);
		*oppositeOperation = newOp;
		newOp->newAnimSeqName = oldAnimSeqName;
		newOp->frame = oldFrame;
		newOp->maxFrame = oldMaxFrame;
		newOp->oldAnimSeqName = newAnimSeqName;
		newOp->oldFrame = frame;
		newOp->oldMaxFrame = maxFrame;
		
		newOp->animSeqName = newAnimSeqName;
		newOp->currentFrame = frame;
	}

	char* imageName = hitboxesLocal.names[0];
	char fnamebuf[32];
	newAnimSeqName.print(fnamebuf);
	int printedChars = sprintf_s(imageName, 32, "%s_", fnamebuf);
	if (printedChars != -1) {
		int remainingSize = 32 - printedChars;
		imageName += printedChars;
		extern int numDigits(int num);
		int numDigs = numDigits(maxFrame);
		char printedDigits[12];
		int numberLength = sprintf_s(printedDigits, "%d", frame);
		int zerosToInsertOnTheLeft;
		if (numberLength == -1) {
			zerosToInsertOnTheLeft = 0;
		} else {
			zerosToInsertOnTheLeft = numDigs - numberLength;
		}
		if (
			zerosToInsertOnTheLeft > 0 && remainingSize >= zerosToInsertOnTheLeft + 1  // include null character
		) {
			for (int i = 0; i < zerosToInsertOnTheLeft; ++i) {
				*imageName = '0';
				++imageName;
			}
			remainingSize -= zerosToInsertOnTheLeft;
		}
		if (
			numberLength != -1 && remainingSize >= numberLength + 1  // add null character
		) {
			memcpy(imageName, printedDigits, numberLength);
			imageName += numberLength;
			remainingSize -= numberLength;
		}
		printedChars = sprintf_s(imageName, remainingSize, ".png");
		if (printedChars != -1) {
			imageName += printedChars;
			remainingSize -= printedChars;
		}
		if (remainingSize > 0) {
			memset(imageName, 0, remainingSize);
		}
	}
	BYTE* jonbin = hitboxesLocal.jonbinPtr;
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];
		if (ent.hitboxes()->jonbinPtr == jonbin) {
			endScene.spriteImpl((void*)ent.ent, spriteName, 1);
			game.allowTickForActor(ent.pawnWorld());
		}
	}
	return true;
}

void SetAnimOperation::determineFrameFromEntity(Entity ent, FName* animSeqName, int* currentFrame, int* maxFrame) {
	REDPawn* pawnWorld = ent.pawnWorld();
	if (pawnWorld) {
		REDAnimNodeSequence* animSeq = pawnWorld->getFirstAnimSeq();
		if (animSeq) {
			*animSeqName = animSeq->AnimSeqName;
			*currentFrame = animSeq->CurrentFrame;
			*maxFrame = animSeq->FrameMax;
			return;
		}
	}
	animSeqName->low = 0;
	animSeqName->high = 0;
	*maxFrame = 0;
	*currentFrame = 0;
}

bool SetAnimOperation::combine(const UndoOperationBase& other) {
	if (combineOk(other, UNDO_OPERATION_TYPE_SET_ANIM) && other.perform(nullptr)) {
		const SetAnimOperation* otherCast = (const SetAnimOperation*)&other;
		oldAnimSeqName = otherCast->newAnimSeqName;
		oldFrame = otherCast->frame;
		oldMaxFrame = otherCast->maxFrame;
		engineTick = other.engineTick;
		return true;
	}
	return false;
}

bool UndoOperationBase::combineOk(const UndoOperationBase& other, UndoOperationType requiredType) const {
	return other.type == requiredType
			&& other.backupEntity == backupEntity
			&& other.bbscrIndexInAswEng == bbscrIndexInAswEng
			&& strcmp(other.spriteName, spriteName) == 0
			&& abs((long)other.engineTick - (long)engineTick) < 20;
}

ReorderLayersOperation::ReorderLayersOperation() : UndoOperationBase() { }

ReorderLayersOperation::ReorderLayersOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

void ReorderLayersOperation::fill(int destinationIndex) {
	fillFromEnt(gifMode.editHitboxesEntity, UNDO_OPERATION_TYPE_REORDER_LAYERS);
	this->selectedHitboxes = selectedHitboxes;
	this->destinationIndex = destinationIndex;
	
	refresh();
	if (!jonbin || !sortedSprite) return;
	HitboxHolder hitboxes;
	hitboxes.parse(jonbin);
	int hitboxCount = hitboxes.hitboxCount() + 1  // always add the pushbox
	;
	oldLayers.resize(hitboxCount);
	EditedHitbox* layer = oldLayers.data();
	if (!sortedSprite->layers) {
		LayerIterator layerIterator(jonbin);
		while (layerIterator.getNext()) {
			layerIterator.copyTo(layer);
			++layer;
		}
	} else {
		memcpy(layer, sortedSprite->layers, hitboxCount * sizeof (EditedHitbox));
	}
	
	int hitboxCountWithoutPushbox = hitboxCount - 1;
	if (hitboxCountWithoutPushbox) {
		oldHitboxes.resize(hitboxCountWithoutPushbox);
		memcpy(oldHitboxes.data(), hitboxes.hitboxesStart(), hitboxCountWithoutPushbox * sizeof (Hitbox));
	}
}

bool ReorderLayersOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	ui.onNewSprite();
	
	refresh();
	
	if (!jonbin || !sortedSprite) {
		ui.showErrorDlg("Can't reorder layers on a 'null' sprite.", true);
		return false;
	}
	
	HitboxHolder hitboxesLocal;
	hitboxesLocal.parse(jonbin);
	
	int hitboxCountWithoutPushbox = hitboxesLocal.hitboxCount();
	int hitboxCount = hitboxCountWithoutPushbox + 1  // always include pushbox
	;
	
	std::vector<int> selectedHitboxesNoDuplicates;
	selectedHitboxesNoDuplicates.reserve(selectedHitboxes.size());
	
	for (int elem : selectedHitboxes) {
		bool found = false;
		for (int existingElem : selectedHitboxesNoDuplicates) {
			if (existingElem == elem) {
				found = true;
				break;
			}
		}
		if (!found) {
			selectedHitboxesNoDuplicates.push_back(elem);
		}
	}
	
	if (!sortedSprite->layers) {
		sortedSprite->resizeLayers(hitboxCount);
		EditedHitbox* layer = sortedSprite->layers;
		LayerIterator layerIterator(jonbin);
		while (layerIterator.getNext()) {
			layerIterator.copyTo(layer);
			++layer;
		}
	}
		
	EditedHitbox* destLayer = sortedSprite->layers + sortedSprite->layersSize - 1;
	EditedHitbox* gapStart = sortedSprite->layers + selectedHitboxesNoDuplicates.size() - 1;
	bool includedGap = false;
	bool selectedFilter = false;
	
	for (int iteration = 0; iteration < 2; ++iteration) {
		if (iteration == 1) {
			destLayer = gapStart;
			selectedFilter = true;
		}
		
		const EditedHitbox* src = oldLayers.data() + hitboxCount - 1;
		int layerIndReverse = 0;
		for (int layerInd = hitboxCount - 1; layerInd >= 0; --layerInd) {
			if (layerIndReverse >= destinationIndex && !includedGap) {
				includedGap = true;
				gapStart = destLayer;
				destLayer -= selectedHitboxesNoDuplicates.size();
			}
			if (
				(
					find(selectedHitboxesNoDuplicates, src->originalIndex) != -1
				) == selectedFilter
			) {
				*destLayer = *src;
				--destLayer;
			}
			--src;
			++layerIndReverse;
		}
	}
	
	Hitbox* hitboxesStart = hitboxesLocal.hitboxesStart();
	
	Hitbox* dest = hitboxesStart;
	for (int hitboxType = 0; hitboxType < 17; ++hitboxType) {
		EditedHitbox* layer = sortedSprite->layers;
		for (int layerInd = 0; layerInd < (int)sortedSprite->layersSize; ++layerInd) {
			if (!layer->isPushbox && layer->type == hitboxType) {
				*dest = oldHitboxes[layer->ptr - hitboxesStart];
				layer->ptr = dest;
				++dest;
			}
			++layer;
		}
	}
	
	ThreadUnsafeSharedPtr<UndoReorderLayersOperation> newOp = new ThreadUnsafeSharedResource<UndoReorderLayersOperation>(*(UndoOperationBase*)this);
	*oppositeOperation = newOp;
	newOp->newLayers = oldLayers;
	newOp->newHitboxes = oldHitboxes;
	newOp->oldLayers.resize(sortedSprite->layersSize);
	memcpy(newOp->oldLayers.data(), sortedSprite->layers, sortedSprite->layersSize * sizeof (EditedHitbox));
	if (hitboxCountWithoutPushbox) {
		newOp->oldHitboxes.resize(hitboxCountWithoutPushbox);
		memcpy(newOp->oldHitboxes.data(), hitboxesStart, hitboxCountWithoutPushbox * sizeof (Hitbox));
	}
	
	return true;
}

UndoReorderLayersOperation::UndoReorderLayersOperation() : UndoOperationBase() { }

UndoReorderLayersOperation::UndoReorderLayersOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

bool UndoReorderLayersOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	
	refresh();
	if (!jonbin || !sortedSprite) {
		ui.showErrorDlg("Can't undo reordering of layers on a 'null' sprite.", true);
		return false;
	}
		
	ThreadUnsafeSharedPtr<UndoReorderLayersOperation> newOp = new ThreadUnsafeSharedResource<UndoReorderLayersOperation>(*(UndoOperationBase*)this);
	*oppositeOperation = newOp;
	newOp->newLayers = oldLayers;
	newOp->newHitboxes = oldHitboxes;
	newOp->oldLayers = newLayers;
	newOp->oldHitboxes = newHitboxes;
	
	HitboxHolder hitboxes;
	hitboxes.parse(jonbin);
	if (!newHitboxes.empty()) {
		memcpy(hitboxes.hitboxesStart(), newHitboxes.data(), newHitboxes.size() * sizeof (Hitbox));
	}
	if (!newLayers.empty()) {
		memcpy(sortedSprite->layers, newLayers.data(), newLayers.size() * sizeof (EditedHitbox));
	}
	return true;
}

AddBoxOperation::AddBoxOperation() : UndoOperationBase() { }

AddBoxOperation::AddBoxOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

void AddBoxOperation::fill(HitboxType typeToAdd, DrawHitboxArrayParams& params, int left, int top, int right, int bottom) {
	fillFromEnt(gifMode.editHitboxesEntity, UNDO_OPERATION_TYPE_ADD_BOX);
	this->typeToAdd = typeToAdd;
	this->params = params;
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

bool AddBoxOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	if (!sortedSprite) {
		ui.showErrorDlg("Can't add a new hitbox to the 'null' sprite.", true);
		return false;
	}
	std::vector<int> newSelectedHitboxes = selectedHitboxes;
	DWORD oldJonbinSize;
	if (fpac->size0x50()) {
		FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
		oldJonbinSize = lookupEntry->size;
	} else {
		FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
		oldJonbinSize = lookupEntry->size;
	}
	BYTE* oldJonbin = jonbin;
	
	HitboxHolder hitboxes;
	hitboxes.parse(oldJonbin);
	
	int oldHitboxCount = hitboxes.hitboxCount();
	
	BYTE* jonbinShorts = jonbin + 4  // skip "JONB"
		+ 2  // short namesCount
		+ hitboxes.nameCount * 32;
	BYTE oldNumTypes = *jonbinShorts;
	++jonbinShorts;
	short short1 = *(short*)jonbinShorts;
	jonbinShorts += 2;
	short short2 = *(short*)jonbinShorts;
	jonbinShorts += 2;
	short short3 = *(short*)jonbinShorts;
	jonbinShorts += 2;
	
	DWORD newJonbinSize = 4  // "JONB"
		+ 2  // short namesCount
		+ hitboxes.nameCount * 32  // each name is 32 characters (including null)
		+ 1  // BYTE numTypes. We will always write 0x14
		+ 2 + 2 + 2  // short1, short2, short3. This data seems to be unused, but we want to preserve it if it existed in the sprite
		+ 17 * 2  // 17 shorts: count of hitboxes of each type
		+ short1 * FPAC::size1
		+ short2 * FPAC::size2
		+ short3 * FPAC::size3
		+ (oldHitboxCount + 1) * sizeof (Hitbox);
	
	sortedSprite->resizeJonbin(secondaryData, newJonbinSize);
	
	auto ue3entry = secondaryData->jonbinToLookupUE3.find(jonbin);
	if (ue3entry != secondaryData->jonbinToLookupUE3.end()) {
		BYTE* lookupBytePtr = (BYTE*)ue3entry->second;
		secondaryData->jonbinToLookupUE3.erase(ue3entry);
		if (fpac->size0x50()) {
			FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)lookupBytePtr;
			lookupEntry->offset = sortedSprite->newJonbin - ((BYTE*)fpac + fpac->headerSize);
		} else {
			FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)lookupBytePtr;
			lookupEntry->offset = sortedSprite->newJonbin - ((BYTE*)fpac + fpac->headerSize);
		}
		memcpy(sortedSprite->newJonbin, jonbin, oldJonbinSize);
		secondaryData->relocateEntityJonbins(jonbin, jonbin + oldJonbinSize, sortedSprite->newJonbin);
		hitboxes.parse(sortedSprite->newJonbin);
		
		EditedHitbox* layer = sortedSprite->layers;
		for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
			if (!layer->isPushbox && (BYTE*)layer->ptr >= jonbin && (BYTE*)layer->ptr < jonbin + oldJonbinSize) {
				layer->ptr = (Hitbox*)((BYTE*)layer->ptr + (sortedSprite->newJonbin - jonbin));
			}
			++layer;
		}
		
	}
	
	if (oldNumTypes < 0x14) {
		DWORD shift = (0x14 - oldNumTypes) * 2;
		BYTE* jonbinRelocateStart = sortedSprite->newJonbin + 4  // skip "JONB"
			+ 2  // short namesCount
			+ hitboxes.nameCount * 32
			+ 1
			+ oldNumTypes * 2;
		secondaryData->relocateEntityJonbins(jonbinRelocateStart, sortedSprite->newJonbin + oldJonbinSize,
			jonbinRelocateStart + shift);
		memmove(jonbinRelocateStart + shift, jonbinRelocateStart, oldJonbinSize - (jonbinRelocateStart - sortedSprite->newJonbin));
		memset(jonbinRelocateStart, 0, shift);
		oldJonbinSize += shift;
		hitboxes.parse(sortedSprite->newJonbin);
		
		EditedHitbox* layer = sortedSprite->layers;
		for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
			if (!layer->isPushbox
					&& (BYTE*)layer->ptr >= jonbinRelocateStart
					&& (BYTE*)layer->ptr < sortedSprite->newJonbin + oldJonbinSize) {
				layer->ptr = (Hitbox*)((BYTE*)layer->ptr + shift);
			}
			++layer;
		}
		
	}
	
	jonbin = sortedSprite->newJonbin + 4  // skip "JONB"
		+ 2  // short namesCount
		+ hitboxes.nameCount * 32
		+ 1  // now for sure 0x14 types
		+ 2 + 2 + 2  // short1, short2, short3, we know them already
		+ 17 * 2  // we can get hitbox counts from the 'hitboxes'
		+ short1 * FPAC::size1
		+ short2 * FPAC::size2
		+ short3 * FPAC::size3;
	
	for (int hitboxType = 0; hitboxType <= (int)typeToAdd; ++hitboxType) {
		jonbin += sizeof (Hitbox) * hitboxes.count[hitboxType];
	}
	
	if (jonbin != sortedSprite->newJonbin + oldJonbinSize) {
		secondaryData->relocateEntityJonbins(jonbin, sortedSprite->newJonbin + oldJonbinSize,
			jonbin + sizeof (Hitbox));
		memmove(jonbin + sizeof (Hitbox), jonbin, oldJonbinSize - (jonbin - sortedSprite->newJonbin));
		hitboxes.parse(sortedSprite->newJonbin);
		
		EditedHitbox* layer = sortedSprite->layers;
		for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
			if (!layer->isPushbox
					&& (BYTE*)layer->ptr >= jonbin
					&& (BYTE*)layer->ptr < sortedSprite->newJonbin + oldJonbinSize) {
				++layer->ptr;
			}
			++layer;
		}
		
	}
	
	Hitbox* newHitboxData = (Hitbox*)jonbin;
	
	DrawHitboxArrayCallParams::arenaToHitbox(params, left, top, right, bottom, newHitboxData);
	
	if (fpac->size0x50()) {
		FPACLookupElement0x50* lookupEntry = (FPACLookupElement0x50*)sortedSprite->name;
		lookupEntry->size = newJonbinSize;
	} else {
		FPACLookupElement0x30* lookupEntry = (FPACLookupElement0x30*)sortedSprite->name;
		lookupEntry->size = newJonbinSize;
	}
	
	jonbin = sortedSprite->newJonbin + 4  // skip "JONB"
		+ 2
		+ hitboxes.nameCount * 32
		+ 1
		+ 2 + 2 + 2
		+ 2 * typeToAdd;
	*(short*)jonbin = *(short*)jonbin + 1;
	
	#pragma warning(suppress:6385)
	++hitboxes.count[typeToAdd];
	
	// the Entities affected in this loop will include the editEntity
	BYTE* editEntityJonbin = hitboxes.jonbinPtr;
	for (int entityInd = 0; entityInd < entityList.count; ++entityInd) {
		Entity ent = entityList.list[entityInd];
		if (ent.hitboxes()->jonbinPtr == editEntityJonbin) {
			#pragma warning(suppress:6385)
			++ent.hitboxes()->count[typeToAdd];
		}
	}
	
	ui.onNewSprite();
	ui.clearSelectedHitboxes();
	
	if (sortedSprite->layers) {
		
		int biggestSubindex = -1;
		int biggestOriginalIndex = -1;
		EditedHitbox* layer = sortedSprite->layers;
		for (int layerIndex = 0; layerIndex < (int)sortedSprite->layersSize; ++layerIndex) {
			if (layer->type == typeToAdd && layer->subindex > biggestSubindex && !layer->isPushbox) {
				biggestSubindex = layer->subindex;
			}
			if (layer->originalIndex > biggestOriginalIndex) {
				biggestOriginalIndex = layer->originalIndex;
			}
			++layer;
		}
		
		sortedSprite->resizeLayers(sortedSprite->layersSize + 1);
		EditedHitbox* newLayer = sortedSprite->layers + sortedSprite->layersSize - 1;
		newLayer->type = typeToAdd;
		newLayer->isPushbox = false;
		newLayer->originalIndex = biggestOriginalIndex + 1;
		newLayer->subindex = biggestSubindex + 1;
		#pragma warning(suppress:6385)
		newLayer->ptr = hitboxes.data[typeToAdd] + hitboxes.count[typeToAdd] - 1;
		
		newSelectedHitboxes.push_back(newLayer->originalIndex);
	} else {
		// we're changing the originalIndex property of possibly some layers, which some of these data structures probably refer to
		// or trigger code that uses these
		ui.onAddBoxWithoutLayers(sortedSprite->newJonbin);
		
		int addedSubindex = hitboxes.count[typeToAdd] - 1;
		
		if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
			Entity editEntity{gifMode.editHitboxesEntity};
			if ((BYTE*)sortedSprite->name == editEntity.hitboxes()->ptrLookup) {
				LayerIterator layerIterator(editEntity, sortedSprite);
				while (layerIterator.getNext()) {
					if (layerIterator.type == typeToAdd && layerIterator.subindex == addedSubindex && !layerIterator.isPushbox) {
						newSelectedHitboxes.push_back(layerIterator.originalIndex);
						break;
					}
				}
			}
		}
	}
	
	if (gifMode.editHitboxes && gifMode.editHitboxesEntity) {
		Entity editEntity = gifMode.editHitboxesEntity;
		if (editEntity.hitboxes()->jonbinPtr == sortedSprite->newJonbin) {
			ui.setSelectedHitboxes(newSelectedHitboxes);
		}
	}
	
	ThreadUnsafeSharedPtr<DeleteLayersOperation> newOp = new ThreadUnsafeSharedResource<DeleteLayersOperation>(*(UndoOperationBase*)this);
	*oppositeOperation = newOp;
	newOp->boxesToDelete = newSelectedHitboxes;
	
	return true;
}

MoveResizeBoxesOperation::MoveResizeBoxesOperation() : UndoOperationBase() { }

MoveResizeBoxesOperation::MoveResizeBoxesOperation(const UndoOperationBase& other) : UndoOperationBase(other) { }

bool MoveResizeBoxesOperation::perform(ThreadUnsafeSharedPtr<UndoOperationBase>* oppositeOperation) const {
	refresh();
	
	HitboxHolder hitboxes;
	hitboxes.parse(jonbin);
	Hitbox* hitboxesStart = hitboxes.hitboxesStart();
	memcpy(hitboxesStart, newData.data(), newData.size() * sizeof (Hitbox));
	
	ThreadUnsafeSharedPtr<MoveResizeBoxesOperation> newOp = new ThreadUnsafeSharedResource<MoveResizeBoxesOperation>(*(UndoOperationBase*)this);
	newOp->oldData = newData;
	newOp->newData = oldData;
	*oppositeOperation = newOp;
	
	return true;
}

void MoveResizeBoxesOperation::update(Hitbox* data, int count) {
	memcpy(oldData.data(), data, count * sizeof (Hitbox));
}

void MoveResizeBoxesOperation::fill(std::vector<Hitbox>&& oldData, Hitbox* newData, int count) {
	fillFromEnt(gifMode.editHitboxesEntity, UNDO_OPERATION_TYPE_MOVE_RESIZE_BOXES);
	this->oldData.resize(count);
	memcpy(this->oldData.data(), newData, count * sizeof (Hitbox));
	this->newData = oldData;
}

bool MoveResizeBoxesOperation::combineOk(Entity ent, DWORD engineTick, int timerRange, UndoOperationType requiredType, int hitboxCount) const {
	return this->type == type
			&& backupEntity == ent
			&& bbscrIndexInAswEng == ent.bbscrIndexInAswEng()
			&& strcmp(spriteName, ent.spriteName()) == 0
			&& abs((long)engineTick - (long)engineTick) < timerRange
			&& newData.size() == (size_t)hitboxCount
			&& oldData.size() == (size_t)hitboxCount;
}
