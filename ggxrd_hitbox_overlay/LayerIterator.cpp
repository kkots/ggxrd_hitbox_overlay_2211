#include "pch.h"
#include "LayerIterator.h"
#include "UI.h"

LayerIterator::LayerIterator(Entity ent) {
	
	FPACSecondaryData& secondaryData = ui.hitboxEditorFPACSecondaryData[ent.bbscrIndexInAswEng()];
	DWORD hash = Entity::hashStringLowercase(ent.spriteName());
	auto found = secondaryData.oldNameToSortedSpriteIndex.find(hash);
	if (found != secondaryData.oldNameToSortedSpriteIndex.end()) {
		parent = &secondaryData.sortedSprites[found->second];
	} else {
		parent = nullptr;
	}
	
	jonbin = ent.hitboxes()->jonbinPtr;
}
// firstRun = true

LayerIterator::LayerIterator(Entity ent, SortedSprite* parent)
	: parent(parent) {
	
	FPACSecondaryData& secondaryData = ui.hitboxEditorFPACSecondaryData[ent.bbscrIndexInAswEng()];
	secondaryData.parseAllSprites();
	FPAC* fpac = secondaryData.Collision->TopData;
	
	if (parent) {
		jonbin = (BYTE*)fpac + fpac->headerSize
			+ (
				fpac->size0x50()
					? ((FPACLookupElement0x50*)parent->name)->offset
					: ((FPACLookupElement0x30*)parent->name)->offset
			);
	} else {
		jonbin = ent.hitboxes()->jonbinPtr;
	}
}
// firstRun = true

LayerIterator::LayerIterator(FPAC* fpac, SortedSprite* parent)
	: parent(parent),
	jonbin((BYTE*)fpac + fpac->headerSize
		+ (
			fpac->size0x50()
				? ((FPACLookupElement0x50*)parent->name)->offset
				: ((FPACLookupElement0x30*)parent->name)->offset
		)
	) { }
	// firstRun = true

LayerIterator::LayerIterator(BYTE* jonbin) : jonbin(jonbin), parent(nullptr) { }

bool LayerIterator::getNext() {
	if (scrolledToEnd) return false;
	if (sof) { scrollToStart(); }
	if (parent && parent->layers) {
		if (firstRun) {
			firstRun = false;
			if (!parent->layersSize) {
				layerPtr = parent->layers;
				eof = true;
				index = 0;
				return false;
			}
			layerPtr = parent->layers;
			(EditedHitbox&)*this = *layerPtr;
			index = 0;
			return true;
		} else {
			int currentLayerIndex = layerPtr - parent->layers;
			if (currentLayerIndex >= (int)parent->layersSize - 1) {
				eof = true;
				index = currentLayerIndex;
				return false;
			}
			++layerPtr;
			(EditedHitbox&)*this = *layerPtr;
			index = layerPtr - parent->layers;
			return true;
		}
	} else if (firstRun) {
		firstRun = false;
		init();
		for (int hitboxType = 0; hitboxType < (int)numTypes; ++hitboxType) {
			int count = hitboxCounts[hitboxType];
			int addPushbox = hitboxType == HITBOXTYPE_PUSHBOX;
			if (!count && !addPushbox) continue;
			isPushbox = !count;
			originalIndex = 0;
			subindex = 0;
			ptr = !count ? nullptr : hitboxesPtr;
			type = (HitboxType)hitboxType;
			index = 0;
			return true;
		}
		type = (HitboxType)numTypes;
		isPushbox = true;
		eof = true;
		index = 0;
		return false;
	} else {
		int hitboxType = type;
		if (hitboxType < (int)numTypes) {
			++originalIndex;
		}
		if (!isPushbox) {
			int count = hitboxType < (int)numTypes ? hitboxCounts[hitboxType] : 0;
			int addPushbox = hitboxType == HITBOXTYPE_PUSHBOX;
			++subindex;
			++hitboxesPtr;
			if (subindex < count + addPushbox) {
				isPushbox = hitboxType == HITBOXTYPE_PUSHBOX && subindex == count;
				ptr = isPushbox ? nullptr : hitboxesPtr;
				type = (HitboxType)hitboxType;
				index = originalIndex;
				return true;
			}
		}
		for (++hitboxType; hitboxType < (int)numTypes; ++hitboxType) {
			int count = hitboxCounts[hitboxType];
			int addPushbox = hitboxType == HITBOXTYPE_PUSHBOX;
			if (!count && !addPushbox) continue;
			isPushbox = !count;
			subindex = 0;
			ptr = !count ? nullptr : hitboxesPtr;
			type = (HitboxType)hitboxType;
			index = originalIndex;
			return true;
		}
		type = (HitboxType)numTypes;
		isPushbox = true;
		eof = true;
		index = originalIndex;
		return false;
	}
}

