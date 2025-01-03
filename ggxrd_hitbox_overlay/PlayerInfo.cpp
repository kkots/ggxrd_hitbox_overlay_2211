#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "PlayerInfo.h"
#include "findMoveByName.h"
#include "EndScene.h"
#include "EntityList.h"

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

void ProjectileInfo::fill(Entity ent, Entity superflashInstigator) {
	ptr = ent;
	team = ent.team();
	animFrame = ent.currentAnimDuration();
	int prevLifetimeCounter = lifeTimeCounter;
	lifeTimeCounter = ent.lifeTimeCounter();
	if (lifeTimeCounter == 0) {
		maxHit.clear();
		hitNumber = 0;
		hitOnFrame = 0;
		hitstopElapsed = 0;
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
	if (hitstop && !superflashInstigator && prevLifetimeCounter != lifeTimeCounter) ++hitstopElapsed;
	
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
	sprite.fill(ent);
	memcpy(animName, ent.animationName(), 32);
	determineMoveNameAndSlangName(&lastName, &lastSlangName);
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
	size_t cancelsOffset = offsetof(PlayerInfo, cancels);
	memset(this, 0, cancelsOffset);
	for (int i = 0; i < _countof(cancels); ++i) {
		cancels[i].clear();
	}
	cancelsOffset += sizeof cancels;
	cancelsTimer = 0;
	cancelsOffset += sizeof cancelsTimer;
	wasCancels.clear();
	cancelsOffset += sizeof wasCancels;
	cancelsCount = 0;
	cancelsOffset += sizeof cancelsCount;
	dmgCalcs.clear();
	cancelsOffset += sizeof dmgCalcs;
	memset(this, 0, sizeof PlayerInfo - cancelsOffset);
}

void PlayerInfo::copyTo(PlayerInfo& dest) {
	memcpy(&dest, this, sizeof PlayerInfo);
}

void PrevStartupsInfo::add(short n, bool partOfStance, const char* name, const char* slangName) {
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
	elem.moveSlangName = slangName;
	++count;
}

void PrevStartupsInfo::print(char*& buf, size_t& bufSize) const {
	if (!count) return;
	int charsPrinted;
	
	if (initialSkip) {
		charsPrinted = sprintf_s(buf, bufSize, "%d", initialSkip);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	
	for (int i = 0; i < count; ++i) {
		charsPrinted = sprintf_s(buf, bufSize, i == 0 && !initialSkip ? "%d" : "+%d", startups[i].startup);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	charsPrinted = sprintf_s(buf, bufSize, "+");
	if (charsPrinted == -1) return;
	buf += charsPrinted;
	bufSize -= charsPrinted;
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

void PrevStartupsInfo::printNames(char*& buf, size_t& bufSize, const char** lastNames, int lastNamesCount, bool slang, bool useMultiplicationSign, bool printFrames) const {
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
			if (i >= count) {
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

void PlayerInfo::printStartup(char* buf, size_t bufSize) {
	if (!bufSize) return;
	*buf = '\0';
	int uhh = startupType();
	if (uhh == -1) return;
	prevStartupsDisp.print(buf, bufSize);
	for (int i = 0; ; ++i) {
		int charsPrinted = -1;
		if (uhh == 0) {
			charsPrinted = sprintf_s(buf, bufSize, "%d+%d", superfreezeStartup, startupDisp - superfreezeStartup);
		} else if (uhh == 1) {
			charsPrinted = sprintf_s(buf, bufSize, "%d", superfreezeStartup);
		} else if (uhh == 2) {
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
	if ((startedUp || startupProj) && !(recoveryDisp == 0 && landingRecovery)) {
		if (recoveryDispCanBlock == -1 || recoveryDisp != 0) {
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
	if (printedTheMainThing && totalCanBlock > totalDisp && !(recoveryDispCanBlock != -1 && recoveryDisp == 0)) {
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
			&& !(recoveryDispCanBlock != -1 && recoveryDisp == 0)
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
	if (printedTheMainThing && totalFD) {
		if (charsPrinted && bufSize) {
			strcat(buf, "+");
			++buf;
			--bufSize;
		}
		sprintf_s(buf, bufSize, "%d FD", totalFD);
	}
}

void PlayerInfo::printTotal(char* buf, size_t bufSize) {
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
		prevStartupsTotalDisp.print(buf, bufSize);
		partsTotal = prevStartupsTotalDisp.total();
		partsCount += prevStartupsTotalDisp.count;
		if (totalCanBlock < totalDisp && totalCanBlock != 0) {
			charsPrinted = sprintf_s(buf, bufSize, "%d can't block+%d can't attack", totalCanBlock, totalDisp - totalCanBlock);
			partsCount += 2;
			mentionedCantAttack = true;
		} else {
			charsPrinted = sprintf_s(buf, bufSize, "%d", totalDisp);
			++partsCount;
		}
		partsTotal += totalCanBlock > totalDisp ? totalCanBlock : totalDisp;
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
		
		if (totalCanBlock > totalDisp) {
			charsPrinted = sprintf_s(buf, bufSize, " can't attack+%d can't block", totalCanBlock - totalDisp);
			mentionedCantAttack = true;
			++partsCount;
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
		if (totalCanFD > maxOfTheTwo && maxOfTheTwo != 0 && totalCanBlock != 0) {
			charsPrinted = sprintf_s(buf, bufSize, "%s+%d can't FD",
				mentionedCantAttack ? "" : " can't attack",
				totalCanFD - maxOfTheTwo);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
		
		if (landingRecovery) {
			charsPrinted = sprintf_s(buf, bufSize, "+%d landing", landingRecovery);
			if (charsPrinted == -1) return;
			buf += charsPrinted;
			bufSize -= charsPrinted;
		}
	}
	if (totalFD) {
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
	sprintf_s(buf, bufSize, "%s (%d/%d)", name, frame, frameMax);
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

void EntityFramebar::setTitle(const char* text,
			const char* slangName,
			const char* nameUncombined,
			const char* slangNameUncombined,
			const char* textFull) {
	
	if (text && *text == '\0') text = nullptr;
	if (slangName && *slangName == '\0') slangName = nullptr;
	if (nameUncombined && *nameUncombined == '\0') nameUncombined = nullptr;
	if (slangNameUncombined && *slangNameUncombined == '\0') slangNameUncombined = nullptr;
	if (textFull && *textFull == '\0') textFull = nullptr;
	titleShort = text;
	titleSlang = slangName;
	titleUncombined = nameUncombined;
	titleSlangUncombined = slangNameUncombined;
	titleFull = textFull;
}

void EntityFramebar::copyTitle(const EntityFramebar& source) {
	titleShort = source.titleShort;
	titleSlang = source.titleSlang;
	titleUncombined = source.titleUncombined;
	titleSlangUncombined = source.titleSlangUncombined;
	titleFull = source.titleFull;
}

int EntityFramebar::confinePos(int pos) {
	if (pos < 0) {
		return (int)_countof(Framebar::frames) + (pos + 1) % (int)_countof(Framebar::frames) - 1;  // (int) very important x_x (all covered in bruises) (written in blood)
	} else {
		return pos % _countof(Framebar::frames);
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

template<typename EntFramebarT, typename FrameT>
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
		
		int i, j;
		for (i = 0; i < prevTypesCount; ++i) {
			if (frame.type == prevTypes[i]) {
				frame.type = newType;
				
				#define piece(posName, barName) \
					if (posName != -1) { \
						FrameT& otherFrame = barName[posName]; \
						if (otherFrame.aswEngineTick == frame.aswEngineTick) { \
							for (j = 0; j < prevTypesCount; ++j) { \
								if (otherFrame.type == prevTypes[j]) { \
									otherFrame.type = newType; \
									framebars->decrementPos(posName); \
									break; \
								} \
							} \
							if (j == prevTypesCount) { \
								posName = -1; \
							} \
						} else if (otherFrame.aswEngineTick > frame.aswEngineTick) { \
							framebars->decrementPos(posName); \
						} \
					}
				
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
	::changePreviousFrames<ProjectileFramebar, Frame>(this,
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
	::changePreviousFrames<PlayerFramebars, PlayerFrame>(this,
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
}

void Framebar::soakUpIntoPreFrame(const FrameBase& srcFrame) {
	::soakUpIntoPreFrame<Framebar, Frame>(this, (Frame&)srcFrame);
}

void PlayerFramebar::soakUpIntoPreFrame(const FrameBase& srcFrame) {
	::soakUpIntoPreFrame<PlayerFramebar, PlayerFrame>(this, (PlayerFrame&)srcFrame);
}

static inline int determineFrameLevel(FrameType type) {
	if (type == FT_ACTIVE_PROJECTILE) {
		return 3;
	}
	if (type == FT_NON_ACTIVE_PROJECTILE) {
		return 2;
	}
	if (type == FT_IDLE_PROJECTILE || type == FT_IDLE_ACTIVE_IN_SUPERFREEZE) {
		return 1;
	}
	return 0;
}

enum SortItemType {
	// if you change the order of these elements you must also change the order of elements in SortItem items[] in printInvuls()
	
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

void printFameStop(char* buf, size_t bufSize, const FrameStopInfo* stopInfo, int hitstop, int hitstopMax) {
	if (!bufSize) return;
	*buf = '\0';
	bool hasStop = stopInfo ? (stopInfo->isHitstun || stopInfo->isBlockstun || stopInfo->isStagger || stopInfo->isWakeup) : false;
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
			stunName = "blockstun";
		} else if (stopInfo->isStagger) {
			stunName = "stagger";
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
	::processRequests<PlayerFramebar, PlayerFrame>(this, (PlayerFrame&)destinationFrame);
}

void Framebar::processRequests(int destinationPosition) {
	processRequests(frames[destinationPosition]);
}

void PlayerFramebar::processRequests(int destinationPosition) {
	processRequests(frames[destinationPosition]);
}

template<typename FramebarT>
inline void copyRequests(FramebarT* destination, FramebarT& source) {
	destination->requestFirstFrame |= source.requestFirstFrame;
	destination->requestNextHit |= source.requestNextHit;
}

void Framebar::copyRequests(FramebarBase& source) {
	::copyRequests<Framebar>(this, (Framebar&)source);
}

void PlayerFramebar::copyRequests(FramebarBase& source) {
	::copyRequests<PlayerFramebar>(this, (PlayerFramebar&)source);
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

CanProgramSecretGardenInfo PlayerInfo::canProgramSecretGarden() const {
	for (int i = 2; i < entityList.count; ++i) {
		Entity e = entityList.list[i];
		if (e
				&& e.isActive()
				&& e.team() == index
				&& strcmp(e.animationName(), "SecretGardenBall") == 0) {
			CanProgramSecretGardenInfo response;
			response.can = e.mem50();
			response.inputs = e.mem51();
			response.inputsMax = 4;
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
	cancels.clear();
	pos += sizeof cancels;
	memset((BYTE*)this + pos, 0, sizeof *this - pos);
}

void Framebar::catchUpToIdle(FramebarBase& source, int destinationStartingPosition, int framesToCatchUpFor) {
	Framebar& cast = (Framebar&)source;
	for (int i = 1; i <= framesToCatchUpFor; ++i) {
		int ind = (destinationStartingPosition + i) % _countof(Framebar::frames);
		soakUpIntoPreFrame(frames[ind]);
		frames[ind] = cast[ind];
	}
	copyRequests(cast);
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
	copyRequests(source);
}

FrameBase& Framebar::getFrame(int index) { return (FrameBase&)frames[index]; }
FrameBase& PlayerFramebar::getFrame(int index) { return (FrameBase&)frames[index]; }

void PlayerFramebar::clearCancels() {
	for (int i = 0; i < _countof(frames); ++i) {
		frames[i].cancels.clear();
	}
}

void PlayerFramebar::clearCancels(int index) {
	frames[index].cancels.clear();
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

void PlayerFramebars::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(PlayerFrame&)destFrame = (PlayerFrame&&)srcFrame;
}

void ProjectileFramebar::copyFrame(FrameBase& destFrame, FrameBase&& srcFrame) const {
	(Frame&)destFrame = (Frame&&)srcFrame;
}

template<typename FrameT>
inline void copyActiveDuringSuperfreeze(FrameT& destFrame, const FrameT& srcFrame) {
	if (!(srcFrame.type != FT_NONE && (srcFrame.activeDuringSuperfreeze || srcFrame.hitConnected))) return;
	if (destFrame.type == FT_NONE) destFrame.type = srcFrame.type;
	destFrame.activeDuringSuperfreeze |= srcFrame.activeDuringSuperfreeze;
	destFrame.hitConnected |= srcFrame.hitConnected;
}

void PlayerFramebars::copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const {
	::copyActiveDuringSuperfreeze((PlayerFrame&)destFrame, (const PlayerFrame&)srcFrame);
}

void ProjectileFramebar::copyActiveDuringSuperfreeze(FrameBase& destFrame, const FrameBase& srcFrame) const {
	::copyActiveDuringSuperfreeze((Frame&)destFrame, (const Frame&)srcFrame);
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

void FrameCancelInfo::clear() {
	gatlings.clear();
	whiffCancels.clear();
	whiffCancelsNote = nullptr;
}

bool CombinedProjectileFramebar::canBeCombined(const Framebar& source) const {
	for (int i = 0; i < _countof(source.frames); ++i) {
		if (!frameTypeDiscardable(main[i].type) && !frameTypeDiscardable(source[i].type)) return false;
	}
	return true;
}

void CombinedProjectileFramebar::combineFramebar(const Framebar& source, const ProjectileFramebar* dad) {
	for (int i = 0; i < _countof(source.frames); ++i) {
		const Frame& sf = source[i];
		Frame& df = main[i];
		sources[i] = dad;
		if (determineFrameLevel(sf.type) >= determineFrameLevel(df.type)) {
			df.type = sf.type;
			df.animName = sf.animName;
			df.animSlangName = sf.animSlangName;
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
		df.newHit |= sf.newHit;
		df.rcSlowdown = max(df.rcSlowdown, sf.rcSlowdown);
		df.rcSlowdownMax = max(df.rcSlowdownMax, sf.rcSlowdownMax);
		df.activeDuringSuperfreeze |= sf.activeDuringSuperfreeze;
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
}

void CombinedProjectileFramebar::determineName() {
	for (int i = _countof(main.frames) - 1; i >= 0; --i) {
		const ProjectileFramebar* source = sources[i];
		if (source) {
			copyTitle(*source);
			moveFramebarId = source->moveFramebarId;
			return;
		}
	}
}

bool PlayerCancelInfo::cancelsEqual(const PlayerCancelInfo& other) const {
	if (cancels.gatlings.size() != other.cancels.gatlings.size()
			|| cancels.whiffCancels.size() != other.cancels.whiffCancels.size()
			|| enableJumpCancel != other.enableJumpCancel
			|| enableSpecialCancel != other.enableSpecialCancel
			|| enableSpecials != other.enableSpecials
			|| airborne != other.airborne) {
		return false;
	}
	{
		auto it = cancels.gatlings.begin();
		auto otherIt = other.cancels.gatlings.begin();
		for (; it != cancels.gatlings.end(); ) {
			if (*it != *otherIt) return false;
			++it;
			++otherIt;
		}
	}
	{
		auto it = cancels.whiffCancels.begin();
		auto otherIt = other.cancels.whiffCancels.begin();
		for (; it != cancels.whiffCancels.end(); ) {
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
	cancels.clear();
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
	return cancels.gatlings.empty() && cancels.whiffCancels.empty() && !enableJumpCancel && !enableSpecialCancel && !enableSpecials;
}

void PlayerInfo::determineMoveNameAndSlangName(const char** name, const char** slangName) const {
	determineMoveNameAndSlangName(moveNonEmpty ? &move : nullptr, idle, pawn, name, slangName);
}

void PlayerInfo::determineMoveNameAndSlangName(Entity pawn, const char** name, const char** slangName) {
	MoveInfo moveInfo;
	bool moveNonEmpty = moves.getInfo(moveInfo,
		pawn.characterType(),
		pawn.currentMoveIndex() == -1 ? nullptr : pawn.currentMove()->name,
		pawn.animationName(),
		false);
	bool idle = false;
	if (moveNonEmpty) {
		static std::unique_ptr<PlayerInfo> dummyPlayer = nullptr;
		if (!dummyPlayer) {
			dummyPlayer = std::make_unique<PlayerInfo>();
		}
		dummyPlayer->pawn = pawn;
		dummyPlayer->wasEnableWhiffCancels = pawn.enableWhiffCancels();
		dummyPlayer->wasForceDisableFlags = pawn.forceDisableFlags();
		idle = moveInfo.isIdle(*dummyPlayer);
	}
	determineMoveNameAndSlangName(moveNonEmpty ? &moveInfo : nullptr, idle, pawn, name, slangName);
}

void PlayerInfo::determineMoveNameAndSlangName(const MoveInfo* move, bool idle, Entity pawn, const char** name, const char** slangName) {
	if (name) *name = nullptr;
	if (slangName) *slangName = nullptr;
	if (move) {
		if (name) *name = move->getDisplayName(idle);
		if (slangName) *slangName = move->getDisplayNameSlang(idle);
	}
	if (name && !*name) {
		int moveIndex = pawn.currentMoveIndex();
		if (moveIndex == -1) {
			*name = (const char*)(pawn.bbscrCurrentFunc() + 4);
		} else {
			const AddedMoveData* actualMove = pawn.movesBase() + moveIndex;
			*name = actualMove->name;
		}
	}
}

void ProjectileInfo::determineMoveNameAndSlangName(const char** name, const char** slangName) const {
	determineMoveNameAndSlangName(moveNonEmpty ? &move : nullptr, ptr, name, slangName);
}

void ProjectileInfo::determineMoveNameAndSlangName(Entity ptr, const char** name, const char** slangName, const char** framebarNameFull) {
	entityList.populate();
	MoveInfo moveInfo;
	bool moveNonEmpty = moves.getInfo(moveInfo,
		entityList.slots[ptr.team()].characterType(),
		nullptr,
		ptr.animationName(),
		true);
	determineMoveNameAndSlangName(moveNonEmpty ? &moveInfo : nullptr, ptr, name, slangName);
	if (framebarNameFull) *framebarNameFull = moveInfo.framebarNameFull;
}

void ProjectileInfo::determineMoveNameAndSlangName(const MoveInfo* move, Entity ptr, const char** name, const char** slangName) {
	if (name) *name = nullptr;
	if (slangName) *slangName = nullptr;
	if (move) {
		if (name) {
			if (move->framebarNameSelector && ptr) {
				*name = move->framebarNameSelector(ptr);
			} else if (move->framebarNameUncombined) {
				*name = move->framebarNameUncombined;
			} else if (move->framebarName) {
				*name = move->framebarName;
			}
		}
		if (slangName) {
			if (move->framebarSlangNameSelector && ptr) {
				*slangName = move->framebarSlangNameSelector(ptr);
			} else if (move->framebarSlangNameUncombined) {
				*slangName = move->framebarSlangNameUncombined;
			} else if (move->framebarSlangName) {
				*slangName = move->framebarSlangName;
			}
		}
	} else if (!ptr) {
		if (name) *name = nullptr;
	} else {
		if (name) *name = (const char*)(ptr.bbscrCurrentFunc() + 4);
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
	if (ptr && !ptr.dealtAttack()->attackMultiHit()) {
		return ptr.hitSomethingOnThisFrame();
	} else {
		return landedHit || clashedOnThisFrame;
	}
}
