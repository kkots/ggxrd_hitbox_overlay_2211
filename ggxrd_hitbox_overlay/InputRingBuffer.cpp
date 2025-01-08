#include "pch.h"
#include "InputRingBuffer.h"
void InputRingBuffer::stepBack() {
	unsigned short framesHeldValue = framesHeld[index];
	if (framesHeldValue == 0) {
		return;
	}
	if (framesHeldValue == 1) {
		framesHeld[index] = 0;
		inputs[index] = Input{0};
		if (index == 0) {
			index = 29;
		} else {
			--index;
		}
	} else {
		--framesHeld[index];
	}
}

unsigned short InputRingBuffer::calculateLength() const {
	unsigned short sourceIndex = index;
	unsigned short sourceLength = 0;
	for (int i = 30; i != 0; --i) {
		if (framesHeld[sourceIndex] == 0) break;

		++sourceLength;

		if (sourceIndex == 0) sourceIndex = 29;
		else --sourceIndex;
	}

	return sourceLength;
}

int InputRingBuffer::parseCharge(ChargeType type, bool isFacingLeft) const {
	int idx = (int)index;
	if (!framesHeld[idx]) return 0;
	
	int accumulatedCharge = 0;
	
	int framesRemaining = 10 - framesHeld[idx];
	bool dirMatches = false;
	int limit = 30;
	do {
		const Input* input = inputs + idx;
		if (type == CHARGE_TYPE_HORIZONTAL) {
			if (!isFacingLeft) {
				dirMatches = input->left;
			} else {
				dirMatches = input->right;
			}
		} else {
			dirMatches = input->down;
		}
		
		if (!dirMatches) break;
		accumulatedCharge += framesHeld[idx];
		if (limit == 0) break;
		--limit;
		--idx;
		if (idx < 0) idx = 29;
		if (framesHeld[idx] == 0) break;
		
	} while (dirMatches);
	return accumulatedCharge;
}
