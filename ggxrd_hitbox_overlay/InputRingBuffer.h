#pragma once
#include "Input.h"
struct InputRingBuffer {
	Input lastInput{0};
	Input currentInput{0};
	Input inputs[30];
	unsigned short framesHeld[30];
	unsigned short index = 0;
	void stepBack();
	unsigned short calculateLength() const;
	inline bool empty() const { return framesHeld[index] == 0; }
	inline bool hasJustBeenReset() const { return index == 0 && framesHeld[0] == 1 && framesHeld[29] == 0; }
	enum ChargeType {
		CHARGE_TYPE_HORIZONTAL,
		CHARGE_TYPE_VERTICAL
	};
	int parseCharge(ChargeType type, bool isFacingLeft) const;
};