bool LayerIterator::getPrev() {
	if (firstRun && !scrolledToEnd) return false;
	if (eof) { scrollToEnd(); }
	scrolledToEnd = false;
	if (parent && parent->layers) {
		if (firstRun) {
			firstRun = false;
			if (parent->layersSize) {
				index = (int)parent->layersSize - 1;
				layerPtr = parent->layers + index;
				(EditedHitbox&)*this = *layerPtr;
				return true;
			} else {
				layerPtr = parent->layers;
				sof = true;
				index = -1;
				return false;
			}
		} else {
			if (layerPtr == parent->layers) {
				sof = true;
				index = -1;
				return false;
			}
			--layerPtr;
			(EditedHitbox&)*this = *layerPtr;
			index = layerPtr - parent->layers;
			return true;
		}
	} else if (firstRun) {
		firstRun = false;
		init();
		int originalIndexReverse = 0;
		for (int hitboxType = 0; hitboxType < (int)numTypes; ++hitboxType) {
			originalIndexReverse += (int)hitboxCounts[hitboxType];
		}
		hitboxesPtr += originalIndexReverse;
		originalIndexReverse += 1;  // add pushbox
		--originalIndexReverse;
		
		for (int hitboxType = (int)numTypes - 1; hitboxType >= 0; --hitboxType) {
			int count = hitboxCounts[hitboxType];
			int addPushbox = hitboxType == HITBOXTYPE_PUSHBOX;
			if (!count && !addPushbox) continue;
			isPushbox = addPushbox;
			originalIndex = originalIndexReverse;
			subindex = count + addPushbox - 1;
			if (!addPushbox) {
				--hitboxesPtr;
				ptr = hitboxesPtr;
			} else {
				ptr = nullptr;
			}
			type = (HitboxType)hitboxType;
			index = originalIndexReverse;
			return true;
		}
		type = (HitboxType)numTypes;
		isPushbox = true;
		sof = true;
		index = -1;
		return false;
	} else {
		if (isPushbox) {
			if ((int)type >= (int)numTypes) {
				sof = true;
				index = -1;
				return false;
			}
			if (hitboxCounts[type]) {
				isPushbox = false;
				--originalIndex;
				subindex = hitboxCounts[type] - 1;
				--hitboxesPtr;
				ptr = hitboxesPtr;
				prevDirection = -1;
				index = originalIndex;
				return true;
			}
		} else if (subindex) {
			isPushbox = false;
			--originalIndex;
			--subindex;
			--hitboxesPtr;
			ptr = hitboxesPtr;
			prevDirection = -1;
			index = originalIndex;
			return true;
		}
		for (int hitboxType = (int)type - 1; hitboxType >= 0; --hitboxType) {
			int count = hitboxCounts[hitboxType];
			int addPushbox = hitboxType == HITBOXTYPE_PUSHBOX;
			if (addPushbox) {
				type = HITBOXTYPE_PUSHBOX;
				isPushbox = true;
				--originalIndex;
				ptr = nullptr;
				prevDirection = -1;
				subindex = count;
				index = originalIndex;
				return true;
			}
			if (!count) continue;
			type = (HitboxType)hitboxType;
			--originalIndex;
			subindex = count - 1;
			--hitboxesPtr;
			ptr = hitboxesPtr;
			isPushbox = false;
			prevDirection = -1;
			index = originalIndex;
			return true;
		}
		isPushbox = true;
		type = (HitboxType)numTypes;
		sof = true;
		index = -1;
		return false;
	}
}

void LayerIterator::reset() {
	firstRun = true;
	eof = false;
	sof = false;
	prevDirection = 0;
}

int LayerIterator::count() {
	if (parent && parent->layers) {
		return (int)parent->layersSize;
	}
	if (!hitboxCounts) {
		init();
	}
	int result = 0;
	for (BYTE type = 0; type < numTypes; ++type) {
		result += hitboxCounts[type];
	}
	return result + 1  // add pushbox
	;
}

void LayerIterator::init() {
	if (!jonbin) {
		numTypes = 0;
	} else {
		numTypes = HitboxHolder::numTypes(jonbin);
		hitboxCounts = HitboxHolder::hitboxCounts(jonbin);
		hitboxesPtr = HitboxHolder::hitboxesStart(jonbin);
	}
}

void LayerIterator::copyTo(EditedHitbox* dest) const {
	const LayerIterator* src = this;
	const EditedHitbox* srcCast = (const EditedHitbox*)src;
	// Microsoft:
	/* The language allows slicing and can be viewed as a special case of a dangerous implicit cast.
	   Even if it's done intentionally and doesn't lead to immediate issues, it's still highly discouraged.
	   It makes code harder to change, by forcing extra requirements on related data types.
	   It's especially true if types are polymorphic or involve resource management. */
	// Counterargument:
	/* struct EditedHitbox is meant to just hold fields. Chill bro, no one
	   is taking your OOP paradigms away. You can just use them elsewhere. */
	#pragma warning(suppress:26437)
	*dest = *src;
}
