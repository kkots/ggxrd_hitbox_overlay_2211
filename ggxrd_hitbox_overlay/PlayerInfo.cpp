#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "PlayerInfo.h"
#include "findMoveByName.h"
#include "EndScene.h"
#include "EntityList.h"
#include "Settings.h"
#include "NamePairManager.h"

#define advanceBuf if (result != -1) { buf += result; bufSize -= result; }

void ActiveDataArray::addActive(int hitNum, int n, bool forceNewHit) {
	if (count == 0) {
		prevHitNum = hitNum;
		data[0].actives = n;
		data[0].nonActives = 0;
		count = 1;
		return;
	}
	if (merging) {
		ActiveData& elem = data[count - 1];
		if (elem.nonActives == 1) {
			if (hitNum == prevHitNum || hitNumConflict) {
				elem.actives += n;
				elem.nonActives = 0;
			} else if (hitNum > prevHitNum && !hitNumConflict) {
				prevHitNum = hitNum;
				elem.nonActives = 0;
				if (count >= _countof(data)) {
					memmove(data, data + 1, sizeof data - sizeof *data);
					// the dumb version of ring buffer
					--count;
				}
				ActiveData& newElem = data[count];
				newElem.actives = n;
				newElem.nonActives = 0;
				++count;
			} else if (!hitNumConflict) {
				hitNumConflict = true;
				elem.actives += n;
				elem.nonActives = 0;
				removeSeparateHits(nullptr);
			}
		} else if (elem.nonActives > 1) {
			prevHitNum = hitNum;
			if (count >= _countof(data)) {
				memmove(data, data + 1, sizeof data - sizeof *data);
				// the dumb version of ring buffer
				--count;
			}
			--elem.nonActives;
			ActiveData& newElem = data[count];
			newElem.actives = n;
			newElem.nonActives = 0;
			++count;
		} else if (hitNum != prevHitNum) {
			hitNumConflict = true;
			removeSeparateHits(nullptr);
		}
		return;
	}
	if (data[count - 1].nonActives || forceNewHit || hitNum > prevHitNum) {
		if (count >= _countof(data)) {
			memmove(data, data + 1, sizeof data - sizeof *data);
			--count;
			// the dumb version of ring buffer
		}
		ActiveData& elem = data[count];
		elem.actives = n;
		elem.nonActives = 0;
		++count;
	} else {
		data[count - 1].actives += n;
	}
	prevHitNum = hitNum;
}

void ActiveDataArray::addSuperfreezeActive(int hitNum) {
	if (count == 0) {
		// fix for Venom Red Hail
		data[0].nonActives = 0;
		data[0].actives = 1;
		prevHitNum = hitNum;
		++count;
		return;
	}
	ActiveData& elem = data[count - 1];
	if (elem.nonActives == 1) {
		elem.nonActives = 0;
		if (hitNum == prevHitNum || hitNumConflict) {
			++elem.actives;
		} else {
			if (count >= _countof(data)) {
				memmove(data, data + 1, sizeof data - sizeof *data);
				--count;
				// the dumb version of ring buffer
			}
			ActiveData& newElem = data[count];
			newElem.actives = 1;
			newElem.nonActives = 0;
			++count;
		}
	} else if (elem.nonActives > 1) {
		--elem.nonActives;
		if (count >= _countof(data)) {
			memmove(data, data + 1, sizeof data - sizeof *data);
			--count;
			// the dumb version of ring buffer
		}
		ActiveData& newElem = data[count];
		newElem.actives = 1;
		newElem.nonActives = 0;
		++count;
	} else if (hitNum == prevHitNum || hitNumConflict) {
		// do nothing
	} else if (elem.actives > 1) {
		--elem.actives;
		if (count >= _countof(data)) {
			memmove(data, data + 1, sizeof data - sizeof *data);
			--count;
			// the dumb version of ring buffer
		}
		ActiveData& newElem = data[count];
		newElem.actives = 1;
		newElem.nonActives = 0;
		++count;
	}
	prevHitNum = hitNum;
}

void ActiveDataArray::findFrame(int frame, int* outIndex, int* outFrame) const {
	*outIndex = 0;
	*outFrame = 0;
	while (frame > 0 && *outIndex < count) {
		const ActiveData& elem = data[*outIndex];
		if (frame < elem.actives + elem.nonActives) {
			*outFrame = frame;
			return;
		} else {
			frame -= elem.actives + elem.nonActives;
			++*outIndex;
		}
	}
}

int ActiveDataArray::total() const {
	int total = 0;
	int prevNonActives = 0;
	for (int i = 0; i < count; ++i) {
		total += prevNonActives + data[i].actives;
		prevNonActives = data[i].nonActives;
	}
	return total;
}

bool ActiveDataArray::checkHitNumConflict(int startup, const ActiveDataArray& other) {
	--startup;
	if (!count || !other.count) return false;
	int index = 0;
	int otherIndex = 0;
	int frame = 0;
	findFrame(startup, &index, &frame);
	if (index >= count) return false;
	ActiveData remainder;
	bool remainderStartsOnANewHit;
	ActiveData& foundElem = data[index];
	if (frame == 0) {
		remainderStartsOnANewHit = true;
		remainder = foundElem;
	} else if (frame < foundElem.actives) {
		remainderStartsOnANewHit = false;
		remainder.actives = foundElem.actives - frame;
		remainder.nonActives = foundElem.nonActives;
	} else {
		remainderStartsOnANewHit = false;
		remainder.actives = 0;
		remainder.nonActives = foundElem.nonActives + foundElem.actives - frame;
	}
	ActiveData otherRemainder = other.data[0];
	bool otherRemainderStartsOnANewHit = true;
	for (; index < count && otherIndex < other.count; ) {
		if (otherRemainder.actives && remainder.actives && otherRemainderStartsOnANewHit != remainderStartsOnANewHit) {
			return true;
		}
		remainderStartsOnANewHit = false;
		otherRemainderStartsOnANewHit = false;
		if (remainder.actives == otherRemainder.actives) {
			remainder.actives = 0;
			otherRemainder.actives = 0;
			if (remainder.nonActives == otherRemainder.nonActives) {
				remainder.nonActives = 0;
				otherRemainder.nonActives = 0;
			} else if (remainder.nonActives < otherRemainder.nonActives) {
				otherRemainder.nonActives = otherRemainder.nonActives - remainder.nonActives;
				remainder.nonActives = 0;
			} else {
				remainder.nonActives = remainder.nonActives - otherRemainder.nonActives;
				otherRemainder.nonActives = 0;
			}
		} else if (remainder.actives < otherRemainder.actives) {
			if (remainder.actives == 0) {
				if (otherRemainder.actives <= remainder.nonActives) {
					remainder.nonActives -= otherRemainder.actives;
					otherRemainder.actives = 0;
				} else {
					otherRemainder.actives -= remainder.nonActives;
					remainder.nonActives = 0;
				}
			} else {
				otherRemainder.actives = otherRemainder.actives - remainder.actives;
				remainder.actives = 0;
			}
		} else {
			if (otherRemainder.actives == 0) {
				if (remainder.actives <= otherRemainder.nonActives) {
					otherRemainder.nonActives -= remainder.actives;
					remainder.actives = 0;
				} else {
					remainder.actives -= otherRemainder.nonActives;
					otherRemainder.nonActives = 0;
				}
			} else {
				remainder.actives = remainder.actives - otherRemainder.actives;
				otherRemainder.actives = 0;
			}
		}
		if (remainder.actives + remainder.nonActives == 0) {
			++index;
			if (index < count) {
				remainder = data[index];
				remainderStartsOnANewHit = true;
			}
		}
		if (otherRemainder.actives + otherRemainder.nonActives == 0) {
			++otherIndex;
			if (otherIndex < other.count) {
				otherRemainder = other.data[otherIndex];
				otherRemainderStartsOnANewHit = true;
			}
		}
	}
	return false;
}

void ActiveDataArray::mergeTimeline(int startup, const ActiveDataArray& other) {
	if (!count && !other.count) return;
	bool hasConflict = true;
	if (!hitNumConflict && !other.hitNumConflict) {
		hasConflict = checkHitNumConflict(startup, other);
	}
	--startup;
	ActiveDataArray destination = other;
	if (hasConflict) {
		if (!other.hitNumConflict) {
			destination.removeSeparateHits(nullptr);
		}
		if (!hitNumConflict) {
			removeSeparateHits(nullptr);
		}
		hitNumConflict = true;
	}
	ActiveDataArray result;
	int index = 0;
	int otherIndex = 0;
	int frame = 0;
	findFrame(startup, &index, &frame);
	result.count = index;
	int resultTotalWithNonActive = 0;
	for (int i = 0; i < index; ++i) {
		ActiveData& elem = data[i];
		result.data[i] = elem;
		resultTotalWithNonActive += elem.actives + elem.nonActives;
	}
	if (index < count && frame > 0) {
		++result.count;
		ActiveData& elem = data[index];
		ActiveData& dest = result.data[index];
		if (frame <= elem.actives) {
			dest.actives = frame;
			dest.nonActives = 0;
		} else {
			dest.actives = elem.actives;
			dest.nonActives = frame - elem.actives;
		}
	} else if (index >= count && startup - resultTotalWithNonActive > 0) {
		result.addNonActive(startup - resultTotalWithNonActive);
	}
	ActiveData remainder;
	bool remainderStartsOnANewHit;
	if (index < count) {
		ActiveData& foundElem = data[index];
		if (frame == 0) {
			remainderStartsOnANewHit = true;
			remainder = foundElem;
		} else if (frame < foundElem.actives) {
			remainderStartsOnANewHit = false;
			remainder.actives = foundElem.actives - frame;
			remainder.nonActives = foundElem.nonActives;
		} else {
			remainderStartsOnANewHit = false;
			remainder.actives = 0;
			remainder.nonActives = foundElem.nonActives + foundElem.actives - frame;
		}
	} else {
		remainder.actives = 0;
		remainder.nonActives = 0;
		remainderStartsOnANewHit = false;
	}
	ActiveData otherRemainder = destination.data[0];
	bool otherRemainderStartsOnANewHit = true;
	int hitNumber = -1;
	result.prevHitNum = hitNumber;
	for (; index < count || otherIndex < destination.count; ) {
		if (!hitNumConflict && (index < count && remainder.actives && remainderStartsOnANewHit
				|| otherIndex < other.count && otherRemainder.actives && otherRemainderStartsOnANewHit)) {
			++hitNumber;
		}
		remainderStartsOnANewHit = false;
		otherRemainderStartsOnANewHit = false;
		int activesToAdd = 0;
		int nonActivesToAdd = 0;
		if (index < count && otherIndex < destination.count) {
			if (remainder.actives == otherRemainder.actives) {
				activesToAdd = remainder.actives;
				remainder.actives = 0;
				otherRemainder.actives = 0;
				if (remainder.nonActives < otherRemainder.nonActives) {
					nonActivesToAdd = remainder.nonActives;
					otherRemainder.nonActives = otherRemainder.nonActives - remainder.nonActives;
					remainder.nonActives = 0;
				} else {
					nonActivesToAdd = otherRemainder.nonActives;
					remainder.nonActives = remainder.nonActives - otherRemainder.nonActives;
					otherRemainder.nonActives = 0;
				}
			} else if (remainder.actives < otherRemainder.actives) {
				if (remainder.actives == 0) {
					if (otherRemainder.actives <= remainder.nonActives) {
						activesToAdd = otherRemainder.actives;
						remainder.nonActives -= otherRemainder.actives;
						otherRemainder.actives = 0;
					} else {
						activesToAdd = remainder.nonActives;
						otherRemainder.actives -= remainder.nonActives;
						remainder.nonActives = 0;
					}
				} else {
					activesToAdd = remainder.actives;
					otherRemainder.actives = otherRemainder.actives - remainder.actives;
					remainder.actives = 0;
				}
			} else {
				if (otherRemainder.actives == 0) {
					if (remainder.actives <= otherRemainder.nonActives) {
						activesToAdd = remainder.actives;
						otherRemainder.nonActives -= remainder.actives;
						remainder.actives = 0;
					} else {
						activesToAdd = otherRemainder.nonActives;
						remainder.actives -= otherRemainder.nonActives;
						otherRemainder.nonActives = 0;
					}
				} else {
					remainder.actives = remainder.actives - otherRemainder.actives;
					otherRemainder.actives = 0;
				}
			}
		} else if (index < count) {
			activesToAdd = remainder.actives;
			nonActivesToAdd = remainder.nonActives;
			remainder.actives = 0;
			remainder.nonActives = 0;
		} else {
			activesToAdd = otherRemainder.actives;
			nonActivesToAdd = otherRemainder.nonActives;
			otherRemainder.actives = 0;
			otherRemainder.nonActives = 0;
		}
		if (activesToAdd) {
			result.addActive(hitNumber, activesToAdd);
		}
		if (nonActivesToAdd) {
			result.addNonActive(nonActivesToAdd);
		}
		if (index < count && remainder.actives + remainder.nonActives == 0) {
			++index;
			if (index < count) {
				remainder = data[index];
				remainderStartsOnANewHit = true;
			}
		}
		if (otherIndex < destination.count && otherRemainder.actives + otherRemainder.nonActives == 0) {
			++otherIndex;
			if (otherIndex < destination.count) {
				otherRemainder = destination.data[otherIndex];
				otherRemainderStartsOnANewHit = true;
			}
		}
	}
	*this = result;
}

int ActiveDataArray::print(char* buf, size_t bufSize) const {
	size_t origBufSize = bufSize;
	char* origBuf = buf;
	int result;
	if (count == 0) {
		result = sprintf_s(buf, bufSize, "0");
		if (result == -1) return 0;
		return 1;
	}
	int maxConsecutiveHits = 0;
	int currentConsecutiveHits = 0;
	int lastActives = 0;
	int lastNonActives = 0;
	for (int i = 0; i < count && bufSize; ++i) {
		if (lastNonActives) {
			currentConsecutiveHits = 1;
			result = sprintf_s(buf, bufSize, "(%d)", lastNonActives);
			if (result != -1) {
				if ((int)bufSize <= result) return buf - origBuf;
				advanceBuf
			} else return buf - origBuf;
		} else if (lastActives) {
			++currentConsecutiveHits;
			if (bufSize > 1) {
				*buf = ',';
				++buf;
				--bufSize;
			}
			if (bufSize) {
				*buf = '\0';
			} else {
				return buf - origBuf;
			}
		} else {
			currentConsecutiveHits = 1;
		}
		if (currentConsecutiveHits > maxConsecutiveHits) {
			maxConsecutiveHits = currentConsecutiveHits;
		}
		const ActiveData& elem = data[i];
		result = sprintf_s(buf, bufSize, "%d", elem.actives);
		if (result != -1) {
			if ((int)bufSize <= result) return buf - origBuf;
			advanceBuf
		} else return buf - origBuf;
		lastNonActives = elem.nonActives;
		lastActives = elem.actives;
	}
	if (count > 5 && buf - origBuf > 10) {
		char ownbuf[512];
		int ownbufSize = buf - origBuf;
		if (ownbufSize > sizeof ownbuf - 1) {
			ownbufSize = sizeof ownbuf - 1;
		}
		memmove(ownbuf, origBuf, ownbufSize);
		ownbuf[ownbufSize] = '\0';
		bufSize = origBufSize;
		buf = origBuf;
		*buf = '\0';
		result = printNoSeparateHits(buf, bufSize);
		int newBufLen = result;
		if (newBufLen > 10) {
			result = printNoSeparateHitsGapsBiggerThan3(buf, bufSize);
			newBufLen = result;
		}
		if (newBufLen >= ownbufSize) {
			newBufLen = sprintf_s(buf, bufSize, "%d", total());
			if (newBufLen == -1) return buf - origBuf;
		}
		buf += newBufLen;
		bufSize -= newBufLen;
		result = sprintf_s(buf, bufSize, " / %s", ownbuf);
		advanceBuf
	}
	return buf - origBuf;
}

int ActiveDataArray::printNoSeparateHits(char* buf, size_t bufSize) const {
	int result;
	char* origBuf = buf;
	if (count == 0) {
		result = sprintf_s(buf, bufSize, "0");
		if (result == -1) return 0;
		return 1;
	}
	int lastNonActives = 0;
	for (int i = 0; i < count && bufSize; ++i) {
		if (lastNonActives) {
			result = sprintf_s(buf, bufSize, "(%d)", lastNonActives);
			if (result != -1) {
				if ((int)bufSize <= result) return buf - origBuf;
				buf += result;
				bufSize -= result;
			} else return buf - origBuf;
		}
		int n = 0;
		for(; i < count; ++i) {
			const ActiveData& elem = data[i];
			n += elem.actives;
			if (elem.nonActives) {
				lastNonActives = elem.nonActives;
				break;
			}
		}
		result = sprintf_s(buf, bufSize, "%d", n);
		if (result != -1) {
			if ((int)bufSize <= result) return buf - origBuf;
			buf += result;
			bufSize -= result;
		} else return buf - origBuf;
	}
	return buf - origBuf;
}

