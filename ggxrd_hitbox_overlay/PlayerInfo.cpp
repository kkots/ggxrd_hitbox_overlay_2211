#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "PlayerInfo.h"

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

void ActiveDataArray::print(char* buf, size_t bufSize) const {
	size_t origBufSize = bufSize;
	char* origBuf = buf;
	if (count == 0) {
		sprintf_s(buf, bufSize, "0");
		return;
	}
	int maxConsecutiveHits = 0;
	int currentConsecutiveHits = 0;
	int lastActives = 0;
	int lastNonActives = 0;
	int result;
	for (int i = 0; i < count && bufSize; ++i) {
		if (lastNonActives) {
			currentConsecutiveHits = 1;
			result = sprintf_s(buf, bufSize, "(%d)", lastNonActives);
			if (result != -1) {
				buf += result;
				if ((int)bufSize <= result) return;
				bufSize -= result;
			} else return;
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
				return;
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
			buf += result;
			if ((int)bufSize <= result) return;
			bufSize -= result;
		} else return;
		lastNonActives = elem.nonActives;
		lastActives = elem.actives;
	}
	if (count > 5 && buf - origBuf > 10) {
		char ownbuf[512];
		memmove(ownbuf, origBuf, buf - origBuf + 1);
		bufSize = origBufSize;
		buf = origBuf;
		*buf = '\0';
		printNoSeparateHits(buf, bufSize);
		size_t newBufLen = strlen(buf);
		if (newBufLen > 10) {
			printNoSeparateHitsGapsBiggerThan3(buf, bufSize);
			newBufLen = strlen(buf);
		}
		if (strlen(buf) >= strlen(ownbuf)) {
			newBufLen = sprintf_s(buf, bufSize, "%d", total());
		}
		sprintf_s(buf + newBufLen, bufSize - newBufLen, " / %s", ownbuf);
	}
}

void ActiveDataArray::printNoSeparateHits(char* buf, size_t bufSize) const {
	if (count == 0) {
		sprintf_s(buf, bufSize, "0");
		return;
	}
	int lastNonActives = 0;
	for (int i = 0; i < count && bufSize; ++i) {
		int result;
		if (lastNonActives) {
			result = sprintf_s(buf, bufSize, "(%d)", lastNonActives);
			if (result != -1) {
				buf += result;
				if ((int)bufSize <= result) return;
				else bufSize -= result;
			} else return;
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
			buf += result;
			if ((int)bufSize <= result) return;
			else bufSize -= result;
		} else return;
	}
}

