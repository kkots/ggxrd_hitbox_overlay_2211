#include "pch.h"
#include "InputsDrawing.h"
#include "PngResource.h"
#include "resource.h"
#include "Input.h"
#include "logging.h"

InputsDrawing inputsDrawing;

#ifdef LOG_PATH
extern bool loggedDrawingInputsOnce;
#endif
void InputsDrawing::produceData(const InputRingBufferStored& inputRingBufferStored, InputsDrawingCommandRow* result, unsigned int* const resultSize, bool rightSide) {
	const unsigned int dataSize = inputRingBufferStored.data.size();
	unsigned int index = (inputRingBufferStored.index + 1) % dataSize;
	InputsDrawingCommandIcon prevDirection = INPUT_ICON_NONE;
	const Input* prevInput = nullptr;
	unsigned short prevFramesHeld = 0;
	
	#ifdef LOG_PATH
	if (!loggedDrawingInputsOnce) {
		logwrap(fprintf(logfile, "InputsDrawing::produceData: inputRingBufferStored.data.size(): %u\n", inputRingBufferStored.data.size()));
	}
	#endif
	
	for (unsigned int i = dataSize; i != 0; --i) {
		const InputRingBufferStored::InputFramesHeld& inputStored = inputRingBufferStored.data[index];
		const Input& input = inputStored.input;
		const unsigned short framesHeld = inputStored.framesHeld;
		if (framesHeld != 0) {

			#ifdef LOG_PATH
			if (!loggedDrawingInputsOnce) {
				logwrap(fprintf(logfile, "InputsDrawing::produceData: index: %u, input: %.4X, framesHeld: %hu\n",
					index, (unsigned int)input, framesHeld));
			}
			#endif

			produceDataFromInput(input, framesHeld, &prevDirection, &prevInput, &prevFramesHeld, &result, resultSize, rightSide);
		}

		++index;
		if (index == dataSize) index = 0;
	}
}

