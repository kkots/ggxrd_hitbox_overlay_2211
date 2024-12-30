#pragma once
#include "InputRingBufferStored.h"
#include "InputsDrawingCommand.h"
class InputsDrawing
{
public:
	void produceData(const InputRingBufferStored& inputRingBufferStored, InputsDrawingCommandRow* result, unsigned int* const resultSize, bool rightSide);
private:
	inline void addToResult(InputsDrawingCommandRow& row, InputsDrawingCommandIcon icon, bool dark) { row.cmds[row.count++] = { icon, dark }; }
	inline void produceDataFromInput(const Input& input, unsigned short framesHeld, InputsDrawingCommandIcon* const prevDirection,
			const Input** const prevInput, unsigned short* const prevFramesHeld, InputsDrawingCommandRow** const result, unsigned int* const resultSize, bool rightSide);
};

extern InputsDrawing inputsDrawing;