int ActiveDataArray::printNoSeparateHitsGapsBiggerThan3(char* buf, size_t bufSize) const {
	char* origBuf = buf;
	int result;
	if (count == 0) {
		result = sprintf_s(buf, bufSize, "0");
		if (result == -1) return 0;
		return 1;
	}
	int lastNonActives = 0;
	for (int i = 0; i < count && bufSize; ++i) {
		int n = 0;
		if (lastNonActives && lastNonActives > 5) {
			result = sprintf_s(buf, bufSize, "(%d)", lastNonActives);
			if (result != -1) {
				if ((int)bufSize <= result) return buf - origBuf;
				bufSize -= result;
				buf += result;
			} else return buf - origBuf;
		} else if (lastNonActives) {
			n += lastNonActives;
		}
		for(; i < count; ++i) {
			const ActiveData& elem = data[i];
			n += elem.actives;
			if (elem.nonActives && elem.nonActives > 5) {
				lastNonActives = elem.nonActives;
				break;
			} else if (elem.nonActives && i != count - 1) {
				n += elem.nonActives;
			}
		}
		result = sprintf_s(buf, bufSize, "%d", n);
		if (result != -1) {
			if ((int)bufSize <= result) return buf - origBuf;
			buf += result;
			bufSize -= result;
		} else return buf - origBuf;
	}
	return buf - origBuf;
}

void ActiveDataArray::removeSeparateHits(int* outIndex) {
	if (!count) return;
	int index = 0;
	bool endsOnActiveFrame = false;
	for (int i = 0; i < count; ++i) {
		ActiveData& elem = data[i];
		if (endsOnActiveFrame) {
			ActiveData& prev = data[index];
			prev.actives += elem.actives;
			if (elem.nonActives) {
				prev.nonActives = elem.nonActives;
				if (i != count - 1) {
					++index;
				}
				endsOnActiveFrame = false;
			} else if (outIndex && *outIndex > index) {
				--*outIndex;
			}
		} else {
			endsOnActiveFrame = elem.nonActives == 0;
			ActiveData& prev = data[index];
			prev.actives = elem.actives;
			prev.nonActives = elem.nonActives;
			if (!endsOnActiveFrame) {
				if (i != count - 1) {
					++index;
				}
			} else if (outIndex && *outIndex > index) {
				--*outIndex;
			}
		}
	}
	count = index + 1;
}

void ProjectileInfo::fill(Entity ent, Entity superflashInstigator, bool isCreated, bool fillName) {
	ptr = ent;
	team = ent.team();
	CharacterType ownerType = (CharacterType)-1;
	if (team == 0 || team == 1) {
		ownerType = endScene.players[team].charType;
	}
	animFrame = ent.currentAnimDuration();
	int prevLifetimeCounter = lifeTimeCounter;
	lifeTimeCounter = ent.lifeTimeCounter();
	bool dontAdvanceRamlethalTime = false;
	if (lifeTimeCounter == 0 || isCreated) {
		if (isCreated) {
			alreadyIncludedInComboRecipe = false;
		}
		maxHit.clear();
		hitNumber = 0;
		hitOnFrame = 0;
		hitstopElapsed = 0;
		bedmanSealElapsedTime = 0;
		elapsedTime = 0;
		titleIsFromAFrameThatHitSomething = false;
	}
	if (ownerType == CHARACTER_TYPE_RAMLETHAL) {
		Entity owner = endScene.players[team].pawn;
		isRamlethalSword = owner && (
				owner.stackEntity(0) == ent
				|| owner.stackEntity(1) == ent
			);
	} else {
		isRamlethalSword = false;
	}
	if (fillName && animFrame == 1 && !ent.isRCFrozen() && isRamlethalSword
			&& animationIsNeedCountRamlethalSwordTime(ent.animationName())) {
		ramlethalSwordElapsedTime = 0;
		dontAdvanceRamlethalTime = true;
	}
	int prevFrameHitstop = hitstop;
	int clashHitstop = ent.clashHitstop();
	clashedOnThisFrame = clashHitstop && !this->clashHitstop;
	if (!ent.hitSomethingOnThisFrame()) {
		hitstop = ent.hitstop() + clashHitstop;
	} else {
		hitstop = 0;
		hitstopElapsed = 0;
	}
	if (!prevFrameHitstop && hitstop) {
		hitstopMax = hitstop;
	}
	if (hitstop && !superflashInstigator && prevLifetimeCounter != lifeTimeCounter) {
		++hitstopElapsed;
	}
	if ((team == 0 || team == 1)
			&& ownerType == CHARACTER_TYPE_BEDMAN
			&& (
				prevLifetimeCounter != lifeTimeCounter
				&& lifeTimeCounter != 0
			)
			&& !superflashInstigator) {
		struct SealInfo {
			Moves::MayIrukasanRidingObjectInfo& info;
			const char* stateName;
			BBScrEvent signal;
		};
		SealInfo seals[4] {
			{ moves.bedmanSealA, "DejavIconBoomerangA", BBSCREVENT_CUSTOM_SIGNAL_6 },
			{ moves.bedmanSealB, "DejavIconBoomerangB", BBSCREVENT_CUSTOM_SIGNAL_8 },
			{ moves.bedmanSealC, "DejavIconSpiralBed", BBSCREVENT_CUSTOM_SIGNAL_9 },
			{ moves.bedmanSealD, "DejavIconFlyingBed", BBSCREVENT_CUSTOM_SIGNAL_7 }
		};
		for (int i = 0; i < 4; ++i) {
			if (strcmp(ent.animationName(), seals[i].stateName) == 0) {
				bool isFrameAfter = false;
				int remainingFrames = moves.getBedmanSealRemainingFrames(*this, seals[i].info, seals[i].signal, &isFrameAfter);
				if (remainingFrames > 0) ++bedmanSealElapsedTime;
				break;
			}
		}
	}
	if (prevLifetimeCounter != lifeTimeCounter && lifeTimeCounter != 0 && !hitstop && !superflashInstigator) {
		++elapsedTime;
		if (!dontAdvanceRamlethalTime && isRamlethalSword
				&& animationIsNeedCountRamlethalSwordTime(ent.animationName())) {
			++ramlethalSwordElapsedTime;
		}
	}
	
	int unused;
	PlayerInfo::calculateSlow(hitstopElapsed,
		hitstop,
		rcSlowedDownCounter,
		&hitstopWithSlow,
		&hitstopMaxWithSlow,
		&unused);
	
	this->clashHitstop = clashHitstop;
	int currentHitNum = ent.currentHitNum();
	maxHit.fill(ent, currentHitNum);
	if (maxHit.currentUse == -1 || maxHit.currentUse > 0 && !(maxHit.maxUse == 1 && hitNumber > 0)) {
		hitNumber = currentHitNum;
	}
	if (fillName) {
		sprite.fill(ent);
		memcpy(animName, ent.animationName(), 32);
		memcpy(trialName, ent.dealtAttack()->trialName, 32);
		if (team == 0 || team == 1) {
			fillInMove();
		}
		determineMoveNameAndSlangName(&lastName);
	}
	
	x = ptr.posX();
	y = ptr.posY();
	
	if (animFrame == 1 && titleIsFromAFrameThatHitSomething) {
		titleIsFromAFrameThatHitSomething = hitConnectedForFramebar();
	}
}

void PlayerInfo::addGap(int length) {
	if (length == 0) return;
	if (gapsCount >= _countof(gaps)) {
		memmove(gaps, gaps + 1, sizeof gaps - sizeof *gaps);
		gaps[gapsCount - 1] = length;
		return;
	}
	gaps[gapsCount++] = length;
}

void PlayerInfo::printGaps(char* buf, size_t bufSize) {
	if (gapsCount == 0) {
		if (bufSize) *buf = '\0';
		return;
	}
	for (int i = 0; i < gapsCount && bufSize; ++i) {
		int result;
		if (i > 0) {
			if (bufSize > 0) {
				*buf = ',';
				++buf;
				--bufSize;
				if (bufSize > 0) {
					*(buf) = ' ';
					++buf;
					--bufSize;
				}
			}
			if (bufSize == 0) return;
		}
		int gapLength = gaps[i];
		result = sprintf_s(buf, bufSize, "%d", gapLength);
		if (result != -1) {
			buf += result;
			if ((int)bufSize <= result) return;
			else bufSize -= result;
		}
	}
}

void PlayerInfo::clear() {
	size_t offset = offsetof(PlayerInfo, cancels);
	memset(this, 0, offset);
	for (int i = 0; i < _countof(cancels); ++i) {
		cancels[i].clear();
	}
	offset += sizeof cancels;
	cancelsTimer = 0;
	offset += sizeof cancelsTimer;
	wasCancels.clear();
	offset += sizeof wasCancels;
	prevFrameCancels.clear();
	offset += sizeof prevFrameCancels;
	cancelsCount = 0;
	offset += sizeof cancelsCount;
	dmgCalcs.clear();
	offset += sizeof dmgCalcs;
	inputs.clear();
	offset += sizeof inputs;
	lastMoveNameBeforeSuperfreeze.clear();
	offset += sizeof lastMoveNameBeforeSuperfreeze;
	lastMoveNameAfterSuperfreeze.clear();
	offset += sizeof lastMoveNameAfterSuperfreeze;
	comboRecipe.clear();
	offset += sizeof comboRecipe;
	createdProjectiles.clear();
	offset += sizeof createdProjectiles;
	memset((BYTE*)this + offset, 0, sizeof PlayerInfo - offset);
	ikMoveIndex = -1;
	counterGuardAirMoveIndex = -1;
	counterGuardStandMoveIndex = -1;
	counterGuardCrouchMoveIndex = -1;
}

void PlayerInfo::copyTo(PlayerInfo& dest) {
	memcpy(&dest, this, sizeof PlayerInfo);
}

void PrevStartupsInfo::add(short n, bool partOfStance, const NamePair* name) {
	if (count >= _countof(startups)) {
		initialSkip += startups[0].startup;
		memmove(startups, startups + 1, sizeof startups - sizeof *startups);
		--count;
		// the dumb version of ring buffer
	}
	PrevStartupsInfoElem& elem = startups[count];
	elem.partOfStance = partOfStance;
	elem.startup = n;
	elem.moveName = name;
	++count;
}

