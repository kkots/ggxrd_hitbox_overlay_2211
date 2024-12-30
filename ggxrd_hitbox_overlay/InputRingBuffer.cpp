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