void ActiveDataArray::printNoSeparateHitsGapsBiggerThan3(char* buf, size_t bufSize) const {
	if (count == 0) {
		sprintf_s(buf, bufSize, "0");
		return;
	}
	int lastNonActives = 0;
	for (int i = 0; i < count && bufSize; ++i) {
		int result;
		int n = 0;
		if (lastNonActives && lastNonActives > 5) {
			result = sprintf_s(buf, bufSize, "(%d)", lastNonActives);
			if (result != -1) {
				buf += result;
				if ((int)bufSize <= result) return;
				else bufSize -= result;
			} else return;
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
			buf += result;
			if ((int)bufSize <= result) return;
			else bufSize -= result;
		} else return;
	}
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

void ProjectileInfo::fill(Entity ent) {
	ptr = ent;
	team = ent.team();
	animFrame = ent.currentAnimDuration();
	lifeTimeCounter = ent.lifeTimeCounter();
	if (!ent.hitSomethingOnThisFrame()) {
		hitstop = ent.hitstop();
	} else {
		hitstop = 0;
	}
	hitNumber = ent.currentHitNum();
	sprite.fill(ent);
	memcpy(animName, ent.animationName(), 32);
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
		*buf = '\0';
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
	memset(this, 0, sizeof PlayerInfo);
}

void PlayerInfo::copyTo(PlayerInfo& dest) {
	memcpy(&dest, this, sizeof PlayerInfo);
}

void PrevStartupsInfo::add(short n) {
	if (count >= _countof(startups)) {
		memmove(startups, startups + 1, sizeof startups - sizeof *startups);
		--count;
		// the dumb version of ring buffer
	}
	startups[count] = n;
	++count;
}

void PrevStartupsInfo::print(char*& buf, size_t& bufSize) const {
	if (!count) return;
	int charsPrinted;
	for (int i = 0; i < count; ++i) {
		charsPrinted = sprintf_s(buf, bufSize, i == 0 ? "%d" : "+%d", startups[i]);
		if (charsPrinted == -1) return;
		buf += charsPrinted;
		bufSize -= charsPrinted;
	}
	charsPrinted = sprintf_s(buf, bufSize, "%c", '+');
	if (charsPrinted == -1) return;
	buf += charsPrinted;
	bufSize -= charsPrinted;
}

void PlayerInfo::printStartup(char* buf, size_t bufSize) {
	*buf = '\0';
	int uhh = -1;
    if (superfreezeStartup && superfreezeStartup <= startupDisp && (startedUp || startupProj)) {
    	uhh = 0;
    } else if (superfreezeStartup && !(startedUp || startupProj)) {
    	uhh = 1;
    } else if (startedUp || startupProj) {
    	uhh = 2;
    } else {
    	return;
    }
	prevStartupsDisp.print(buf, bufSize);
    for (int i = 0; ; ++i) {
    	int charsPrinted;
	    if (uhh == 0) {
			charsPrinted = sprintf_s(buf, bufSize, "%d+%d", superfreezeStartup, startupDisp - superfreezeStartup);
	    } else if (uhh == 1) {
	    	charsPrinted = sprintf_s(buf, bufSize, "%d", superfreezeStartup);
	    } else if (uhh == 2) {
	    	charsPrinted = sprintf_s(buf, bufSize, "%d", startupDisp);
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
	*buf = '\0';
	prevStartups.print(buf, bufSize);
	sprintf_s(buf, bufSize, "%d", startup);
}

void PlayerInfo::printRecovery(char* buf, size_t bufSize) {
	*buf = '\0';
	int charsPrinted = 0;
	bool printedTheMainThing = false;
    if ((startedUp || startupProj) && !(recoveryDisp == 0 && landingRecovery)) {
    	if (totalCanBlock < totalDisp && recoveryDisp - (totalDisp - totalCanBlock) > 0) {
    		charsPrinted = sprintf_s(buf, bufSize, "%d can't block+%d can't attack",
    			recoveryDisp - (totalDisp - totalCanBlock),
    			totalDisp - totalCanBlock);
    	} else {
			charsPrinted = sprintf_s(buf, bufSize, "%d", recoveryDisp);
    	}
    	if (charsPrinted == -1) return;
    	buf += charsPrinted;
    	bufSize -= charsPrinted;
    	printedTheMainThing = true;
    }
    if (printedTheMainThing && totalCanBlock > totalDisp) {
    	charsPrinted = sprintf_s(buf, bufSize, "%s%d can't block", charsPrinted ? " can't attack+" : "", totalCanBlock - totalDisp);
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
	char* origBuf = buf;
	*buf = '\0';
	int charsPrinted;
	bool printedMainPart = false;
	int partsCount = 0;
	int partsTotal = 0;
    if ((idlePlus || isLandingOrPreJump || cmnActIndex == CmnActJump) && totalDisp) {
    	printedMainPart = true;
    	prevStartupsTotalDisp.print(buf, bufSize);
    	partsTotal = prevStartupsTotalDisp.total();
    	partsCount += prevStartupsTotalDisp.count;
    	if (totalCanBlock < totalDisp && totalCanBlock != 0) {
    		charsPrinted = sprintf_s(buf, bufSize, "%d can't block+%d can't attack", totalCanBlock, totalDisp - totalCanBlock);
    		partsCount += 2;
    	} else {
    		charsPrinted = sprintf_s(buf, bufSize, "%d", totalDisp);
    		++partsCount;
    	}
		partsTotal += totalCanBlock > totalDisp ? totalCanBlock : totalDisp;
    	if (charsPrinted == -1) return;
    	buf += charsPrinted;
    	bufSize -= charsPrinted;
    	charsPrinted = 0;
	    if (totalCanBlock > totalDisp) {
	    	charsPrinted = sprintf_s(buf, bufSize, " can't attack+%d can't block", totalCanBlock - totalDisp);
	    	++partsCount;
	    }
    	if (charsPrinted == -1) return;
    	buf += charsPrinted;
    	bufSize -= charsPrinted;
	    if (landingRecovery) {
	    	charsPrinted = sprintf_s(buf, bufSize, "+%d landing", landingRecovery);
	    	if (charsPrinted == -1) return;
	    	buf += charsPrinted;
	    	bufSize -= charsPrinted;
	    }
    }
    if (idlePlus && totalFD) {
    	charsPrinted = sprintf_s(buf, bufSize, "%s%d FD", printedMainPart ? "+" : "", totalFD);
    	if (charsPrinted == -1) return;
    	buf += charsPrinted;
    	bufSize -= charsPrinted;
    }
    
    if (partsCount > 2) {
    	sprintf_s(buf, bufSize, "=%d", partsTotal);
	    if (landingRecovery) {
	    	charsPrinted = sprintf_s(buf, bufSize, "+%d landing", landingRecovery);
	    	if (charsPrinted == -1) return;
	    	buf += charsPrinted;
	    	bufSize -= charsPrinted;
	    }
	    if (idlePlus && totalFD) {
	    	sprintf_s(buf, bufSize, "+%d FD", totalFD);
	    }
    }
}

void ProjectileInfo::printTotal(char* buf, size_t bufSize) {
	*buf = '\0';
	prevStartups.print(buf, bufSize);
	sprintf_s(buf, bufSize, "%d", total);
}

bool PlayerInfo::isIdleInNewSection() {
	return inNewMoveSection
		&& move
		&& move->considerIdleInSeparatedSectionAfterThisManyFrames
		&& timeInNewSection > move->considerIdleInSeparatedSectionAfterThisManyFrames;
}

int PrevStartupsInfo::total() const {
	int sum = 0;
	for (int i = 0; i < count; ++i) {
		sum += startups[i];
	}
	return sum;
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

void EntityFramebar::setTitle(const char* text, const char* textFull) {
	
	if (textFull && *textFull == '\0') textFull = nullptr;
	
	int len;
	int bytesUpToMax;
	int cpsTotal;
	utf8len(text, &len, &cpsTotal, titleShortCharsCountMax, &bytesUpToMax);
	
	if (bytesUpToMax != len) {
		strncpy(titleShort, text, bytesUpToMax);
		memset(titleShort + bytesUpToMax, 0, sizeof titleShort - bytesUpToMax);
		if (textFull) {
			titleFull = textFull;
		} else {
			titleFull = text;
		}
	} else {
		memset(titleShort, 0, sizeof titleShort);
		strcpy(titleShort, text);
		if (textFull) {
			titleFull = textFull;
		} else {
			titleFull.clear();
		}
	}
}

void EntityFramebar::copyTitle(const EntityFramebar& source) {
	memcpy(titleShort, source.titleShort, sizeof titleShort);
	titleFull = source.titleFull;
}

int EntityFramebar::confinePos(int pos) {
	if (pos < 0) {
		return (int)_countof(Framebar::frames) + (pos + 1) % (int)_countof(Framebar::frames) - 1;  // (int) very important x_x (all covered in bruises) (written in blood)
	} else {
		return pos % _countof(Framebar::frames);
	}
}

int Framebar::findTickNoGreaterThan(int startingPos, DWORD tick) const {
	for (int i = 0; i < _countof(frames); ++i) {
		int curPos = (startingPos - i + _countof(frames)) % _countof(frames);
		if (frames[curPos].type == FT_NONE) return -1;
		DWORD curTick = frames[curPos].aswEngineTick;
		if (curTick <= tick) {
			return curPos;
		}
	}
	return -1;
}

void EntityFramebar::changePreviousFrames(FrameType prevType,
		FrameType newType,
		int positionHitstopIdle,
		int positionHitstop,
		int positionIdle,
		int position,
		int maxCount,
		bool stopAtFirstFrame) {
	if (maxCount <= 0) return;
	
	positionHitstopIdle = confinePos(positionHitstopIdle);
	
	DWORD aswEngineTick = idleHitstop[positionHitstopIdle].aswEngineTick;
	
	int hitstopPos = hitstop.findTickNoGreaterThan(confinePos(positionHitstop), aswEngineTick);
	int idlePos = idle.findTickNoGreaterThan(confinePos(positionIdle), aswEngineTick);
	int mainPos = main.findTickNoGreaterThan(confinePos(position), aswEngineTick);
	
	while (maxCount) {
		Frame& frame = idleHitstop[positionHitstopIdle];
		
		if (stopAtFirstFrame && frame.isFirst) break;
		
		if (frame.type == prevType) {
			frame.type = newType;
			
			#define piece(posName, barName) \
				if (posName != -1) { \
					Frame& otherFrame = barName[posName]; \
					if (otherFrame.aswEngineTick == frame.aswEngineTick) { \
						if (otherFrame.type == prevType) { \
							otherFrame.type = newType; \
							decrementPos(posName); \
						} else { \
							posName = -1; \
						} \
					} else if (otherFrame.aswEngineTick > frame.aswEngineTick) { \
						decrementPos(posName); \
					} \
				}
			
			piece(hitstopPos, hitstop)	
			piece(idlePos, idle)
			piece(mainPos, main)
			
			#undef piece
		} else {
			break;
		}
		
		--maxCount;
		decrementPos(positionHitstopIdle);
	}
}

void Framebar::clear() {
	memset(this, 0, sizeof *this);
}

void Framebar::soakUpIntoPreFrame(const Frame& srcFrame) {
	if (preFrame == srcFrame.type && !srcFrame.isFirst) {
		++preFrameLength;
	} else {
		preFrame = srcFrame.type;
		preFrameLength = 1;
	}
}

static inline int determineFrameLevel(const Frame& frame) {
	if (frame.type == FT_ACTIVE) {
		return 3;
	}
	if (frame.type == FT_NON_ACTIVE) {
		return 2;
	}
	if (frame.type == FT_IDLE) {
		return 1;
	}
	return 0;
}

void Framebar::combineFramebar(const Framebar& source) {
	for (int i = 0; i < _countof(frames); ++i) {
		if (determineFrameLevel(source[i]) >= determineFrameLevel(frames[i])) {
			frames[i].type = source[i].type;
		}
		frames[i].strikeInful |= source[i].strikeInful;
	}
}