void PrevStartupsInfo::print(char*& buf, size_t& bufSize, std::vector<NameDuration>* elems) const {
	if (!count) return;
	int charsPrinted;
	
	if (initialSkip) {
		if (elems) {
			elems->push_back({ "<skipped>", initialSkip });
		}
		charsPrinted = sprintf_s(buf, bufSize, "%d", initialSkip);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	
	for (int i = 0; i < count; ++i) {
		const PrevStartupsInfoElem& elem = startups[i];
		if (elems) {
			elems->push_back({
				!elem.moveName
					? nullptr
					: settings.useSlangNames && elem.moveName->slang
						? elem.moveName->slang
						: elem.moveName->name,
				elem.startup
			});
		}
		charsPrinted = sprintf_s(buf, bufSize, i == 0 && !initialSkip ? "%d" : "+%d", elem.startup);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	charsPrinted = sprintf_s(buf, bufSize, "+");
	if (charsPrinted == -1) return;
	buf += charsPrinted;
	bufSize -= charsPrinted;
}

const char* PlayerInfo::getLastPerformedMoveName(bool disableSlang) const { 
	if (!lastPerformedMoveName) return nullptr;
	if (!disableSlang && settings.useSlangNames && lastPerformedMoveName->slang) {
		return lastPerformedMoveName->slang;
	} else {
		return lastPerformedMoveName->name;
	}
}

void PlayerInfo::updateLastMoveNameBeforeAfterSuperfreeze(bool disableSlang) {
	const char* name = getLastPerformedMoveName(disableSlang);
	
	lastMoveNameBeforeSuperfreeze = name;
	lastMoveNameBeforeSuperfreeze += " Superfreeze Startup";
	
	lastMoveNameAfterSuperfreeze = name;
	lastMoveNameAfterSuperfreeze += " After Superfreeze";
}

int PrevStartupsInfo::countOfNonEmptyUniqueNames(const char** lastNames, int lastNamesCount, bool slang) const {
	int answer = 0;
	for (int i = 0; i < count + lastNamesCount; ++i) {
		const char* selectedName;
		if (i >= count) {
			selectedName = lastNames[i - count];
		} else {
			selectedName = startups[i].selectName(slang);
		}
		if (!selectedName) continue;
		if (i > 0 && i - 1 < count) {
			const char* otherName = startups[i - 1].selectName(slang);
			if (otherName && strcmp(selectedName, otherName) == 0) continue;
		}
		
		++answer;
		
		int sameCount = 0;
		for (int j = i + 1; j < count + lastNamesCount; ++j) {
			const char* otherName;
			if (j >= count) {
				otherName = lastNames[j - count];
			} else {
				otherName = startups[j].selectName(slang);
			}
			if (!otherName) continue;
			if (strcmp(selectedName, otherName) == 0) {
				++sameCount;
			} else {
				break;
			}
		}
		if (sameCount) {
			i += sameCount;
		}
	}
	return answer;
}

void PrevStartupsInfo::printNames(char*& buf, size_t& bufSize, const char** lastNames, int lastNamesCount, bool slang, bool useMultiplicationSign,
								bool printFrames, int* lastNamesDurations) const {
	int charsPrinted;
	bool isFirst = true;
	
	if (initialSkip) {
		charsPrinted = sprintf_s(buf, bufSize, "???");
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		
		isFirst = false;
		if (printFrames) {
			charsPrinted = sprintf_s(buf, bufSize, " (frames 1-%d)", initialSkip);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
	}
	
	int currentStart = initialSkip + 1;
	for (int i = 0; i < count + lastNamesCount; ++i) {
		int currentEnd = currentStart;
		const char* selectedName;
		if (i >= count) {
			selectedName = lastNames[i - count];
			if (lastNamesDurations) {
				currentEnd += lastNamesDurations[i - count];
			}
		} else {
			selectedName = startups[i].selectName(slang);
			currentEnd += startups[i].startup;
		}
		if (!selectedName) continue;
		if (i > 0 && i - 1 < count) {
			const char* otherName = startups[i - 1].selectName(slang);
			if (otherName && strcmp(selectedName, otherName) == 0) continue;
		}
		
		
		
		int sameCount = 0;
		for (int j = i + 1; j < count + lastNamesCount; ++j) {
			const char* otherName;
			if (j >= count) {
				otherName = lastNames[j - count];
			} else {
				otherName = startups[j].selectName(slang);
			}
			if (!otherName) continue;
			if (strcmp(selectedName, otherName) == 0) {
				++sameCount;
			} else {
				break;
			}
		}
		
		charsPrinted = sprintf_s(buf, bufSize, isFirst ? "%s" : "+%s", selectedName);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		
		if (sameCount) {
			if (useMultiplicationSign) {
				charsPrinted = sprintf_s(buf, bufSize, "*%d", sameCount + 1);
				if (charsPrinted == -1) return;
				buf += charsPrinted;
				bufSize -= charsPrinted;
			}
			i += sameCount;
		}
		
		if (printFrames) {
			if (i >= count && !lastNamesDurations) {
				charsPrinted = sprintf_s(buf, bufSize, " (frames %d+)", currentStart);
			} else {
				charsPrinted = sprintf_s(buf, bufSize, " (frames %d-%d)", currentStart, currentEnd - 1);
			}
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
		currentStart = currentEnd;
		isFirst = false;
	}
}

int PlayerInfo::startupType() const {
	if (superfreezeStartup && superfreezeStartup <= startupDisp && (startedUp || startupProj)) {
		return 0;
	} else if (superfreezeStartup && !(startedUp || startupProj)) {
		return 1;
	} else if (startedUp || startupProj) {
		return 2;
	} else {
		return -1;
	}
}

void PlayerInfo::printStartup(char* buf, size_t bufSize, std::vector<NameDuration>* elems) {
	if (elems) elems->clear();
	if (!bufSize) return;
	*buf = '\0';
	int uhh = startupType();
	if (uhh == -1) return;
	prevStartupsDisp.print(buf, bufSize, elems);
	const char* lastName = getLastPerformedMoveName(false);
	updateLastMoveNameBeforeAfterSuperfreeze(false);
	for (int i = 0; ; ++i) {
		int charsPrinted = -1;
		if (uhh == 0) {
			if (elems) {
				elems->push_back({ lastMoveNameBeforeSuperfreeze.c_str(), superfreezeStartup });
				elems->push_back({ lastMoveNameAfterSuperfreeze.c_str(), startupDisp - superfreezeStartup });
			}
			charsPrinted = sprintf_s(buf, bufSize, "%d+%d", superfreezeStartup, startupDisp - superfreezeStartup);
		} else if (uhh == 1) {
			if (elems) {
				elems->push_back({ lastName, superfreezeStartup });
			}
			charsPrinted = sprintf_s(buf, bufSize, "%d", superfreezeStartup);
		} else if (uhh == 2) {
			if (elems) {
				elems->push_back({ lastName, startupDisp });
			}
			if (prevStartupsDisp.count && superfreezeStartup) {
				charsPrinted = sprintf_s(buf, bufSize, "%d (superfreeze happens on frame %d)", startupDisp, prevStartupsDisp.total() + superfreezeStartup);
			} else {
				charsPrinted = sprintf_s(buf, bufSize, "%d", startupDisp);
			}
		}
		
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		
		if (i == 0 && prevStartupsDisp.count > 1) {
			charsPrinted = sprintf_s(buf, bufSize, "=%d+", prevStartupsDisp.total());
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		} else {
			break;
		}
	}
}

int PlayerInfo::printStartupForFramebar() {
	int uhh = startupType();
	if (uhh == -1) return 0;
	int result = prevStartupsDisp.total();
	if (uhh == 0) {
		return result + startupDisp;
	} else if (uhh == 1) {
		return result + superfreezeStartup;
	} else if (uhh == 2) {
		return result + startupDisp;
	} else {
		return 0;
	}
}

void ProjectileInfo::printStartup(char* buf, size_t bufSize) {
	if (!bufSize) return;
	*buf = '\0';
	prevStartups.print(buf, bufSize);
	sprintf_s(buf, bufSize, "%d", startup);
}

void PlayerInfo::printRecovery(char* buf, size_t bufSize) {
	if (!bufSize) return;
	*buf = '\0';
	int charsPrinted = 0;
	bool printedTheMainThing = false;
	bool mentionedCantAttack = false;
	bool answerTaunt = recoveryDispCanBlock != -1 && recoveryDisp == 0;
	if ((startedUp || startupProj) && !(recoveryDisp == 0 && landingRecovery)) {
		if (!answerTaunt) {
			if (totalCanBlock < totalDisp && recoveryDisp - (totalDisp - totalCanBlock) > 0) {
				charsPrinted = sprintf_s(buf, bufSize, "%d can't block+%d can't attack",
					recoveryDisp - (totalDisp - totalCanBlock),
					totalDisp - totalCanBlock);
				mentionedCantAttack = true;
			} else {
				charsPrinted = sprintf_s(buf, bufSize, "%d", recoveryDisp);
			}
		} else {
			charsPrinted = sprintf_s(buf, bufSize, "%d can't block", recoveryDispCanBlock);
		}
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		printedTheMainThing = true;
	}
	if (printedTheMainThing && totalCanBlock > totalDisp && !answerTaunt) {
		charsPrinted = sprintf_s(buf, bufSize, "%s%s%d can't block",
			mentionedCantAttack ? " can't attack" : "",
			charsPrinted ? "+" : "",
			totalCanBlock - totalDisp);
		mentionedCantAttack = true;
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	int maxOfTheTwo = max(totalCanBlock, totalDisp);
	if (printedTheMainThing
			&& totalCanFD > maxOfTheTwo
			&& maxOfTheTwo != 0
			&& totalCanBlock != 0
			&& !answerTaunt
	) {
		charsPrinted = sprintf_s(buf, bufSize, "%s%s%d can't FD",
			mentionedCantAttack ? " can't attack" : "",
			charsPrinted ? "+" : "",
			totalCanFD - maxOfTheTwo);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	if (landingRecovery) {
		charsPrinted = sprintf_s(buf, bufSize, "%s%d landing", charsPrinted ? "+" : "", landingRecovery);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	if (sinHungerRecovery) {
		charsPrinted = sprintf_s(buf, bufSize, "%s%d hunger", charsPrinted ? "+" : "", sinHungerRecovery);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	if (printedTheMainThing && totalFD) {
		if (charsPrinted && bufSize) {
			strcat(buf, "+");
			++buf;
			--bufSize;
		}
		sprintf_s(buf, bufSize, "%d FD", totalFD);
	}
}

int PlayerInfo::printRecoveryForFramebar() {
	int result = 0;
	bool printedTheMainThing = false;
	bool answerTaunt = recoveryDispCanBlock != -1 && recoveryDisp == 0;
	if ((startedUp || startupProj) && !(recoveryDisp == 0 && landingRecovery)) {
		if (!answerTaunt) {
			result = recoveryDisp;
		} else {
			result = recoveryDispCanBlock;
		}
		printedTheMainThing = true;
	}
	if (printedTheMainThing && totalCanBlock > totalDisp && !answerTaunt) {
		result += totalCanBlock - totalDisp;
	}
	int maxOfTheTwo = max(totalCanBlock, totalDisp);
	if (printedTheMainThing
			&& totalCanFD > maxOfTheTwo
			&& maxOfTheTwo != 0
			&& totalCanBlock != 0
			&& !answerTaunt
	) {
		result += totalCanFD - maxOfTheTwo;
	}
	result += landingRecovery + sinHungerRecovery;
	if (printedTheMainThing && totalFD) {
		result += totalFD;
	}
	return result;
}

void PlayerInfo::printTotal(char* buf, size_t bufSize, std::vector<NameDuration>* elems) {
	if (elems) elems->clear();
	if (!bufSize) return;
	char* origBuf = buf;
	*buf = '\0';
	int charsPrinted;
	bool printedMainPart = false;
	int partsCount = 0;
	int partsTotal = 0;
	bool mentionedCantAttack = false;
	int maxOfTheTwo = max(totalCanBlock, totalDisp);
	if (canPrintTotal()) {
		printedMainPart = true;
		prevStartupsTotalDisp.print(buf, bufSize, elems);
		partsTotal = prevStartupsTotalDisp.total();
		partsCount += prevStartupsTotalDisp.count;
		if (totalCanBlock < totalDisp && totalCanBlock != 0) {
			if (elems) {
				elems->push_back({ "can't block", totalCanBlock });
				elems->push_back({ "can't attack", totalDisp - totalCanBlock });
			}
			charsPrinted = sprintf_s(buf, bufSize, "%d can't block+%d can't attack", totalCanBlock, totalDisp - totalCanBlock);
			partsCount += 2;
			mentionedCantAttack = true;
		} else {
			if (elems) {
				elems->push_back({ getLastPerformedMoveName(false), totalDisp });
			}
			charsPrinted = sprintf_s(buf, bufSize, "%d", totalDisp);
			++partsCount;
		}
		partsTotal += totalCanBlock > totalDisp ? totalCanBlock : totalDisp;
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		
		if (totalCanBlock > totalDisp) {
			if (elems) {
				elems->push_back({ "can't block", totalCanBlock - totalDisp });
			}
			charsPrinted = sprintf_s(buf, bufSize, " can't attack+%d can't block", totalCanBlock - totalDisp);
			mentionedCantAttack = true;
			++partsCount;
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
		if (totalCanFD > maxOfTheTwo && maxOfTheTwo != 0 && totalCanBlock != 0) {
			if (elems) {
				elems->push_back({ "can't FD", totalCanFD - maxOfTheTwo });
			}
			charsPrinted = sprintf_s(buf, bufSize, "%s+%d can't FD",
				mentionedCantAttack ? "" : " can't attack",
				totalCanFD - maxOfTheTwo);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
		if (landingRecovery) {
			if (elems) {
				elems->push_back({ "landing", landingRecovery });
			}
			charsPrinted = sprintf_s(buf, bufSize, "+%d landing", landingRecovery);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
		if (sinHungerRecovery) {
			if (elems) {
				elems->push_back({ "hunger", sinHungerRecovery });
			}
			charsPrinted = sprintf_s(buf, bufSize, "+%d hunger", sinHungerRecovery);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
	}
	if (totalFD) {
		if (elems) {
			elems->push_back({ "FD", totalFD });
		}
		charsPrinted = sprintf_s(buf, bufSize, "%s%d FD", printedMainPart ? "+" : "", totalFD);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	
	if (partsCount > 2) {
		charsPrinted = sprintf_s(buf, bufSize, "=%d", partsTotal);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		if (landingRecovery) {
			charsPrinted = sprintf_s(buf, bufSize, "+%d landing", landingRecovery);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		if (sinHungerRecovery) {
			charsPrinted = sprintf_s(buf, bufSize, "+%d hunger", sinHungerRecovery);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		if (totalFD) {
			charsPrinted = sprintf_s(buf, bufSize, "+%d FD", totalFD);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		if (totalCanFD > maxOfTheTwo && maxOfTheTwo != 0 && totalCanBlock != 0) {
			charsPrinted = sprintf_s(buf, bufSize, "+%d can't FD",
				totalCanFD - maxOfTheTwo);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
	}
}

void ProjectileInfo::printTotal(char* buf, size_t bufSize) {
	if (!bufSize) return;
	*buf = '\0';
	prevStartups.print(buf, bufSize);
	sprintf_s(buf, bufSize, "%d", total);
}

bool PlayerInfo::isIdleInNewSection() {
	return inNewMoveSection
		&& move.considerIdleInSeparatedSectionAfterThisManyFrames
		&& timeInNewSection > move.considerIdleInSeparatedSectionAfterThisManyFrames;
}

bool PlayerInfo::isInVariableStartupSection() {
	return inNewMoveSection
			&& (move.considerNewSectionAsBeingInVariableStartup
				|| move.considerIdleInSeparatedSectionAfterThisManyFrames
				&& !(
					charType == CHARACTER_TYPE_JOHNNY
					&& pawn
					&& pawn.mem54()
					&& (
						strcmp(anim, "MistFinerLoop") == 0
						|| strcmp(anim, "AirMistFinerLoop") == 0
					)
				))
			|| move.isInVariableStartupSection
			&& move.isInVariableStartupSection(*this);
}

int PrevStartupsInfo::total() const {
	int sum = 0;
	for (int i = 0; i < count; ++i) {
		sum += startups[i].startup;
	}
	return initialSkip + sum;
}

void SpriteFrameInfo::print(char* buf, size_t bufSize) const {
	if (frameMax == 2147483647) {
		sprintf_s(buf, bufSize, "%s (%d/inf)", name, frame);
	} else {
		sprintf_s(buf, bufSize, "%s (%d/%d)", name, frame, frameMax);
	}
}

void SpriteFrameInfo::fill(Entity ent) {
	memcpy(name, ent.spriteName(), 32);
	frame = ent.spriteFrameCounter();
	frameMax = ent.spriteFrameCounterMax();
}

void EntityFramebar::utf8len(const char* txt, int* byteLen, int* cpCountTotal, int maxCodepointCount, int* byteLenBelowMax) {
	int cpCount = 0;
	const char* c = txt;
	while (*c != '\0') {
		char ch = *c;
		if (cpCount == maxCodepointCount && byteLenBelowMax) {
			*byteLenBelowMax = c - txt;
		}
		++c;
		++cpCount;
		int extraBytesCount = 0;
		if ((ch & 0b11100000) == 0b11000000) {
			extraBytesCount = 1;
		} else if ((ch & 0b11110000) == 0b11100000) {
			extraBytesCount = 2;
		} else if ((ch & 0b11111000) == 0b11110000) {
			extraBytesCount = 3;
		}
		for (int i = 0; i < extraBytesCount && *c != '\0'; ++i) {
			++c;
		}
	}
	if (cpCount <= maxCodepointCount && byteLenBelowMax) {
		*byteLenBelowMax = c - txt;
	}
	*byteLen = c - txt;
	*cpCountTotal = cpCount;
}

int EntityFramebar::confinePos(int pos) {
	if (pos < 0) {
		return (int)_countof(Framebar::frames) + (pos + 1) % (int)_countof(Framebar::frames) - 1;  // (int) very important x_x (all covered in bruises) (written in blood)
	} else {
		return pos % _countof(Framebar::frames);
	}
}

int EntityFramebar::confinePos(int pos, int size) {
	if (pos < 0) {
		return size + (pos + 1) % size - 1;
	} else {
		return pos % size;
	}
}

template<typename FramebarT>
inline int findTickNoGreaterThan(const FramebarT* framebar, int startingPos, DWORD tick) {
	for (int i = 0; i < _countof(framebar->frames); ++i) {
		int curPos = (startingPos - i + _countof(framebar->frames)) % _countof(framebar->frames);
		if (framebar->frames[curPos].type == FT_NONE) return -1;
		DWORD curTick = framebar->frames[curPos].aswEngineTick;
		if (curTick <= tick) {
			return curPos;
		}
	}
	return -1;
}

int Framebar::findTickNoGreaterThan(int startingPos, DWORD tick) const {
	return ::findTickNoGreaterThan<Framebar>(this, startingPos, tick);
}

int PlayerFramebar::findTickNoGreaterThan(int startingPos, DWORD tick) const {
	return ::findTickNoGreaterThan<PlayerFramebar>(this, startingPos, tick);
}

template<typename EntFramebarT, typename FramebarT, typename FrameT>
static inline void changePreviousFrames_piece(EntFramebarT* framebars,
			FrameType* prevTypes,
			int prevTypesCount,
			int& pos,
			FramebarT& bar,
			DWORD aswEngineTick,
			FrameType newType) {
	
	if (pos != -1) {
		FrameT& otherFrame = bar[pos];
		if (otherFrame.aswEngineTick == aswEngineTick) {
			int j;
			for (j = 0; j < prevTypesCount; ++j) {
				if (otherFrame.type == prevTypes[j]) {
					otherFrame.type = newType;
					framebars->decrementPos(pos);
					break;
				}
			}
			if (j == prevTypesCount) {
				pos = -1;
			}
		} else if (otherFrame.aswEngineTick > aswEngineTick) {
			framebars->decrementPos(pos);
		}
	}
}

template<typename EntFramebarT, typename FramebarT, typename FrameT>
inline void changePreviousFrames(EntFramebarT* framebars,
		FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame) {
	if (maxCount <= 0 || prevTypesCount <= 0) return;
	
	positionHitstopIdle = EntityFramebar::confinePos(positionHitstopIdle);
	
	DWORD aswEngineTick = framebars->idleHitstop[positionHitstopIdle].aswEngineTick;
	
	int hitstopPos = framebars->hitstop.findTickNoGreaterThan(EntityFramebar::confinePos(positionHitstop), aswEngineTick);
	int idlePos = framebars->idle.findTickNoGreaterThan(EntityFramebar::confinePos(positionIdle), aswEngineTick);
	int mainPos = framebars->main.findTickNoGreaterThan(EntityFramebar::confinePos(position), aswEngineTick);
	
	while (maxCount) {
		FrameT& frame = framebars->idleHitstop[positionHitstopIdle];
		
		if (stopAtFirstFrame && frame.isFirst) break;
		
		int i;
		for (i = 0; i < prevTypesCount; ++i) {
			if (frame.type == prevTypes[i]) {
				frame.type = newType;
				
				#define piece(pos, bar) \
					changePreviousFrames_piece<EntFramebarT, FramebarT, FrameT>( \
						framebars, \
						prevTypes, \
						prevTypesCount, \
						pos, \
						bar, \
						frame.aswEngineTick, \
						newType);
				
				piece(hitstopPos, framebars->hitstop)
				piece(idlePos, framebars->idle)
				piece(mainPos, framebars->main)
				
				#undef piece
				break;
			}
		}
		if (i == prevTypesCount) {
			break;
		}
		
		--maxCount;
		framebars->decrementPos(positionHitstopIdle);
	}
}

void ProjectileFramebar::changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame) {
	::changePreviousFrames<ProjectileFramebar, Framebar, Frame>(this,
		prevTypes,
		prevTypesCount,
		newType,
		positionHitstopIdle,
		positionHitstop,
		positionIdle,
		position,
		maxCount,
		stopAtFirstFrame);
}

void CombinedProjectileFramebar::changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame) {
}

void PlayerFramebars::changePreviousFrames(FrameType* prevTypes,
		int prevTypesCount,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame) {
	::changePreviousFrames<PlayerFramebars, PlayerFramebar, PlayerFrame>(this,
		prevTypes,
		prevTypesCount,
		newType,
		positionHitstopIdle,
		positionHitstop,
		positionIdle,
		position,
		maxCount,
		stopAtFirstFrame);
}

template<typename FramebarT, typename FrameT>
inline void soakUpIntoPreFrame(FramebarT* framebar, const FrameT& srcFrame) {
	if (framebar->preFrame == srcFrame.type && !srcFrame.isFirst) {
		++framebar->preFrameLength;
	} else {
		framebar->preFrame = srcFrame.type;
		framebar->preFrameLength = 1;
	}
	// I was so naive (discovered that I have to do this purely by accident)
	FrameType type = frameMap(srcFrame.type);
	if (framebar->preFrameMapped == type && !srcFrame.isFirst) {
		++framebar->preFrameMappedLength;
	} else {
		framebar->preFrameMapped = type;
		framebar->preFrameMappedLength = 1;
	}
	type = frameMapNoIdle(srcFrame.type);
	if (framebar->preFrameMappedNoIdle == type && !srcFrame.isFirst) {
		++framebar->preFrameMappedNoIdleLength;
	} else {
		framebar->preFrameMappedNoIdle = type;
		framebar->preFrameMappedNoIdleLength = 1;
	}
}

void Framebar::soakUpIntoPreFrame(const FrameBase& srcFrame) {
	::soakUpIntoPreFrame<Framebar, Frame>(this, (Frame&)srcFrame);
}

void PlayerFramebar::soakUpIntoPreFrame(const FrameBase& srcFrame) {
	::soakUpIntoPreFrame<PlayerFramebar, PlayerFrame>(this, (PlayerFrame&)srcFrame);
}

static inline int determineFrameLevel(FrameType type) {
	if (isEddieFrame(type)) {
		return 7;
	}
	if (type == FT_ACTIVE_PROJECTILE) {
		return 6;
	}
	if (type == FT_ACTIVE_HITSTOP_PROJECTILE) {
		return 5;
	}
	if (type == FT_NON_ACTIVE_PROJECTILE) {
		return 4;
	}
	if (type == FT_IDLE_PROJECTILE_HITTABLE) {
		return 3;
	}
	if (type == FT_BACCHUS_SIGH) {
		return 2;
	}
	if (type == FT_IDLE_PROJECTILE || type == FT_IDLE_NO_DISPOSE) {
		return 1;
	}
	return 0;
}

enum SortItemType {
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) enumName,
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
};
const SortItemType SUPER_ARMOR_FIRST = SUPER_ARMOR_THROW;
const SortItemType SUPER_ARMOR_LAST = SUPER_ARMOR_BLITZ_BREAK;

struct InvulFlags {
	
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) bool fieldName:1;
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
	
	int print(char* buf,
		size_t bufSize,
		bool includeASpaceAtTheStart,
		const char* guardImpossibleTooltipHelp,
		bool needPrintHighMidLowInfoWhenItsAllGuardArmor,
		bool* out_thisSuperArmorHasMidHighLowInfo,
		bool* advisedToReadGuardImpossibleTooltip,
		bool justSayThatSuperArmorIsSameAsSomePreviousOne) const;
};

void PlayerInfo::printInvuls(char* buf, size_t bufSize) const {
	
	if (bufSize) *buf = '\0';
	if (!canPrintTotal()) {
		return;
	}
	
	struct SortItem {
		SortItemType type = STRIKE_INVUL;
		const InvulData* invulData = nullptr;
		bool included = false;
		int index = 0;
		int position = INT_MAX;  // starts from 1. Values < start are behind the earliest start
		ActiveData last { 0 };
		SortItem(SortItemType type,
				const InvulData* invulData
				) :
				type(type),
				invulData(invulData)
			{
			if (invulData->start && invulData->frames.count) {
				index = 0;
				position = invulData->start;
				last = invulData->frames.data[0];
			}
		}
		inline bool isSuperArmorType() const { return type >= SUPER_ARMOR_FIRST && type <= SUPER_ARMOR_LAST; }
	};
	
	SortItem items[] {
		// the types must go in the same order as the elements of SortItemType enum
		
		#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) { enumName, &fieldName },
		INVUL_TYPES_TABLE
		#undef INVUL_TYPES_EXEC
		
	};
	int count = _countof(items);
	
	struct SuperArmorProperties {
		bool valid = false;
		
		SortItem* armors[SUPER_ARMOR_LAST - SUPER_ARMOR_FIRST + 1] { nullptr };
		int count = 0;
		
		inline void add(SortItem* item) {
			armors[count++] = item;
		}
	};
	SuperArmorProperties prevSuperArmor { false };
	bool prevSuperArmorHadMidHighLowInfo = false;
	
	int position;
	int minLength;
	// keep track of which to include using bool included in struct SortItem
	
	int result;
	bool needComma = false;
	bool alreadyAdvisedToCheckTooltip = false;
	do {
		
		position = INT_MAX;
		for (int i = 0; i < count; ++i) {
			SortItem& item = items[i];
			if (item.position < position) {
				position = item.position;
			}
		}
		if (position == INT_MAX) break;
		
		SuperArmorProperties currentSuperArmor { false };
		minLength = INT_MAX;
		for (int i = 0; i < count; ++i) {
			SortItem& item = items[i];
			if (item.position == position) {
				if (item.isSuperArmorType()) {
					currentSuperArmor.add(&item);
				}
				item.included = true;
				const int actives = item.last.actives;
				if (actives < minLength) {
					minLength = actives;
				}
			} else {
				item.included = false;
				if (item.position != INT_MAX) {
					const int actives = item.position - position;
					if (actives < minLength) {
						minLength = actives;
					}
				}
			}
		}
		
		bool superArmorIsSame = false;
		if (items[SUPER_ARMOR].included) {
			currentSuperArmor.valid = true;
			if (!prevSuperArmor.valid) {
				prevSuperArmor = currentSuperArmor;
			} else {
				if (memcmp(&prevSuperArmor, &currentSuperArmor, sizeof SuperArmorProperties) == 0) {
					superArmorIsSame = true;
				} else {
					prevSuperArmor = currentSuperArmor;
				}
			}
		}
		
		if (minLength != INT_MAX) {
			// the caller is responsible for clearing the stack for vararg functions, so I should be able to supply more arguments than the function will use
			result = sprintf_s(buf, bufSize,
				minLength == 1
				&& needComma  // the needComma check is needed because from my experience displaying the initial "just the frame number" was confusing as fuck, like I though it starts from frame 1 and is that many frames long
				&& false  // actually you know what, disable this completely, this is too easy to confuse with the amount of frames since the previous range
				? "%s%d" : "%s%d-%d",
				needComma ? ", " : "",
				position,
				position + minLength - 1);
			if (result != -1) {
				buf += result;
				bufSize -= result;
			}
			needComma = true;
		}
		
		InvulFlags flags;
		
		#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) flags.fieldName = items[enumName].included;
		INVUL_TYPES_TABLE
		#undef INVUL_TYPES_EXEC
		
		int printedChars = flags.print(buf,
			bufSize,
			true,
			alreadyAdvisedToCheckTooltip ? "" : " - see full list in tooltip",
			prevSuperArmorHadMidHighLowInfo,
			&prevSuperArmorHadMidHighLowInfo,
			&alreadyAdvisedToCheckTooltip,
			superArmorIsSame);
		
		buf += printedChars;
		bufSize -= printedChars;
		
		for (int i = 0; i < count; ++i) {
			SortItem& item = items[i];
			if (item.included) {
				const int actives = item.last.actives;
				if (actives == minLength) {
					++item.index;
					if (item.index >= item.invulData->frames.count) {
						item.position = INT_MAX;
					} else {
						item.position += item.last.actives + item.last.nonActives;
						item.last = item.invulData->frames.data[item.index];
					}
				} else {
					item.last.actives -= minLength;
					item.position += minLength;
				}
			}
		}
		
	} while (1);
	
}

void InvulData::clear() {
	start = 0;
	frames.clear();
}

void InvulData::addInvulFrame(int prevTotal) {
	if (!start && active) {
		start = prevTotal;
	}
	frames.hitNumConflict = true;
	if (active) {
		frames.addActive(-1, 1);
	} else {
		frames.addNonActive(1);
	}
}

bool PlayerInfo::canPrintTotal() const {
	return !(onTheDefensive && !idlePlus) && totalDisp;
}

void PlayerFrame::printInvuls(char* buf, size_t bufSize) const {
	if (!bufSize) return;
	*buf = '\0';
	
	InvulFlags flags;
	
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) flags.fieldName = fieldName;
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
	
	flags.print(buf,
		bufSize,
		false,
		" - see full list in the tooltip of field 'Invul' in the main mod's UI",
		false,
		nullptr,
		nullptr,
		false);
	
}

void printFameStop(char* buf, size_t bufSize, const FrameStopInfo* stopInfo,
					int hitstop, int hitstopMax, bool lastBlockWasIB, bool lastBlockWasFD) {
	if (!bufSize) return;
	*buf = '\0';
	bool hasStop = stopInfo ? (
			stopInfo->isHitstun
			|| stopInfo->isBlockstun
			|| stopInfo->isStagger
			|| stopInfo->isWakeup
			|| stopInfo->isRejection
		) : false;
	if (!hitstop && !hasStop) return;
	int result;
	
	if (hitstop && hitstopMax) {
		result = sprintf_s(buf, bufSize, "%d/%d hitstop%s", hitstop, hitstopMax,
			hasStop ? "+" : "");
		if (result != -1) {
			buf += result;
			bufSize -= result;
		}
	} else if (hitstop) {
		result = sprintf_s(buf, bufSize, "%d hitstop%s", hitstop,
			hasStop ? "+" : "");
		if (result != -1) {
			buf += result;
			bufSize -= result;
		}
	}
	if (hasStop) {
		const char* stunName;
		if (stopInfo->isHitstun) {
			stunName = "hitstun";
		} else if (stopInfo->isBlockstun) {
			if (lastBlockWasIB) {
				stunName = "blockstun (IB)";
			} else if (lastBlockWasFD) {
				stunName = "blockstun (FD)";
			} else {
				stunName = "blockstun";
			}
		} else if (stopInfo->isStagger) {
			stunName = "stagger";
		} else if (stopInfo->isRejection) {
			stunName = "rejection";
		} else {
			stunName = "wakeup";
		}
		if (stopInfo->valueMaxExtra) {
			result = sprintf_s(buf, bufSize, "%d/(%d+%d) %s", stopInfo->value - (hitstop ? 1 : 0),
				stopInfo->valueMax, stopInfo->valueMaxExtra, stunName);
		} else {
			result = sprintf_s(buf, bufSize, "%d/%d %s", stopInfo->value - (hitstop ? 1 : 0),
				stopInfo->valueMax, stunName);
		}
		if (result != -1) {
			buf += result;
			bufSize -= result;
		}
	}
}

int InvulFlags::print(char* buf,
		size_t bufSize,
		bool includeASpaceAtTheStart,
		const char* guardImpossibleTooltipHelp,
		bool needPrintHighMidLowInfoWhenItsAllGuardArmor,
		bool* out_thisSuperArmorHasMidHighLowInfo,
		bool* advisedToReadGuardImpossibleTooltip,
		bool justSayThatSuperArmorIsSameAsSomePreviousOne) const {
	
	char* origBuf = buf;
	struct SortItem {
		SortItemType type;
		const char* description;
		bool active;
		inline bool isSuperArmorType() const { return type >= SUPER_ARMOR_FIRST && type <= SUPER_ARMOR_LAST; }
	};
	
	SortItem items[] {
		// the types must go in the same order as the elements of SortItemType enum
		
		#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) { enumName, stringDesc, fieldName },
		INVUL_TYPES_TABLE
		#undef INVUL_TYPES_EXEC
		
	};
	int count = _countof(items);
	
	bool hasFull = strikeInvul && throwInvul;
	bool printedFull = false;
	bool needPlus = false;
	int result;
	
	for (int i = 0; i < count; ++i) {
		SortItem& item = items[i];
		if (item.active) {
			bool shouldBeIncludedInFull = hasFull && (item.type == STRIKE_INVUL || item.type == THROW_INVUL);
			if (!(shouldBeIncludedInFull && printedFull || item.isSuperArmorType())) {
				const char* stringArg = item.description;
				if (shouldBeIncludedInFull) {
					stringArg = "full";
					printedFull = true;
				}
				result = sprintf_s(buf, bufSize,
					needPlus
						? " + %s%s"
						: includeASpaceAtTheStart
							? " %s%s" : "%s%s",
					stringArg,
					item.type == SUPER_ARMOR && justSayThatSuperArmorIsSameAsSomePreviousOne ? " (same)" : ""
				);
				includeASpaceAtTheStart = true;
				if (result != -1) {
					buf += result;
					bufSize -= result;
				}
				bool thisSuperArmorHasMidHighLowInfo = false;
				if (item.type == SUPER_ARMOR && !justSayThatSuperArmorIsSameAsSomePreviousOne) {
					bool flickableOnly = !superArmorProjectileLevel0 && superArmorObjectAttacck;
					bool flickableOnlyOnly = flickableOnly && !superArmorHontaiAttacck;
					bool hasObjectAndHontai = superArmorObjectAttacck && superArmorHontaiAttacck;
					bool ignoreMidLowOverhead = flickableOnly;
					bool needSubplus = false;
					char* oldBuf = buf;
					if (bufSize >= 2) {
						buf += 2;
						bufSize -= 2;
					}
					bool allGuard = false;
					if (!flickableOnlyOnly) {
						result = -1;
						if (superArmorMid
								&& superArmorOverhead
								&& !superArmorLow) {
							ignoreMidLowOverhead = true;
							result = sprintf_s(buf, bufSize, "mids and overheads");
						}
						allGuard = superArmorMid
								&& superArmorOverhead
								&& superArmorLow;
						if (allGuard) {
							ignoreMidLowOverhead = true;
							if (needPrintHighMidLowInfoWhenItsAllGuardArmor) {
								result = sprintf_s(buf, bufSize, "lows, mids and overheads");
								allGuard = false;
							}
						}
						if (superArmorMid
								&& !superArmorOverhead
								&& superArmorLow) {
							ignoreMidLowOverhead = true;
							result = sprintf_s(buf, bufSize, "mids and lows");
						}
						if (!superArmorMid
								&& !superArmorOverhead
								&& superArmorLow) {
							ignoreMidLowOverhead = true;
							result = sprintf_s(buf, bufSize, "lows only");
						}
						if (result != -1) {
							thisSuperArmorHasMidHighLowInfo = true;
							needSubplus = true;
							buf += result;
							bufSize -= result;
						}
					}
					if (flickableOnly) {
						if (allGuard) {
							result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s", "lows, mids and overheads");
							if (result != -1) {
								needSubplus = true;
								buf += result;
								bufSize -= result;
							}
							allGuard = false;
						}
						result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s",
							!flickableOnlyOnly ? "if projectiles, then flickable only" : "flickable projectiles only");
						if (result != -1) {
							needSubplus = true;
							buf += result;
							bufSize -= result;
						}
					}
					
					bool gaveHintThatCantArmorSupers = false;
					for (int j = SUPER_ARMOR_FIRST; j <= SUPER_ARMOR_LAST; ++j) {
						SortItem& superArmorType = items[j];
						if (!superArmorType.active
								|| j == SUPER_ARMOR_PROJECTILE_LEVEL_0
								|| j == SUPER_ARMOR_BLITZ_BREAK
								|| !superArmorHontaiAttacck
								&& (
									j == SUPER_ARMOR_THROW
									|| j == SUPER_ARMOR_GUARD_IMPOSSIBLE
									|| j == SUPER_ARMOR_OVERDRIVE
								)
								|| flickableOnlyOnly
								&& (
									j == SUPER_ARMOR_HONTAI_ATTACCK
									|| j == SUPER_ARMOR_OBJECT_ATTACCK
								)
								|| j == SUPER_ARMOR_BURST) continue;
						
						if (hasObjectAndHontai && (j == SUPER_ARMOR_OBJECT_ATTACCK || j == SUPER_ARMOR_HONTAI_ATTACCK)) {
							continue;
						}
						if (ignoreMidLowOverhead && (j == SUPER_ARMOR_MID || j == SUPER_ARMOR_OVERHEAD || j == SUPER_ARMOR_LOW)) {
							continue;
						}
						if (j == SUPER_ARMOR_OVERDRIVE && !superArmorBlitzBreak) continue;
						if (allGuard) {
							result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s", "lows, mids and overheads");
							if (result != -1) {
								needSubplus = true;
								buf += result;
								bufSize -= result;
							}
							allGuard = false;
						}
						// all overdrives are blitz breaks by default
						if (j == SUPER_ARMOR_GUARD_IMPOSSIBLE && (!superArmorOverdrive || !superArmorBlitzBreak)) {
							result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s", "unblockables, but not overdrive unblockables");
							gaveHintThatCantArmorSupers = true;
							if (result != -1) {
								needSubplus = true;
								buf += result;
								bufSize -= result;
							}
							continue;
						}
						result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s", superArmorType.description);
						if (result != -1) {
							needSubplus = true;
							buf += result;
							bufSize -= result;
						}
					}
					if (superArmorObjectAttacck && !superArmorBurst) {
						result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s", "can't armor burst");
						if (result != -1) {
							needSubplus = true;
							buf += result;
							bufSize -= result;
						}
					}
					if (!superArmorGuardImpossible) {
						result = sprintf_s(buf, bufSize, needSubplus ? ", %s%s" : "%s%s",
							"can't armor unblockables",
							guardImpossibleTooltipHelp);
						if (advisedToReadGuardImpossibleTooltip) *advisedToReadGuardImpossibleTooltip = true;
						if (result != -1) {
							needSubplus = true;
							buf += result;
							bufSize -= result;
						}
					}
					// Blitz shield has the overdrive armor flag, but not blitz break armor flag
					// However, all supers by default have the blitz break property
					// So blitz shield having the overdrive armor flag still does not mean it can armor overdrives
					if ((!superArmorOverdrive || !superArmorBlitzBreak) && !gaveHintThatCantArmorSupers) {
						result = sprintf_s(buf, bufSize, needSubplus ? ", %s" : "%s", "can't armor overdrives");
						gaveHintThatCantArmorSupers = true;
						if (result != -1) {
							needSubplus = true;
							buf += result;
							bufSize -= result;
						}
					}
					if (needSubplus) {
						*oldBuf = ' ';
						*(oldBuf + 1) = '(';
						result = sprintf_s(buf, bufSize, ")");
						if (result != -1) {
							buf += result;
							bufSize -= result;
						}
					} else {
						buf -= 2;
						bufSize += 2;
					}
				}
				if (item.type == SUPER_ARMOR && out_thisSuperArmorHasMidHighLowInfo) {
					*out_thisSuperArmorHasMidHighLowInfo = thisSuperArmorHasMidHighLowInfo;
				}
				needPlus = true;
			}
		}
	}
	return buf - origBuf;
}

template<typename FramebarT, typename FrameT>
inline void processRequests(FramebarT* framebar, FrameT& destinationFrame) {
	if (framebar->requestFirstFrame) {
		destinationFrame.isFirst = true;
		framebar->requestFirstFrame = false;
	}
	if (framebar->requestNextHit) {
		destinationFrame.newHit = true;
		framebar->requestNextHit = false;
	}
}

void Framebar::processRequests(FrameBase& destinationFrame) {
	::processRequests<Framebar, Frame>(this, (Frame&)destinationFrame);
}

void PlayerFramebar::processRequests(FrameBase& destinationFrame) {
	PlayerFrame& frame = (PlayerFrame&)destinationFrame;
	::processRequests<PlayerFramebar, PlayerFrame>(this, frame);
	if (!inputs.empty()) {
		bool overflow = false;
		bool newMultipleInputs = PlayerFrame::shoveMoreInputsAtTheStart(frame.prevInput, frame.multipleInputs, frame.input,
				frame.inputs, prevInput,
				inputs, &overflow);
		frame.multipleInputs = newMultipleInputs;
		frame.inputsOverflow |= inputsOverflow | overflow;
		inputs.clear();
	}
	if (!createdProjectiles.empty()) {
		if (!frame.createdProjectiles || frame.createdProjectiles.use_count() != 1) {
			const std::vector<CreatedProjectileStruct>* oldCreatedProjectiles = nullptr;
			if (frame.createdProjectiles) {
				oldCreatedProjectiles = frame.createdProjectiles.get();
			}
			frame.createdProjectiles = new ThreadUnsafeSharedResource<std::vector<CreatedProjectileStruct>>();
			frame.createdProjectiles->reserve(
				(oldCreatedProjectiles ? oldCreatedProjectiles->size() : 0)
				+ createdProjectiles.size()
			);
			frame.createdProjectiles->insert(frame.createdProjectiles->end(), createdProjectiles.begin(), createdProjectiles.end());
			if (oldCreatedProjectiles) {
				frame.createdProjectiles->insert(frame.createdProjectiles->end(), oldCreatedProjectiles->begin(), oldCreatedProjectiles->end());
			}
		} else {
			frame.createdProjectiles->insert(frame.createdProjectiles->begin(), createdProjectiles.begin(), createdProjectiles.end());
		}
		createdProjectiles.clear();
	}
	inputsOverflow = false;
	prevInputCopied = false;
	prevInput = Input{0x0000};
}

void Framebar::processRequests(int destinationPosition) {
	processRequests(frames[destinationPosition]);
}

void PlayerFramebar::processRequests(int destinationPosition) {
	processRequests(frames[destinationPosition]);
}

template<typename FramebarT>
inline void collectRequests(FramebarT* destination, FramebarT& source) {
	destination->requestFirstFrame |= source.requestFirstFrame;
	destination->requestNextHit |= source.requestNextHit;
}

template<typename FramebarT>
inline void cloneRequests(FramebarT* destination, FramebarT& source) {
	destination->requestFirstFrame = source.requestFirstFrame;
	destination->requestNextHit = source.requestNextHit;
}


void Framebar::cloneRequests(FramebarBase& source) {
	::cloneRequests<Framebar>(this, (Framebar&)source);
}

void PlayerFramebar::cloneRequests(FramebarBase& source) {
	PlayerFramebar& cast = (PlayerFramebar&)source;
	::cloneRequests<PlayerFramebar>(this, cast);
	inputs = cast.inputs;
	createdProjectiles = cast.createdProjectiles;
	inputsOverflow = cast.inputsOverflow;
	prevInputCopied = cast.prevInputCopied;
	prevInput = cast.prevInput;
}

void Framebar::collectRequests(FramebarBase& source, bool framebarAdvancedIdleHitstop, const FrameBase& sourceFrame) {
	::collectRequests<Framebar>(this, (Framebar&)source);
}

void PlayerFramebar::collectRequests(FramebarBase& source, bool framebarAdvancedIdleHitstop, const FrameBase& sourceFrame) {
	::collectRequests<PlayerFramebar>(this, (PlayerFramebar&)source);
	if (framebarAdvancedIdleHitstop) {
		const PlayerFrame& frame = (const PlayerFrame&)sourceFrame;
		if (frame.type != FT_NONE && !(frame.multipleInputs && frame.inputs->empty())) {
			bool overflow = false;
			if (frame.multipleInputs) {
				PlayerFrame::shoveMoreInputs(prevInput, inputs, frame.prevInput, *frame.inputs, &overflow);
			} else {
				PlayerFrame::shoveMoreInputs(prevInput, inputs, frame.prevInput, frame.input, &overflow);
			}
			if (overflow || frame.inputsOverflow) inputsOverflow = true;
		}
		if (frame.createdProjectiles && !frame.createdProjectiles->empty()) {
			createdProjectiles.insert(createdProjectiles.end(), frame.createdProjectiles->begin(), frame.createdProjectiles->end());
		}
	}
}

template<typename FramebarT>
inline void clearRequests(FramebarT* framebar) {
	framebar->requestFirstFrame = false;
	framebar->requestNextHit = false;
}

void Framebar::clearRequests() {
	::clearRequests<Framebar>(this);
}

void PlayerFramebar::clearRequests() {
	::clearRequests<PlayerFramebar>(this);
	inputsOverflow = false;
	inputs.clear();
	createdProjectiles.clear();
	prevInputCopied = false;
	prevInput = Input{0x0000};
}

void PlayerInfo::setMoveName(char* destination, Entity ent) {
	if (ent.currentMoveIndex() == -1) {
		memset(destination, 0, 32);
	} else {
		memcpy(destination, ent.currentMove()->name, 32);
	}
}

void PlayerInfo::addActiveFrame(Entity ent, PlayerFramebar& framebar) {
	if (maxHit.currentUse == -1 || maxHit.currentUse > 0 && !(maxHit.maxUse == 1 && hitNumber > 0)) {
		hitNumber = ent.currentHitNum();
	}
	if (actives.count
			&& !actives.data[actives.count - 1].nonActives
			&& hitNumber != actives.prevHitNum
			&& !pawn.isRCFrozen()) {
		framebar.requestNextHit = true;
	}
	actives.addActive(hitNumber);
	if (!hitOnFrame && pawn.hitSomethingOnThisFrame()) {
		hitOnFrame = actives.total();
	}
}

AddedMoveData* PlayerInfo::findMoveByName(const char* name) const {
	return (AddedMoveData*)::findMoveByName((void*)pawn.ent, name, 0);
}

bool PlayerInfo::wasHadGatling(const char* name) const {
	return endScene.wasPlayerHadGatling(index, name);
}

bool PlayerInfo::wasHadWhiffCancel(const char* name) const {
	return endScene.wasPlayerHadWhiffCancel(index, name);
}

bool PlayerInfo::wasHadGatlings() const {
	return endScene.wasPlayerHadGatlings(index);
}

bool PlayerInfo::wasHadWhiffCancels() const {
	return endScene.wasPlayerHadWhiffCancels(index);
}

MilliaInfo PlayerInfo::canProgramSecretGarden() const {
	for (int i = 2; i < entityList.count; ++i) {
		Entity e = entityList.list[i];
		if (e
				&& e.isActive()
				&& e.team() == index
				&& strcmp(e.animationName(), "SecretGardenBall") == 0) {
			MilliaInfo response;
			response.canProgramSecretGarden = e.mem50();
			response.SGInputs = e.mem51();
			response.SGInputsMax = 4;
			return response;
		}
	}
	return { false, 0, 4 };
}

FramebarBase& PlayerFramebars::getMain() { return main; }
FramebarBase& PlayerFramebars::getHitstop() { return hitstop; }
FramebarBase& PlayerFramebars::getIdle() { return idle; }
FramebarBase& PlayerFramebars::getIdleHitstop() { return idleHitstop; }

FramebarBase& ProjectileFramebar::getMain() { return main; }
FramebarBase& ProjectileFramebar::getHitstop() { return hitstop; }
FramebarBase& ProjectileFramebar::getIdle() { return idle; }
FramebarBase& ProjectileFramebar::getIdleHitstop() { return idleHitstop; }

FramebarBase& CombinedProjectileFramebar::getMain() { return main; }
FramebarBase& CombinedProjectileFramebar::getHitstop() { return main; }
FramebarBase& CombinedProjectileFramebar::getIdle() { return main; }
FramebarBase& CombinedProjectileFramebar::getIdleHitstop() { return main; }

const FramebarBase& PlayerFramebars::getMain() const { return main; }
const FramebarBase& PlayerFramebars::getHitstop() const { return hitstop; }
const FramebarBase& PlayerFramebars::getIdle() const { return idle; }
const FramebarBase& PlayerFramebars::getIdleHitstop() const { return idleHitstop; }

const FramebarBase& ProjectileFramebar::getMain() const { return main; }
const FramebarBase& ProjectileFramebar::getHitstop() const { return hitstop; }
const FramebarBase& ProjectileFramebar::getIdle() const { return idle; }
const FramebarBase& ProjectileFramebar::getIdleHitstop() const { return idleHitstop; }

const FramebarBase& CombinedProjectileFramebar::getMain() const { return main; }
const FramebarBase& CombinedProjectileFramebar::getHitstop() const { return main; }
const FramebarBase& CombinedProjectileFramebar::getIdle() const { return main; }
const FramebarBase& CombinedProjectileFramebar::getIdleHitstop() const { return main; }

void Framebar::clear() { memset((BYTE*)this + sizeof(uintptr_t), 0, sizeof *this - sizeof(uintptr_t)); }

void PlayerFramebar::clear() {
	for (int i = 0; i < _countof(frames); ++i) {
		frames[i].clear();
	}
}

void PlayerFrame::clear() {
	DWORD pos = offsetof(PlayerFrame, cancels);
	memset(this, 0, pos);
	cancels = nullptr;
	pos += sizeof cancels;
	if (multipleInputs) {
		if (inputs.use_count() == 1) {
			inputs->clear();
		} else {
			inputs = nullptr;
		}
	}
	pos += sizeof inputs;
	if (createdProjectiles && !createdProjectiles->empty()) {
		if (createdProjectiles.use_count() == 1) {
			createdProjectiles->clear();
		} else {
			createdProjectiles = nullptr;
		}
	}
	pos += sizeof createdProjectiles;
	memset((BYTE*)this + pos, 0, sizeof *this - pos);
}

void Framebar::catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) {
	Framebar& cast = (Framebar&)source;
	for (int i = 1; i <= framesToCatchUpFor; ++i) {
		int ind = (destinationStartingPosition + i) % _countof(Framebar::frames);
		soakUpIntoPreFrame(frames[ind]);
		frames[ind] = cast[ind];
	}
	cloneRequests(source);
}

void PlayerFramebar::catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) {
	PlayerFramebar& cast = (PlayerFramebar&) source;
	int ind = EntityFramebar::posPlusOne(destinationStartingPosition);
	for (int i = 1; i <= framesToCatchUpFor; ++i) {
		soakUpIntoPreFrame(frames[ind]);
		frames[ind] = cast[ind];
		frames[ind].cancels = cast[ind].cancels;
		EntityFramebar::incrementPos(ind);
	}
	cloneRequests(source);
}

FrameBase& Framebar::getFrame(int index) { return (FrameBase&)frames[index]; }
FrameBase& PlayerFramebar::getFrame(int index) { return (FrameBase&)frames[index]; }
const FrameBase& Framebar::getFrame(int index) const { return (const FrameBase&)frames[index]; }
const FrameBase& PlayerFramebar::getFrame(int index) const { return (const FrameBase&)frames[index]; }

template<typename T>
bool lastNFramesCompletelyEmpty(const T* framebar, int framebarPosition, int n) {
	iterateFramesBegin(framebarPosition, n)
	if (!frameTypeDiscardable(framebar->frames[iterateFrames_pos].type)) return false;
	iterateFramesEnd
	return true;
}

bool Framebar::lastNFramesCompletelyEmpty(int framebarPosition, int n) const {
	return ::lastNFramesCompletelyEmpty<Framebar>(this, framebarPosition, n);
}

bool PlayerFramebar::lastNFramesCompletelyEmpty(int framebarPosition, int n) const {
	return ::lastNFramesCompletelyEmpty<PlayerFramebar>(this, framebarPosition, n);
}

bool Framebar::lastNFramesHaveMarker(int framebarPosition, int n) const {
	iterateFramesBegin(framebarPosition, n)
	const Frame& frame = frames[iterateFrames_pos];
	if (frame.type == FT_NONE) return false;
	if (frame.marker) return true;
	iterateFramesEnd
	return false;
}

void PlayerFramebar::clearCancels() {
	for (int i = 0; i < _countof(frames); ++i) {
		frames[i].cancels = nullptr;
	}
}

void PlayerFramebar::clearCancels(int index) {
	frames[index].cancels = nullptr;
}

void PlayerFramebar::copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const {
	(PlayerFrame&)destFrame = (const PlayerFrame&)srcFrame;
}

void Framebar::copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const {
	(Frame&)destFrame = (const Frame&)srcFrame;
}

void PlayerFramebar::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(PlayerFrame&)destFrame = (PlayerFrame&&)srcFrame;
}

void Framebar::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(Frame&)destFrame = (Frame&&)srcFrame;
}

void PlayerFramebars::copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const {
	(PlayerFrame&)destFrame = (const PlayerFrame&)srcFrame;
}

void ProjectileFramebar::copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const {
	(Frame&)destFrame = (const Frame&)srcFrame;
}

void CombinedProjectileFramebar::copyFrame(FrameBase& destFrame, const FrameBase& srcFrame) const {
	(Frame&)destFrame = (const Frame&)srcFrame;
}

void PlayerFramebars::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(PlayerFrame&)destFrame = (PlayerFrame&&)srcFrame;
}

void ProjectileFramebar::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(Frame&)destFrame = (Frame&&)srcFrame;
}

void CombinedProjectileFramebar::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(Frame&)destFrame = (Frame&&)srcFrame;
}

template<typename FrameT>
inline void copyActiveDuringSuperfreeze(FrameT& destFrame, const FrameT& srcFrame) {
	if (!(srcFrame.type != FT_NONE && (srcFrame.activeDuringSuperfreeze || srcFrame.hitConnected))) return;
	if (destFrame.type == FT_NONE) destFrame.type = srcFrame.type;
	destFrame.activeDuringSuperfreeze |= srcFrame.activeDuringSuperfreeze;
	destFrame.hitConnected |= srcFrame.hitConnected;
}

void copyActiveDuringSuperfreezeProjectile(Frame& destFrame, const Frame& srcFrame) {
	copyActiveDuringSuperfreeze(destFrame, srcFrame);
	if (destFrame.title.text == nullptr || srcFrame.hitConnected) {
		destFrame.title = srcFrame.title;
		destFrame.animName = srcFrame.animName;
	}
}

void PlayerFramebars::copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const {
	::copyActiveDuringSuperfreeze((PlayerFrame&)destFrame, (const PlayerFrame&)srcFrame);
}

void ProjectileFramebar::copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const {
	copyActiveDuringSuperfreezeProjectile((Frame&)destFrame, (const Frame&)srcFrame);
}

void CombinedProjectileFramebar::copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const {
	copyActiveDuringSuperfreezeProjectile((Frame&)destFrame, (const Frame&)srcFrame);
}

void MaxHitInfo::fill(Entity ent, int currentHitNum) {
	int maxHit = ent.maxHit();
	int remainingHits = -1;
	int numOfHits = ent.numberOfHits();
	if (!ent.isPawn() && ent.destroyOnBlockOrArmor() && ent.destroyOnHitPlayer() && !ent.notDestroyOnMaxNumOfHits()) {
		remainingHits = numOfHits - ent.numberOfHitsTaken() + (ent.hitSomethingOnThisFrame() ? 1 : 0);
	}
	if (maxHit == -1 && remainingHits == -1) {
		max = -1;
		current = -1;
		maxUse = -1;
		currentUse = -1;
		return;
	} else if (maxHit != -1) {
		int maxHitMax = maxHit + (ent.hitSomethingOnThisFrame() ? 1 : 0);
		if (max == -1) {
			max = currentHitNum == 0 ? maxHitMax : maxHitMax + currentHitNum - 1;
		} else if (maxHitMax > current) {
			max += maxHitMax - current;
		}
		current = maxHitMax;
	} else if (remainingHits != -1) {
		max = numOfHits;
		maxUse = numOfHits;
		current = remainingHits;
		currentUse = remainingHits;
		return;
	}
	
	maxUse = max;
	currentUse = current;
	if (remainingHits != -1) {
		if (max != -1 && numOfHits < max || max == -1) {
			maxUse = numOfHits;
		}
		if (current != -1 && remainingHits < current || current == -1) {
			currentUse = remainingHits;
		}
	}
}

bool CombinedProjectileFramebar::canBeCombined(const Framebar& source, int sourceId) const {
	for (int i = 0; i < (int)_countof(Framebar::frames); ++i) {
		if (!frameTypeDiscardable(main[i].type) && !frameTypeDiscardable(source[i].type)
				&& (sources[i] == nullptr || sourceId != sources[i]->idForCombinedFramebar())) return false;
	}
	return true;
}

void CombinedProjectileFramebar::combineFramebar(int framebarPosition, Framebar& source, const ProjectileFramebar* dad) {
	const ProjectileFramebar* lastConnectedSource = nullptr;
	const NamePair* lastConnectedAnimName = nullptr;
	
	int posNext = framebarPosition == _countof(Framebar::frames) - 1
		? 0
		: framebarPosition + 1;
	int pos;
	
	for (int i = 0; i < (int)_countof(Framebar::frames); ++i) {
		
		pos = posNext;
		posNext = posNext == _countof(Framebar::frames) - 1
			? 0
			: posNext + 1;
		
		Frame& sf = source[pos];
		if (sf.type != FT_NONE) {
			Frame& df = main[pos];
			
			int sfLvl = determineFrameLevel(sf.type);
			int dfLvl = determineFrameLevel(df.type);
			bool repeatLast = !df.hitConnected && !sf.hitConnected && df.type == FT_IDLE_PROJECTILE && sf.type == FT_IDLE_PROJECTILE;
			bool sWin = sfLvl > dfLvl || sfLvl == dfLvl && !(df.hitConnected && !sf.hitConnected);
			if (repeatLast && lastConnectedSource) {
				sources[pos] = lastConnectedSource;
			} else if (sWin) {
				sources[pos] = dad;
			}
			if (repeatLast && lastConnectedAnimName) {
				df.animName = lastConnectedAnimName;
			} else if (sWin) {
				df.animName = sf.animName;
			}
			if (sWin) {
				df.type = sf.type;
			}
			if (!df.hitstopConflict) {
				if (df.hitstop != sf.hitstop || df.hitstopMax != sf.hitstopMax) {
					if (df.hitstop || df.hitstopMax) {
						df.hitstopConflict = true;
						df.hitstop = 0;
						df.hitstopMax = 0;
					} else {
						df.hitstop = sf.hitstop;
						df.hitstopMax = sf.hitstopMax;
					}
				}
			}
			df.hitConnected |= sf.hitConnected;
			if (df.hitConnected) {
				lastConnectedSource = sources[pos];
				lastConnectedAnimName = df.animName;
			} else {
				if (sources[pos] != lastConnectedSource) lastConnectedSource = nullptr;
				if (df.animName != lastConnectedAnimName) lastConnectedAnimName = nullptr;
			}
			df.newHit |= sf.newHit;
			df.rcSlowdown = max(df.rcSlowdown, sf.rcSlowdown);
			df.rcSlowdownMax = max(df.rcSlowdownMax, sf.rcSlowdownMax);
			df.activeDuringSuperfreeze |= sf.activeDuringSuperfreeze;
			df.powerup |= sf.powerup;
			df.marker |= sf.marker;
			df.charSpecific1 = sf.charSpecific1;
			df.charSpecific2 = sf.charSpecific2;
			df.title = sf.title;  // will get corrected in determineName
			sf.next = df.next;
			df.next = &sf;
		}
	}
	if (source.preFrameLength) {
		if (source.preFrame != main.preFrame) {
			if (determineFrameLevel(source.preFrame) >= determineFrameLevel(main.preFrame)) {
				main.preFrame = source.preFrame;
				main.preFrameLength = source.preFrameLength;
			}
		} else if (source.preFrameLength > main.preFrameLength) {
			main.preFrameLength = source.preFrameLength;
		}
	}
	if (source.preFrameMappedLength) {
		if (source.preFrameMapped != main.preFrameMapped) {
			if (determineFrameLevel(source.preFrameMapped) >= determineFrameLevel(main.preFrameMapped)) {
				main.preFrameMapped = source.preFrameMapped;
				main.preFrameMappedLength = source.preFrameMappedLength;
			}
		} else if (source.preFrameMappedLength > main.preFrameMappedLength) {
			main.preFrameMappedLength = source.preFrameMappedLength;
		}
	}
	if (source.preFrameMappedNoIdleLength) {
		if (source.preFrameMappedNoIdle != main.preFrameMappedNoIdle) {
			if (determineFrameLevel(source.preFrameMappedNoIdle) >= determineFrameLevel(main.preFrameMappedNoIdle)) {
				main.preFrameMappedNoIdle = source.preFrameMappedNoIdle;
				main.preFrameMappedNoIdleLength = source.preFrameMappedNoIdleLength;
			}
		} else if (source.preFrameMappedNoIdleLength > main.preFrameMappedNoIdleLength) {
			main.preFrameMappedNoIdleLength = source.preFrameMappedNoIdleLength;
		}
	}
}

void CombinedProjectileFramebar::determineName(int framebarPosition, bool isHitstop) {
	int posNext = framebarPosition == _countof(Framebar::frames) - 1
		? 0
		: framebarPosition + 1;
	int pos;
	const FramebarTitle* title = nullptr;
	bool charSpecific1 = false;
	bool charSpecific2 = false;
	
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		
		pos = posNext;
		posNext = posNext == _countof(Framebar::frames) - 1
			? 0
			: posNext + 1;
		
		const ProjectileFramebar* source = sources[pos];
		if (source) {
			const Framebar& framebar = isHitstop ? source->hitstop : source->main;
			moveFramebarId = source->moveFramebarId;  // why do I need this on a combined framebar?
			const Frame& frame = framebar[pos];
			title = &frame.title;
			charSpecific1 = frame.charSpecific1;
			charSpecific2 = frame.charSpecific2;
		}
		
		if (title) {
			Frame& frame = main[pos];
			frame.title = *title;
			frame.charSpecific1 = charSpecific1;
			frame.charSpecific2 = charSpecific2;
		}
	}
}

bool PlayerCancelInfo::cancelsEqual(const PlayerCancelInfo& other) const {
	if (gatlings.size() != other.gatlings.size()
			|| whiffCancels.size() != other.whiffCancels.size()
			|| enableJumpCancel != other.enableJumpCancel
			|| enableSpecialCancel != other.enableSpecialCancel
			|| enableSpecials != other.enableSpecials
			|| airborne != other.airborne) {
		return false;
	}
	{
		auto it = gatlings.begin();
		auto otherIt = other.gatlings.begin();
		for (; it != gatlings.end(); ) {
			if (*it != *otherIt) return false;
			++it;
			++otherIt;
		}
	}
	{
		auto it = whiffCancels.begin();
		auto otherIt = other.whiffCancels.begin();
		for (; it != whiffCancels.end(); ) {
			if (*it != *otherIt) return false;
			++it;
			++otherIt;
		}
	}
	return true;
}

void PlayerCancelInfo::clear() {
	start = 0;
	end = 0;
	gatlings.clear();
	whiffCancels.clear();
	enableJumpCancel = false;
	enableSpecialCancel = false;
	enableSpecials = false;
	hitAlreadyHappened = false;
}

void PlayerInfo::appendPlayerCancelInfo(const PlayerCancelInfo& playerCancel) {
	PlayerCancelInfo* lastEmpty = nullptr;
	if (!cancelsCount) {
		cancels[0] = playerCancel;
		cancelsCount = 1;
		return;
	}
	PlayerCancelInfo& lastCancel = cancels[cancelsCount - 1];
	if (playerCancel.start == lastCancel.end + 1 && lastCancel.cancelsEqual(playerCancel)) {
		lastCancel.end = playerCancel.end;
		return;
	}
	if (cancelsCount == _countof(cancels)) {
		for (int i = 0; i < _countof(cancels) - 1; ++i) {
			cancels[i] = cancels[i + 1];  // ring buffer maybe? Naah. When will this ever happen?
		}
		cancels[_countof(cancels) - 1] = playerCancel;
		cancelsOverflow = true;
		return;
	}
	cancels[cancelsCount++] = playerCancel;
}

void PlayerInfo::appendPlayerCancelInfo(PlayerCancelInfo&& playerCancel) {
	PlayerCancelInfo* lastEmpty = nullptr;
	if (!cancelsCount) {
		cancels[0] = playerCancel;
		cancelsCount = 1;
		return;
	}
	PlayerCancelInfo& lastCancel = cancels[cancelsCount - 1];
	if (playerCancel.start == lastCancel.end + 1 && lastCancel.cancelsEqual(playerCancel)) {
		lastCancel.end = playerCancel.end;
		return;
	}
	if (cancelsCount == _countof(cancels)) {
		for (int i = 0; i < _countof(cancels) - 1; ++i) {
			cancels[i] = cancels[i + 1];  // ring buffer maybe? Naah. When will this ever happen?
		}
		cancels[_countof(cancels) - 1] = playerCancel;
		cancelsOverflow = true;
		return;
	}
	cancels[cancelsCount++] = playerCancel;
}

bool PlayerCancelInfo::isCompletelyEmpty() const {
	return gatlings.empty() && whiffCancels.empty() && !enableJumpCancel && !enableSpecialCancel && !enableSpecials;
}

void PlayerInfo::determineMoveNameAndSlangName(const NamePair** name) {
	determineMoveNameAndSlangName(moveNonEmpty ? &move : nullptr, idle, *this, name);
}

static std::unique_ptr<PlayerInfo> dummyPlayer = nullptr;
void PlayerInfo::determineMoveNameAndSlangName(Entity pawn, const NamePair** name) {
	MoveInfo moveInfo;
	bool moveNonEmpty = moves.getInfo(moveInfo,
		pawn.characterType(),
		pawn.currentMoveIndex() == -1 ? nullptr : pawn.currentMove()->name,
		pawn.animationName(),
		false);
	bool idle = false;
	if (!dummyPlayer) {
		dummyPlayer = std::make_unique<PlayerInfo>();
	}
	dummyPlayer->pawn = pawn;
	dummyPlayer->wasEnableWhiffCancels = pawn.enableWhiffCancels();
	dummyPlayer->wasForceDisableFlags = pawn.forceDisableFlags();
	if (moveNonEmpty) {
		idle = moveInfo.isIdle(*dummyPlayer);
	}
	determineMoveNameAndSlangName(moveNonEmpty ? &moveInfo : nullptr, idle, *dummyPlayer, name);
}

bool PlayerInfo::determineMove(Entity pawn, MoveInfo* destination) {
	*destination = MoveInfo();
	bool moveNonEmpty = moves.getInfo(*destination,
		pawn.characterType(),
		pawn.currentMoveIndex() == -1 ? nullptr : pawn.currentMove()->name,
		pawn.animationName(),
		false);
	return moveNonEmpty;
}

void PlayerInfo::determineMoveNameAndSlangName(const MoveInfo* move, bool idle, PlayerInfo& pawn, const NamePair** name) {
	if (name) *name = nullptr;
	if (move) {
		if (name) *name = move->getDisplayName(pawn);
	}
	if (name && !*name) {
		int moveIndex = pawn.pawn.currentMoveIndex();
		if (moveIndex == -1) {
			*name = NamePairManager::getPair(asInstr(pawn.pawn.bbscrCurrentFunc(), beginState)->name);
		} else {
			const AddedMoveData* actualMove = pawn.pawn.movesBase() + moveIndex;
			*name = NamePairManager::getPair(actualMove->name);
		}
	}
}

void ProjectileInfo::determineMoveNameAndSlangName(const NamePair** name) const {
	determineMoveNameAndSlangName(moveNonEmpty ? &move : nullptr, ptr, name);
}

void ProjectileInfo::determineMoveNameAndSlangName(Entity ptr, const NamePair** name, const char** framebarNameFull) {
	entityList.populate();
	MoveInfo moveInfo;
	bool moveNonEmpty = moves.getInfo(moveInfo,
		entityList.slots[ptr.team()].characterType(),
		nullptr,
		ptr.animationName(),
		true);
	determineMoveNameAndSlangName(moveNonEmpty ? &moveInfo : nullptr, ptr, name);
	if (framebarNameFull) *framebarNameFull = moveInfo.framebarNameFull;
}

void ProjectileInfo::determineMoveNameAndSlangName(const MoveInfo* move, Entity ptr, const NamePair** name) {
	if (name) *name = nullptr;
	if (move) {
		if (name) {
			if (move->framebarNameSelector && ptr) {
				*name = move->framebarNameSelector(ptr);
			} else if (move->framebarNameUncombinedSelector && ptr) {
				*name = move->framebarNameUncombinedSelector(ptr);
			} else if (move->framebarNameUncombined) {
				*name = move->framebarNameUncombined;
			} else if (move->framebarName) {
				*name = move->framebarName;
			}
		}
	} else if (!ptr || !ptr.bbscrCurrentFunc()) {
		if (name) *name = nullptr;
	} else if (name) {
		*name = NamePairManager::getPair(asInstr(ptr.bbscrCurrentFunc(), beginState)->name);
	}
}

void PlayerInfo::onAnimReset() {
	prevStartups.clear();
	
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) fieldName.clear();
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
	
	cancelsTimer = 0;
	cancelsCount = 0;
	cancelsOverflow = false;
}

void PlayerInfo::removeNonStancePrevStartups() {
	int amountToRemove = 0;
	int countToRemove = 0;
	for (int i = 0; i < prevStartups.count; ++i) {
		if (!prevStartups[i].partOfStance) {
			++countToRemove;
			amountToRemove += prevStartups[i].startup;
		}
	}
	if (!countToRemove) return;
	if (countToRemove == prevStartups.count) {
		prevStartups.clear();
	} else {
		memmove(prevStartups.startups, prevStartups.startups + countToRemove, sizeof PrevStartupsInfoElem * (prevStartups.count - countToRemove));
		prevStartups.count -= countToRemove;
	}
	countToRemove = 0;
	for (int i = 0; i < cancelsCount; ++i) {
		PlayerCancelInfo& info = cancels[i];
		if (info.end <= amountToRemove) {
			++countToRemove;
		} else {
			if (info.start - amountToRemove >= 1) {
				info.start -= amountToRemove;
			} else {
				info.start = 1;
			}
			info.end -= amountToRemove;
		}
	}
	if (countToRemove) {
		if (countToRemove == cancelsCount) {
			cancelsCount = 0;
		} else {
			for (int i = 0; i < cancelsCount - countToRemove; ++i) {
				cancels[i] = cancels[i + countToRemove];
			}
			cancelsCount -= countToRemove;
		}
		cancelsOverflow = false;
	}
	cancelsTimer -= amountToRemove;
	
	#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) fieldName.removeFirstNFrames(amountToRemove);
	INVUL_TYPES_TABLE
	#undef INVUL_TYPES_EXEC
	
}

void InvulData::removeFirstNFrames(int n) {
	if (start > n) {
		start -= n;
		return;
	}
	int amountToEat = n - start + 1;
	start = 1;
	int i;
	for (i = 0; i < frames.count; ++i) {
		ActiveData& data = frames[i];
		if (data.actives >= amountToEat) {
			data.actives -= amountToEat;
			amountToEat = 0;
			break;
		} else {
			amountToEat -= data.actives;
			data.actives = 0;
		}
		if (data.nonActives >= amountToEat) {
			data.nonActives -= amountToEat;
			amountToEat = 0;
			break;
		} else {
			amountToEat -= data.nonActives;
			data.nonActives = 0;
		}
	}
	if (i == frames.count) {
		clear();
		return;
	}
	ActiveData& data = frames[i];
	if (!data.actives) {
		start += data.nonActives;
		++i;
	}
	if (i == frames.count) {
		clear();
		return;
	}
	memmove(frames.data, frames.data + i, sizeof ActiveData * (frames.count - i));
	frames.count -= i;
}

void PlayerInfo::fillInMove() {
	moveNonEmpty = moves.getInfo(move,
		charType,
		pawn.currentMoveIndex() == -1 ? nullptr : pawn.currentMove()->name,
		pawn.animationName(),
		false);
}

void ProjectileInfo::fillInMove() {
	entityList.populate();
	moveNonEmpty = moves.getInfo(move,
		entityList.slots[team].characterType(),
		nullptr,
		animName,
		true);
}

void PlayerInfo::calculateSlow(int valueElapsed, int valueRemaining, int slowRemaining, int* result, int* resultMax, int *newSlowRemaining) {
	if (valueElapsed != 0 && valueRemaining) {
		--valueElapsed;
	}
	if (!slowRemaining || !valueRemaining) {
		*result = valueRemaining;
		*resultMax = valueElapsed + valueRemaining;
		*newSlowRemaining = slowRemaining;
		return;
	}
	/*
	| slowRemaining | valueElapsed | valueRemaining | didn't advance on this frame |
	| ---------------------------------------------------------------------------- |
	| 8             | 0            | 3              | 0                            |
	| 7             | 0            | 3              | 1                            |
	| 6             | 1            | 2              | 0                            |
	| 5             | 1            | 2              | 1                            |
	| 4             | 2            | 1              | 0                            |
	| 3             | 2            | 1              | 1                            |
	| 2             | 3            | 0              | 0                            |
	| 1             |              |                | 1                            |
	| 0             |              |                | 0                            |
	| ---------------------------------------------------------------------------- |
	*/
	bool slowIsOdd = slowRemaining & 1;
	int extraNewFrames = (slowRemaining >> 1) + slowIsOdd;
	if (extraNewFrames > valueRemaining) extraNewFrames = valueRemaining;
	int rm = valueElapsed + valueRemaining + extraNewFrames - slowIsOdd;
	int r = rm - valueElapsed;
	*result = r;
	*resultMax = rm;
	*newSlowRemaining = slowRemaining - r;
}

bool ProjectileInfo::hitConnectedForFramebar() const {
	// I don't understand why I have this safeguard in the first place. Don't trust my own hit registration?
	if (ptr && !ptr.dealtAttack()->attackMultiHit()) {
		return ptr.hitSomethingOnThisFrame()
			// unfortunately, in order to show projectiles activating things like Elphelt Berry Pine and Dizzy Bubble,
			// have to add this
			|| landedHit;
	} else {
		return landedHit || clashedOnThisFrame;
	}
}

// returns the new value of destination.multipleInputs
bool PlayerFrame::shoveMoreInputsAtTheStart(Input& prevInput, bool destinationMultipleInputs, Input& destinationInput,
			ThreadUnsafeSharedPtr<std::vector<Input>>& destination, const Input& sourcePrevInput,
			const std::vector<Input>& source, bool* overflow) {
	if (source.empty()) return destinationMultipleInputs;
	size_t oldSize = 1;
	if (destinationMultipleInputs) {
		oldSize = destination->size();
	}
	if (oldSize == 80) {
		if (!source.empty() && overflow) *overflow = true;
		return true;
	}
	size_t totalSize = oldSize + source.size();
	if (totalSize > 80) {
		if (overflow) *overflow = true;
		
		if (!destination || destination.use_count() != 1) {
			destination = new ThreadUnsafeSharedResource<std::vector<Input>>();
		}
		destination->resize(80);
		if (destinationMultipleInputs) {
			int framesToShove = 80 - (int)oldSize;
			Input* destinationData = destination->data();
			memmove(destinationData + framesToShove, destinationData, oldSize * sizeof Input);
			memcpy(destinationData, source.data() + source.size() - framesToShove, framesToShove * sizeof Input);
			prevInput = source[source.size() - framesToShove - 1];
		} else {
			(*destination)[79] = destinationInput;
			memcpy(destination->data(), source.data() + 1, 79 * sizeof Input);
			prevInput = source.front();
		}
		return true;
	} else if (totalSize == 1) {
		destinationInput = source[0];
		return false;
	} else {
		if (!destination || destination.use_count() != 1) {
			destination = new ThreadUnsafeSharedResource<std::vector<Input>>();
		}
		if (destinationMultipleInputs) {
			destination->insert(destination->begin(), source.begin(), source.end());
		} else {
			destination->resize(totalSize);
			destination->back() = destinationInput;
			memcpy(destination->data(), source.data(), source.size() * sizeof Input);
		}
		prevInput = sourcePrevInput;
		return true;
	}
}

void PlayerFrame::shoveMoreInputs(Input& prevInput, std::vector<Input>& destination, const Input& sourcePrevInput,
		const std::vector<Input>& source, bool* overflow) {
	if (source.empty()) return;
	size_t oldSize = destination.size();
	size_t totalSize = oldSize + source.size();
	if (totalSize > 80) {
		if (overflow) *overflow = true;
		size_t indexToTakeFrom = totalSize - 80 - 1;  // in the would-be combined, size-unrestrained array of destination+source where destination's elements go first and then source's
		prevInput = destination[indexToTakeFrom];
		if (indexToTakeFrom + 1 < oldSize) {
			size_t elementsMoved = oldSize - indexToTakeFrom - 1;
			memmove(destination.data(), destination.data() + indexToTakeFrom + 1, elementsMoved * sizeof Input);
			if (oldSize != 80U) destination.resize(80);
			memcpy(destination.data() + elementsMoved, source.data(), source.size() * sizeof Input);
		} else {
			destination = source;
		}
	} else {
		if (destination.empty()) prevInput = sourcePrevInput;
		destination.insert(destination.end(), source.begin(), source.end());
	}
}

void PlayerFrame::shoveMoreInputs(Input& prevInput, std::vector<Input>& destination, const Input& sourcePrevInput,
		const Input& sourceInput, bool* overflow) {
	size_t oldSize = destination.size();
	if (oldSize == 80) {
		if (overflow) *overflow = true;
		prevInput = destination.front();
		memmove(destination.data(), destination.data() + 1, 79 * sizeof Input);
		destination[79] = sourceInput;
	} else {
		if (destination.empty()) prevInput = sourcePrevInput;
		destination.push_back(sourceInput);
	}
}

void PlayerInfo::getInputs(const InputRingBuffer* ringBuffer, bool isTheFirstFrameInTheMatch) {
	if (isTheFirstFrameInTheMatch) {
		inputs.reserve(80);
		int inputsCount = ringBuffer->calculateLength();
		int index = ringBuffer->index;
		
		int sumFramesHeld = 0;
		bool overflow = false;
		int i;
		for (i = 0; i < inputsCount; ++i) {
			Input input = ringBuffer->inputs[index];
			// framesHeld won't be 0 because we did a check for that in the call to ringBuffer->calculateLength()
			const int sumFramesHeldNew = sumFramesHeld + ringBuffer->framesHeld[index];
			if (sumFramesHeldNew > 80) {
				overflow = true;
				break;
			}
			sumFramesHeld = sumFramesHeldNew;
			if (index == 0) index = 29;
			else --index;
		}
		
		if (!overflow) {
			
			index = ringBuffer->index - inputsCount + 1;
			if (index < 0) index += 30;
			
			for (int i = 0; i < inputsCount; ++i) {
				Input input = ringBuffer->inputs[index];
				int framesHeld = ringBuffer->framesHeld[index];
				// framesHeld won't be 0 because we did a check for that in the call to ringBuffer->calculateLength()
				for (int j = 0; j < framesHeld; ++j) {
					inputs.push_back(input);
				}
				if (index == 29) index = 0;
				else ++index;
			}
		} else {
			
			if (sumFramesHeld != 80) {
				int framesHeldLimited = 80 - sumFramesHeld;
				Input input = ringBuffer->inputs[index];
				for (int j = 0; j < framesHeldLimited; ++j) {
					inputs.push_back(input);
				}
			}
			
			if (i != 0) {
				if (index == 29) index = 0;
				else ++index;
				--i;
				
				for (; i >= 0; --i) {
					Input input = ringBuffer->inputs[index];
					int framesHeld = ringBuffer->framesHeld[index];
					// framesHeld won't be 0 because we did a check for that in the call to ringBuffer->calculateLength()
					for (int j = 0; j < framesHeld; ++j) {
						inputs.push_back(input);
					}
					if (index == 29) index = 0;
					else ++index;
				}
			}
			
		}
	} else if (inputs.size() < 80) {
		inputs.push_back(ringBuffer->inputs[ringBuffer->index]);
	} else {
		memmove(inputs.data(), inputs.data() + 1, 79 * sizeof Input);
		inputs[79] = ringBuffer->inputs[ringBuffer->index];
		inputsOverflow = true;
	}
}

void PlayerInfo::fillInPlayervalSetter(int playervalNum) {
	if (playervalSetterOffset) return;
	BYTE* func = pawn.bbscrCurrentFunc();
	if (!func) return;
	BYTE* instr = moves.skipInstr(func);
	InstrType type = moves.instrType(instr);
	bool found = false;
	bool foundSpriteEnd = false;
	while (type != instr_endState) {
		if (!found
				&& type == instr_storeValue
				&& asInstr(instr, storeValue)->dest == (BBScrVariable)(BBSCRVAR_PLAYERVAL_0 + playervalNum)
				&& asInstr(instr, storeValue)->src == BBSCRTAG_VALUE) {
			found = true;
		} else if (found && type == instr_sprite || foundSpriteEnd) {
			playervalSetterOffset = instr - func;
			return;
		} else if (found && type == instr_spriteEnd) {
			foundSpriteEnd = true;
		}
		instr = moves.skipInstr(instr);
		type = moves.instrType(instr);
	}
	if (found) {
		playervalSetterOffset = instr - func;
	} else {
		playervalSetterOffset = -1;  // rev1. For example, in Rev1, Slayer doesn't have the blood infused buff
	}
}

void Framebar::modifyFrame(int pos, DWORD aswEngineTick, FrameType newType) {
	int ind = pos;
	for (int i = 0; i < _countof(Framebar::frames); ++i) {
		Frame& f = frames[ind];
		if (f.type == FT_NONE) break;
		if (f.aswEngineTick == aswEngineTick) {
			f.type = newType;
			break;
		}
		if (f.aswEngineTick < aswEngineTick) break;
		--ind;
		if (ind < 0) ind = _countof(Framebar::frames) - 1;
	}
}

int PlayerInfo::getElpheltRifle_AimMem46() const {
	if (charType == CHARACTER_TYPE_ELPHELT && strncmp(anim, "Rifle_", 6) == 0) {
		Entity aim = nullptr;
		for (int i = 2; i < entityList.count; ++i) {
			Entity p = entityList.list[i];
			if (p.isActive() && p.team() == index && strcmp(p.animationName(), "Rifle_Aim") == 0) {
				aim = p;
				break;
			}
		}
		
		if (!aim || !aim.bbscrCurrentFunc()) return 0;
		
		int mem45 = aim.mem45();
		if (!mem45) return 0;
		
		int mem46 = aim.mem46();
		if (mem46) return 1;
		
		moves.fillElpheltRifleFireStartup(pawn);
		moves.fillElpheltRifleFirePowerupStartup(aim.bbscrCurrentFunc());
		if (!moves.elpheltRifleFirePowerupStartup || !moves.elpheltRifleFireStartup) return 0;
		
		bool stopLinked = aim.stopLinkObject() == pawn;
		bool frozen = 0;
		int slowdown = 0;
		
		
		// we're not in superfreeze
		
		// Rev2 links stop of aim to player. Rev1 doesn't
		if (stopLinked) {
			slowdown = rcSlowedDownCounter;
		} else {
			ProjectileInfo& aimProjectile = endScene.findProjectile(aim);
			if (aimProjectile.ptr) {
				slowdown = aimProjectile.rcSlowedDownCounter;
			}
		}
		
		int timeLeft = INT_MAX;
		if (strcmp(aim.gotoLabelRequests(), "Aim_Super") == 0) {
			timeLeft = 1;
		} else {
			timeLeft = moves.elpheltRifleFirePowerupStartup - mem45
				+ 1;  // account for gotoLabelRequests
		}
		
		int attackStartupWithSlow = 0;
		int unused;
		calculateSlow(0,
			moves.elpheltRifleFireStartup - (
				strcmp(anim, "Rifle_Fire") == 0 ? animFrame : 0
			),
			rcSlowedDownCounter,
			&attackStartupWithSlow,
			&unused,
			&unused);
		
		int timeLeftWithSlow = 0;
		calculateSlow(0,
			timeLeft,
			slowdown,
			&timeLeftWithSlow,
			&unused,
			&unused);
		if (timeLeftWithSlow > 0) ++timeLeftWithSlow;  // player bbscript runs before projectile bbscript,
		// which means player can fire and send CUSTOM_SIGNAL_0 and deactivate the projectile before it gets a chance to run storeValue: MEM(46), Val(1)
		
		if (timeLeftWithSlow <= attackStartupWithSlow) return 1;
		return 0;
		
	}
	return 0;
}

void PlayerInfo::calcFrameAdvantageForFramebar(FrameAdvantageForFramebarResult* result) const {
	if (frameAdvantageValid && landingFrameAdvantageValid && frameAdvantage != landingFrameAdvantage) {
		result->frameAdvantage = frameAdvantage;
		result->landingFrameAdvantage = landingFrameAdvantage;
	} else if (frameAdvantageValid || landingFrameAdvantageValid) {
		int frameAdvantageUse = frameAdvantageValid ? frameAdvantage : landingFrameAdvantage;
		result->frameAdvantage = frameAdvantageUse;
		result->landingFrameAdvantage = SHRT_MIN;
	} else {
		result->frameAdvantage = SHRT_MIN;
		result->landingFrameAdvantage = SHRT_MIN;
	}
	
	if (frameAdvantageValid && landingFrameAdvantageValid && frameAdvantageNoPreBlockstun != landingFrameAdvantageNoPreBlockstun) {
		result->frameAdvantageNoPreBlockstun = frameAdvantageNoPreBlockstun;
		result->landingFrameAdvantageNoPreBlockstun = landingFrameAdvantageNoPreBlockstun;
	} else if (frameAdvantageValid || landingFrameAdvantageValid) {
		int frameAdvantageUse = frameAdvantageValid ? frameAdvantageNoPreBlockstun : landingFrameAdvantageNoPreBlockstun;
		result->frameAdvantageNoPreBlockstun = frameAdvantageUse;
		result->landingFrameAdvantageNoPreBlockstun = SHRT_MIN;
	} else {
		result->frameAdvantageNoPreBlockstun = SHRT_MIN;
		result->landingFrameAdvantageNoPreBlockstun = SHRT_MIN;
	}
}

ComboRecipeElement* PlayerInfo::findLastNonProjectileComboElement() {
	if (comboRecipe.empty()) return nullptr;
	for (int i = (int)comboRecipe.size() - 1; i >= 0; --i) {
		ComboRecipeElement& elem = comboRecipe[i];
		if (!elem.isProjectile && !elem.artificial) {
			return &elem;
		}
	}
	return nullptr;
}

ComboRecipeElement* PlayerInfo::findLastDash() {
	if (comboRecipe.empty()) return nullptr;
	for (int i = (int)comboRecipe.size() - 1; i >= 0; --i) {
		ComboRecipeElement& elem = comboRecipe[i];
		if (elem.dashDuration) {
			return &elem;
		}
	}
	return nullptr;
}

void PlayerInfo::bringComboElementToEnd(ComboRecipeElement* modifiedElement) {
	if (comboRecipe.empty()) return;
	for (int i = (int)comboRecipe.size() - 1; i >= 0; --i) {
		if (&comboRecipe[i] == modifiedElement) {
			if (i == (int)comboRecipe.size() - 1) return;
			ComboRecipeElement temp = comboRecipe[i];
			int numElementsMoved = (int)comboRecipe.size() - i - 1;
			memmove(comboRecipe.data() + i, comboRecipe.data() + i + 1, numElementsMoved * sizeof (ComboRecipeElement));
			comboRecipe[comboRecipe.size() - 1] = temp;
		}
	}
}

bool PlayerInfo::lastComboHitEqualsProjectile(Entity ptr) const {
	if (comboRecipe.empty()) return false;
	const ComboRecipeElement& lastElem = comboRecipe.back();
	return
		lastElem.isProjectile
		&& (
			strcmp(lastElem.stateName, ptr.animationName()) == 0
			&& strcmp(lastElem.trialName, ptr.dealtAttack()->trialName) == 0
		);
}

ComboRecipeElement::ComboRecipeElement()
	: whiffed(true),
	counterhit(false),
	otg(false),
	isMeleeAttack(false),
	isProjectile(false),
	artificial(false),
	isWalkForward(false),
	isWalkBackward(false),
	doneAfterIdle(false),
	isJump(false),
	isSuperJumpInstall(false) {
}

GatlingOrWhiffCancelInfoStored::GatlingOrWhiffCancelInfoStored()
		: nameIncludesInputs(false),
		wasAddedDuringHitstopFreeze(false),
		foundOnThisFrame(false),
		countersIncremented(false) { }

GatlingOrWhiffCancelInfo::GatlingOrWhiffCancelInfo()
		: GatlingOrWhiffCancelInfoStored() { }

bool Frame::operator==(const Frame& other) const {
	if (memcmp(this, &other, offsetof(Frame, next)) != 0) return false;
	size_t startCmp = offsetof(Frame, title);
	size_t endCmp = offsetof(Frame, type) + sizeof type;
	if (memcmp((BYTE*)this + startCmp, (BYTE*)&other + startCmp, endCmp - startCmp) != 0) return false;
	return activeDuringSuperfreeze == other.activeDuringSuperfreeze
		&& powerup == other.powerup
		&& marker == other.marker
		&& charSpecific1 == other.charSpecific1
		&& charSpecific2 == other.charSpecific2;
}

void ComboRecipeElement::player_markAsNotWhiff(PlayerInfo& attacker, const DmgCalc& dmgCalc, DWORD aswEngTick) {
	attacker.determineMoveNameAndSlangName(&name);
	counterhit = dmgCalc.u.hit.counterHit;
	otg = dmgCalc.isOtg;
	whiffed = false;
	timestamp = aswEngTick;
	if (hitCount == 0) hitCount = 1;
	attacker.bringComboElementToEnd(this);
}

void ComboRecipeElement::player_onFirstHitHappenedBeforeFrame3(PlayerInfo& attacker, const DmgCalc& dmgCalc, DWORD aswEngTick, bool isNormalThrow) {
	
	static const NamePair airThrow {
		"Air Throw"
	};
	
	static const NamePair groundThrow {
		"Ground Throw"
	};
	
	if (isNormalThrow) {
		if (attacker.pawn.y() != 0 || attacker.pawn.ascending()) {
			name = &airThrow;
		} else {
			name = &groundThrow;
		}
	} else {
		attacker.determineMoveNameAndSlangName(&name);
	}
	counterhit = dmgCalc.u.hit.counterHit;
	whiffed = false;
	otg = dmgCalc.isOtg;
	timestamp = aswEngTick;
	framebarId = -1;
	hitCount = 1;
	attacker.lastPerformedMoveNameIsInComboRecipe = true;
}

const char PROJECTILES_STR[12] = "Projectiles";

const NamePair PROJECTILES_NAMEPAIR {
	"Projectiles"
};

bool animationIsNeedCountRamlethalSwordTime(const char* animName) {
	DWORD hash = Entity::hashString(animName);
	const char* cmpStr;
	switch (hash) {
		case 0x4fb10dc6: return cmpStr = "BitN6C"; break;
		case 0x4faec33f: return cmpStr = "BitF6D"; break;
		case 0x76812821: return cmpStr = "BitN2C_Bunri"; break;
		case 0x9f2b250a: return cmpStr = "BitF2D_Bunri"; break;
		case 0x7a0a94ce: return cmpStr = "Bit4C"; break;
		case 0x7a0a94cf: return cmpStr = "Bit4D"; break;
		default: return false;
	}
	return strcmp(animName, cmpStr) == 0;
}

bool GatlingOrWhiffCancelInfoStored::operator==(const GatlingOrWhiffCancelInfoStored& other) const {
	return name == other.name
		&& replacementInputs == other.replacementInputs
		&& move == other.move
		&& bufferTime == other.bufferTime
		&& nameIncludesInputs == nameIncludesInputs;
}

bool PlayerCancelInfo::hasCancel(const char* skillName, const GatlingOrWhiffCancelInfo** infoPtr) const {
	for (const GatlingOrWhiffCancelInfo& info : gatlings) {
		if (strcmp(info.move->name, skillName) == 0) {
			if (infoPtr) *infoPtr = &info;
			return true;
		}
	}
	for (const GatlingOrWhiffCancelInfo& info : whiffCancels) {
		if (strcmp(info.move->name, skillName) == 0) {
			if (infoPtr) *infoPtr = &info;
			return true;
		}
	}
	return false;
}

template<typename T>
T* FixedArrayOfGatlingOrWhiffCancelInfos<T>::erase(T* ptr) {
	int index = ptr - elems;
	if (count - index > 1) {
		const size_t elemSize = sizeof (T);
		memmove(ptr, ptr + 1, elemSize * (count - index - 1));
	}
	--count;
	return elems + index;
}

template GatlingOrWhiffCancelInfoStored* FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfoStored>::erase(GatlingOrWhiffCancelInfoStored* ptr);
template GatlingOrWhiffCancelInfo* FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>::erase(GatlingOrWhiffCancelInfo* ptr);

// does not include the 'ending'
template<typename T>
T* FixedArrayOfGatlingOrWhiffCancelInfos<T>::erase(T* start, T* ending) {
	int startIndex = start - elems;
	int endIndex = ending - elems;
	if (count - endIndex > 0) {
		const size_t elemSize = sizeof (T);
		memmove(start, ending, elemSize * (count - endIndex));
	}
	count -= ending - start;
	return elems + startIndex;
}

template GatlingOrWhiffCancelInfoStored* FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfoStored>::erase(GatlingOrWhiffCancelInfoStored* start, GatlingOrWhiffCancelInfoStored* ending);
template GatlingOrWhiffCancelInfo* FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>::erase(GatlingOrWhiffCancelInfo* start, GatlingOrWhiffCancelInfo* ending);

template<typename T>
void FixedArrayOfGatlingOrWhiffCancelInfos<T>::emplace(T* ptr) {
	if (count == _countof(elems)) return;
	int index = ptr - elems;
	if (index == count) {
		++count;
		return;
	}
	const size_t elemSize = sizeof (T);
	memmove(ptr + 1, ptr, elemSize * (count - index));
	++count;
}

template void FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfoStored>::emplace(GatlingOrWhiffCancelInfoStored* ptr);
template void FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>::emplace(GatlingOrWhiffCancelInfo* ptr);

template<typename T>
bool FixedArrayOfGatlingOrWhiffCancelInfos<T>::hasCancel(const char* skillName, const T** infoPtr) const {
	for (const T& info : *this) {
		if (strcmp(info.move->name, skillName) == 0) {
			if (infoPtr) *infoPtr = &info;
			return true;
		}
	}
	return false;
}

template bool FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfoStored>::hasCancel(const char* skillName, const GatlingOrWhiffCancelInfoStored** infoPtr) const;
template bool FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>::hasCancel(const char* skillName, const GatlingOrWhiffCancelInfo** infoPtr) const;

template<typename T>
void FrameCancelInfoBase<T>::clear() {
	gatlings.clear();
	whiffCancels.clear();
	whiffCancelsNote = nullptr;
}

template void FrameCancelInfoBase<GatlingOrWhiffCancelInfo>::clear();
template void FrameCancelInfoBase<GatlingOrWhiffCancelInfoStored>::clear();

template<typename T>
bool FrameCancelInfoBase<T>::hasCancel(const char* skillName, const T** infoPtr) const {
	for (const T& info : gatlings) {
		if (strcmp(info.move->name, skillName) == 0) {
			if (infoPtr) *infoPtr = &info;
			return true;
		}
	}
	for (const T& info : whiffCancels) {
		if (strcmp(info.move->name, skillName) == 0) {
			if (infoPtr) *infoPtr = &info;
			return true;
		}
	}
	return false;
}

template bool FrameCancelInfoBase<GatlingOrWhiffCancelInfo>::hasCancel(const char* skillName, const GatlingOrWhiffCancelInfo** infoPtr) const;
template bool FrameCancelInfoBase<GatlingOrWhiffCancelInfoStored>::hasCancel(const char* skillName, const GatlingOrWhiffCancelInfoStored** infoPtr) const;

FrameCancelInfoStored::FrameCancelInfoStored() : FrameCancelInfoBase() { }

void FrameCancelInfoStored::copyFromAnotherArray(const FrameCancelInfoFull& src) {
	copyCancelsFromAnotherArrayPart(gatlings, src.gatlings);
	copyCancelsFromAnotherArrayPart(whiffCancels, src.whiffCancels);
	whiffCancelsNote = src.whiffCancelsNote;
}

void FrameCancelInfoStored::copyCancelsFromAnotherArrayPart(FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfoStored>& dest,
		const FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& src) {
	size_t elemsToCopy = src.size();
	dest.count = elemsToCopy;
	GatlingOrWhiffCancelInfoStored* destPtr = dest.data();
	const GatlingOrWhiffCancelInfo* srcPtr = src.data();
	for (size_t counter = elemsToCopy; counter != 0; --counter) {
		*destPtr = (const GatlingOrWhiffCancelInfoStored&)*srcPtr;
		++destPtr;
		++srcPtr;
	}
}

bool FrameCancelInfoStored::equalTruncated(const FrameCancelInfoFull& src) const {
	return equalTruncatedPart(gatlings, src.gatlings)
		&& equalTruncatedPart(whiffCancels, src.whiffCancels)
		&& whiffCancelsNote == src.whiffCancelsNote;
}

bool FrameCancelInfoStored::equalTruncatedPart(const FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfoStored>& dest,
			const FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& src) const {
	size_t srcSize = src.size();
	size_t destSize = dest.size();
	if (srcSize != destSize) return false;
	const GatlingOrWhiffCancelInfo* srcPtr = src.data();
	const GatlingOrWhiffCancelInfoStored* destPtr = dest.data();
	for (size_t counter = srcSize; counter != 0; --counter) {
		if (*srcPtr != *destPtr) return false;
		++srcPtr;
		++destPtr;
	}
	return true;
}

FrameCancelInfoFull::FrameCancelInfoFull() : FrameCancelInfoBase() { }

void FrameCancelInfoFull::unsetWasFoundOnThisFrame(bool unsetCountersIncremented) {
	for (GatlingOrWhiffCancelInfo& info : gatlings) {
		info.foundOnThisFrame = false;
		if (unsetCountersIncremented) {
			info.countersIncremented = false;
		}
	}
	for (GatlingOrWhiffCancelInfo& info : whiffCancels) {
		info.foundOnThisFrame = false;
		if (unsetCountersIncremented) {
			info.countersIncremented = false;
		}
	}
}

void FrameCancelInfoFull::deleteThatWhichWasNotFoundPart(FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& ar) {
	int i;
	int deletionStart;
	if (!ar.empty()) {
		GatlingOrWhiffCancelInfo* beginPtr = ar.begin();
		deletionStart = -1;
		for (i = (int)ar.size() - 1; i >= 0; --i) {
			if (!ar[i].foundOnThisFrame) {
				if (deletionStart == -1) {
					deletionStart = i;
				}
			} else if (deletionStart != -1) {
				ar.erase(beginPtr + i + 1, beginPtr + deletionStart + 1);
				deletionStart = -1;
			}
		}
		if (deletionStart != -1) {
			ar.erase(beginPtr, beginPtr + deletionStart + 1);
		}
	}
}

void FrameCancelInfoFull::deleteThatWhichWasNotFound() {
	deleteThatWhichWasNotFoundPart(gatlings);
	deleteThatWhichWasNotFoundPart(whiffCancels);
}

void FrameCancelInfoFull::clearDelays() {
	for (GatlingOrWhiffCancelInfo& frameCancel : gatlings) {
		frameCancel.framesBeenAvailableForNotIncludingHitstopFreeze = 0;
	}
	for (GatlingOrWhiffCancelInfo& frameCancel : whiffCancels) {
		frameCancel.framesBeenAvailableForNotIncludingHitstopFreeze = 0;
	}
}

void PlayerCancelInfo::copyFromAnotherArray(const FrameCancelInfoFull& src) {
	copyCancelsFromAnotherArrayPart(gatlings, src.gatlings);
	copyCancelsFromAnotherArrayPart(whiffCancels, src.whiffCancels);
}

void PlayerCancelInfo::copyCancelsFromAnotherArrayPart(std::vector<GatlingOrWhiffCancelInfo>& dest,
		const FixedArrayOfGatlingOrWhiffCancelInfos<GatlingOrWhiffCancelInfo>& src) {
	size_t elemsToCopy = src.size();
	dest.clear();
	dest.resize(elemsToCopy);
	memcpy(dest.data(), src.data(), elemsToCopy * sizeof GatlingOrWhiffCancelInfo);
}

void PlayerInfo::registerCreatedProjectile(ProjectileInfo& projectile) {
	if (projectile.lifeTimeCounter == 0
			//&& projectile.creator == player.pawn  // May Beach Ball is not directly created by the player
			//&& !idle  // Millia creates RoseObj objects during her dash animation which is considered 'idle'
	) {
		CreatedProjectileStruct theName;
		projectile.determineCreatedName(&theName, false, false);
		if (projectile.creatorNamePtr.useNamePair) {
			theName.createdByNamePair = projectile.creatorNamePtr.namePair;
			theName.useCreatedByNamePair = true;
		} else {
			theName.createdBy = projectile.creatorNamePtr.name;
			theName.useCreatedByNamePair = false;
		}
		createdProjectiles.push_back(theName);
	}
}

void ProjectileInfo::determineCreatedName(const MoveInfo* move, Entity ent, const NamePair* lastName, CreatedProjectileStruct* result, bool allowDetermineMove, bool leaveNullIfFailed) {
	MoveInfo moveInfoDataStorage;
	const MoveInfo* moveInfoPtr = move;
	if (!move && allowDetermineMove && ent) {
		entityList.populate();
		bool moveNonEmpty = moves.getInfo(moveInfoDataStorage,
			entityList.slots[ent.team()].characterType(),
			nullptr,
			ent.animationName(),
			true);
		if (moveNonEmpty) {
			moveInfoPtr = &moveInfoDataStorage;
		}
	}
	determineCreatedName(moveInfoPtr, ent, lastName, result, leaveNullIfFailed);
}

void ProjectileInfo::determineCreatedName(const MoveInfo* move, Entity ent, const NamePair* lastName, CreatedProjectileStruct* result, bool leaveNullIfFailed) {
	result->name = leaveNullIfFailed ? nullptr : "Created a projectile";
	result->createdBy = nullptr;
	result->usePrefix = false;
	result->useNamePair = false;
	result->useCreatedByNamePair = false;
	if (move && move->framebarNameUncombinedSelector && ent) {
		result->namePair = move->framebarNameUncombinedSelector(ent);
		result->useNamePair = true;
		result->usePrefix = true;
		return;
	} else if (move && move->framebarNameUncombined) {
		result->namePair = move->framebarNameUncombined;
		result->useNamePair = true;
		result->usePrefix = true;
		return;
	} else if (lastName) {
		result->namePair = lastName;
		result->useNamePair = true;
		result->usePrefix = true;
		return;
	} else if (move) {
		const NamePair* determinedName = nullptr;
		determineMoveNameAndSlangName(move, ent, &determinedName);
		if (determinedName) {
			result->namePair = determinedName;
			result->useNamePair = true;
			result->usePrefix = true;
			return;
		}
	}
	if (ent && ent.bbscrCurrentFunc()) {
		result->name = asInstr(ent.bbscrCurrentFunc(), beginState)->name;
		result->usePrefix = true;
	}
}

void PlayerInfo::determineCancelDelay(CancelDelay* result) const {
	const char* currentAnim = pawn.animationName();
	
	if (
		charType == CHARACTER_TYPE_JOHNNY
		&& (
			currentAnim[0] == 'M'
			&& currentAnim[1] == 'i'
			&& currentAnim[2] == 's'
			&& currentAnim[3] == 't'
			&& currentAnim[4] == 'F'
			&& currentAnim[5] == 'i'
			&& currentAnim[6] == 'n'
			&& currentAnim[7] == 'e'
			&& currentAnim[8] == 'r'
			&& (
				(
					currentAnim[9] >= 'A'
					&& currentAnim[9] <= 'C'
				)
				&& currentAnim[10] == 'L'
				&& currentAnim[11] == 'v'
				|| (
					currentAnim[9] == 'F'
					|| currentAnim[9] == 'B'
				)
				&& (
					currentAnim[10] == 'W'
					&& currentAnim[11] == 'a'
					&& currentAnim[12] == 'l'
					&& currentAnim[13] == 'k'
					|| currentAnim[10] == 'D'
					&& currentAnim[11] == 'a'
					&& currentAnim[12] == 's'
					&& currentAnim[13] == 'h'
					// include MistFinerFDashAir and MistFinerBDashAir
				)
				|| currentAnim[9] == 'C'
				&& currentAnim[10] == 'a'
				&& currentAnim[11] == 'n'
				&& currentAnim[12] == 'c'
				&& currentAnim[13] == 'e'
				&& currentAnim[14] == 'l'
				|| currentAnim[9] == 'L'
				&& currentAnim[10] == 'o'
				&& currentAnim[11] == 'o'
				&& currentAnim[12] == 'p'
				&& pawn.mem54()  // rev1 only
			)
			|| currentAnim[0] == 'A'
			&& currentAnim[1] == 'i'
			&& currentAnim[2] == 'r'
			&& currentAnim[3] == 'M'
			&& currentAnim[4] == 'i'
			&& currentAnim[5] == 's'
			&& currentAnim[6] == 't'
			&& currentAnim[7] == 'F'
			&& currentAnim[8] == 'i'
			&& currentAnim[9] == 'n'
			&& currentAnim[10] == 'e'
			&& currentAnim[11] == 'r'
			&& (
				(
					currentAnim[12] >= 'A'
					&& currentAnim[12] <= 'C'
				)
				&& currentAnim[13] == 'L'
				&& currentAnim[14] == 'v'
				|| currentAnim[12] == 'C'
				&& currentAnim[13] == 'a'
				&& currentAnim[14] == 'n'
				&& currentAnim[15] == 'c'
				&& currentAnim[16] == 'e'
				&& currentAnim[17] == 'l'
				|| currentAnim[12] == 'L'
				&& currentAnim[13] == 'o'
				&& currentAnim[14] == 'o'
				&& currentAnim[15] == 'p'
				&& pawn.mem54()  // rev1 only
			)
		)
	) {
		result->isAfterIdle = true;
		result->delay = timeInNewSectionForCancelDelay;
		return;
	}
	
	if (!pawn.currentAnimData()->isPerformedRaw()
		&& !(
				charType == CHARACTER_TYPE_LEO
				&& strcmp(pawn.previousAnimName(), "Semuke") == 0
			)) {
		result->isAfterIdle = false;
		result->delay = 0;
		if (!(
				charType == CHARACTER_TYPE_LEO
				&& strcmp(pawn.previousAnimName(), "CmnActFDash") == 0
			)) {
			const GatlingOrWhiffCancelInfo* foundCancel = nullptr;
			if (prevFrameCancels.hasCancel(pawn.currentMove()->name, &foundCancel)) {
				if (foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze > 0
						&& !(
							foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze == 1
							&& foundCancel->wasAddedDuringHitstopFreeze
						)) {
					result->delay = foundCancel->framesBeenAvailableForNotIncludingHitstopFreeze;
				}
			} else if (
					(timeSinceWasEnableSpecialCancel || timeSinceWasEnableSpecials)
					&& pawn.dealtAttack()->type >= ATTACK_TYPE_EX
					&& timeSinceWasEnableSpecialCancel
			) {
				result->delay = timeSinceWasEnableSpecialCancel;
			}
		}
	} else {
		result->delay = timePassedPureIdle;
		result->isAfterIdle = true;
	}
}
