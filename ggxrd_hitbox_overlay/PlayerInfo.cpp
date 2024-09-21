#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "PlayerInfo.h"

int PlayerInfo::findActiveProjectile(Entity ent) {
	for (int i = 0; i < activeProjectilesCount; ++i) {
		if (activeProjectiles[i] == ent) {
			return i;
		}
	}
	return -1;
}

void PlayerInfo::addActiveProjectile(Entity ent) {
	hasNewActiveProjectiles = true;
	if (findActiveProjectile(ent) != -1) return;
	if (activeProjectilesCount >= _countof(activeProjectiles)) {
		memmove(activeProjectiles, activeProjectiles + 1, sizeof activeProjectiles - sizeof *activeProjectiles);
		activeProjectiles[activeProjectilesCount - 1] = ent;
		return;
	}
	activeProjectiles[activeProjectilesCount] = ent;
	++activeProjectilesCount;
}

void PlayerInfo::removeActiveProjectile(int index) {
	if (index != activeProjectilesCount - 1) {
		memmove(activeProjectiles + index, activeProjectiles + index + 1, (activeProjectilesCount - index - 1) * sizeof *activeProjectiles);
	}
	--activeProjectilesCount;
}

void ActiveDataArray::addActive(int n) {
	if (count == 0) {
		data[0].actives = n;
		data[0].nonActives = 0;
		count = 1;
		return;
	}
	if (count >= _countof(data)) {
		memmove(data, data + 1, sizeof data - sizeof *data);
		data[count - 1].actives = n;
		data[count - 1].nonActives = 0;
		return;
	}
	if (data[count - 1].nonActives) {
		data[count].actives = n;
		data[count].nonActives = 0;
		++count;
		return;
	} else {
		data[count - 1].actives += n;
	}
}

void ActiveDataArray::print(char* buf, size_t bufSize) {
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
			}
		}
		ActiveData& elem = data[i];
		result = sprintf_s(buf, bufSize, "%d", elem.actives);
		if (result != -1) {
			buf += result;
			if ((int)bufSize <= result) return;
			else bufSize -= result;
		}
		lastNonActives = elem.nonActives;
	}
}

void PlayerInfo::addGap(int length) {
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