inline void InputsDrawing::produceDataFromInput(const Input& input, unsigned short framesHeld, InputsDrawingCommandIcon* const prevDirection,
		const Input** const prevInput, unsigned short* const prevFramesHeld, InputsDrawingCommandRow** const result, unsigned int* const resultSize, bool rightSide) {
	InputsDrawingCommandIcon direction = INPUT_ICON_NONE;
	if (input.right) {
		if (input.up) {
			direction = INPUT_ICON_UPRIGHT;
		}
		else if (input.down) {
			direction = INPUT_ICON_DOWNRIGHT;
		}
		else {
			direction = INPUT_ICON_RIGHT;
		}
	}
	else if (input.left) {
		if (input.up) {
			direction = INPUT_ICON_UPLEFT;
		}
		else if (input.down) {
			direction = INPUT_ICON_DOWNLEFT;
		}
		else {
			direction = INPUT_ICON_LEFT;
		}
	}
	else if (input.up) {
		direction = INPUT_ICON_UP;
	}
	else if (input.down) {
		direction = INPUT_ICON_DOWN;
	}
	
	#ifdef LOG_PATH
	if (!loggedDrawingInputsOnce) {
		if (direction == INPUT_ICON_UP) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: up\n"));
		}
		else if (direction == INPUT_ICON_UPRIGHT) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: up right\n"));
		}
		else if (direction == INPUT_ICON_RIGHT) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: right\n"));
		}
		else if (direction == INPUT_ICON_DOWNRIGHT) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: down right\n"));
		}
		else if (direction == INPUT_ICON_DOWN) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: down\n"));
		}
		else if (direction == INPUT_ICON_DOWNLEFT) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: down left\n"));
		}
		else if (direction == INPUT_ICON_LEFT) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: left\n"));
		}
		else if (direction == INPUT_ICON_UPLEFT) {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: up left\n"));
		}
		else {
			logwrap(fprintf(logfile, "InputsDrawing::produceData: direction: neutral\n"));
		}
	}
	#endif

	const Input* prevInputInput = *prevInput;
	bool punchPressed;
	bool kickPressed;
	bool slashPressed;
	bool heavySlashPressed;
	bool dustPressed;
	bool specialPressed;
	bool tauntPressed;
	if (prevInputInput == nullptr) {
		punchPressed = input.punch;
		kickPressed = input.kick;
		slashPressed = input.slash;
		heavySlashPressed = input.heavySlash;
		dustPressed = input.dust;
		specialPressed = input.special;
		tauntPressed = input.taunt;
	} else {
		punchPressed = input.punch && !prevInputInput->punch;
		kickPressed = input.kick && !prevInputInput->kick;
		slashPressed = input.slash && !prevInputInput->slash;
		heavySlashPressed = input.heavySlash && !prevInputInput->heavySlash;
		dustPressed = input.dust && !prevInputInput->dust;
		specialPressed = input.special && !prevInputInput->special;
		tauntPressed = input.taunt && !prevInputInput->taunt;
	}
	
	#ifdef LOG_PATH
	if (!loggedDrawingInputsOnce) {
		logwrap(fprintf(logfile, "InputsDrawing::produceData: punchPressed: %u, kickPressed: %u, slashPressed: %u, heavySlashPressed: %u,"
			" dustPressed: %u, specialPressed: %u, tauntPressed: %u\n",
			punchPressed, kickPressed, slashPressed, heavySlashPressed, dustPressed, specialPressed, tauntPressed));
	}
	#endif

	if (direction && direction != *prevDirection
		|| punchPressed
		|| kickPressed
		|| slashPressed
		|| heavySlashPressed
		|| dustPressed
		|| specialPressed
		|| tauntPressed) {
		if (*prevFramesHeld > 40
			&& !*prevDirection
			&& !prevInputInput->punch
			&& !prevInputInput->kick
			&& !prevInputInput->slash
			&& !prevInputInput->heavySlash
			&& !prevInputInput->dust
			&& !prevInputInput->special
			&& !prevInputInput->taunt) {
			
			#ifdef LOG_PATH
			if (!loggedDrawingInputsOnce) {
				logwrap(fprintf(logfile, "InputsDrawing::produceData: adding an empty line\n"));
			}
			#endif
			
			++(*result);
			++(*resultSize);
		}

		InputsDrawingCommandRow& row = **result;
		if (!rightSide) {
			if (direction) addToResult(row, direction, direction == *prevDirection);
			if (input.punch) addToResult(row, INPUT_ICON_PUNCH, !punchPressed);
			if (input.kick) addToResult(row, INPUT_ICON_KICK, !kickPressed);
			if (input.slash) addToResult(row, INPUT_ICON_SLASH, !slashPressed);
			if (input.heavySlash) addToResult(row, INPUT_ICON_HEAVYSLASH, !heavySlashPressed);
			if (input.dust) addToResult(row, INPUT_ICON_DUST, !dustPressed);
			if (input.special) addToResult(row, INPUT_ICON_SPECIAL, !specialPressed);
			if (input.taunt) addToResult(row, INPUT_ICON_TAUNT, !tauntPressed);
		} else {
			if (input.taunt) addToResult(row, INPUT_ICON_TAUNT, !tauntPressed);
			if (input.special) addToResult(row, INPUT_ICON_SPECIAL, !specialPressed);
			if (input.dust) addToResult(row, INPUT_ICON_DUST, !dustPressed);
			if (input.heavySlash) addToResult(row, INPUT_ICON_HEAVYSLASH, !heavySlashPressed);
			if (input.slash) addToResult(row, INPUT_ICON_SLASH, !slashPressed);
			if (input.kick) addToResult(row, INPUT_ICON_KICK, !kickPressed);
			if (input.punch) addToResult(row, INPUT_ICON_PUNCH, !punchPressed);
			if (direction) addToResult(row, direction, direction == *prevDirection);
		}
		++(*result);
		++(*resultSize);
	}

	*prevFramesHeld = framesHeld;
	*prevInput = &input;
	*prevDirection = direction;
}
