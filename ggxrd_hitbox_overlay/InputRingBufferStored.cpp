#include "pch.h"
#include "InputRingBufferStored.h"
#include "logging.h"

#ifdef LOG_PATH
extern bool loggedDrawingInputsOnce;
#endif
void InputRingBufferStored::update(const InputRingBuffer& inputRingBuffer, const InputRingBuffer& prevInputRingBuffer) {
	
	unsigned short indexDiff = 0;
	const bool currentEmpty = inputRingBuffer.empty();
	const bool prevEmpty = prevInputRingBuffer.empty();
	#ifdef LOG_PATH
	if (!loggedDrawingInputsOnce) {
		logwrap(fprintf(logfile, "InputRingBufferStored::update: currentEmpty: %u, prevEmpty: %u\n", (unsigned int)currentEmpty, (unsigned int)prevEmpty));
	}
	#endif
	if (currentEmpty) {
		
		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fputs("InputRingBufferStored::update: currentEmpty, exiting\n", logfile));
		}
		#endif
		
		return;
		
	} else if (inputRingBuffer.hasJustBeenReset() && !prevInputRingBuffer.hasJustBeenReset()) {
		indexDiff = 1;
		
	} else if (prevEmpty && !currentEmpty) {
		indexDiff = inputRingBuffer.index + 1;
		
		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fprintf(logfile, "InputRingBufferStored::update: prevEmpty && !currentEmpty. indexDiff: %u\n", indexDiff));
		}
		#endif
		
	} else {
		if (inputRingBuffer.index < prevInputRingBuffer.index) {
			indexDiff = inputRingBuffer.index + 30 - prevInputRingBuffer.index;
		} else {
			indexDiff = inputRingBuffer.index - prevInputRingBuffer.index;
		}
		
		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fprintf(logfile, "InputRingBufferStored::update: walked into initial else branch. indexDiff: %u\n", indexDiff));
		}
		#endif
	}
	isCleared = false;

	if (data[index].framesHeld == 0 && indexDiff > 0) {
		--indexDiff;
		
		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fprintf(logfile, "InputRingBufferStored::update: correcting indexDiff because data is empty. indexDiff: %u\n", indexDiff));
		}
		#endif
	}
	index = (index + indexDiff) % data.size();
	
	#ifdef LOG_PATH
	if (!loggedDrawingInputsOnce) {
		logwrap(fprintf(logfile, "InputRingBufferStored::update: the new index value: %u\n", index));
	}
	#endif
	
	unsigned int destinationIndex = index;
	unsigned short sourceIndex = inputRingBuffer.index;
	
	#ifdef LOG_PATH
	if (!loggedDrawingInputsOnce) {
		logwrap(fprintf(logfile, "InputRingBufferStored::update: iterating inputRingBuffer beginning from %u\n", sourceIndex));
	}
	#endif
	
	for (int i = 30; i != 0; --i) {
		
		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fprintf(logfile, "InputRingBufferStored::update: inputRingBuffer index %u\n", sourceIndex));
		}
		#endif

		const Input& input = inputRingBuffer.inputs[sourceIndex];
		const unsigned short framesHeld = inputRingBuffer.framesHeld[sourceIndex];

		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fprintf(logfile, "InputRingBufferStored::update: input: %.4X; framesHeld: %hu\n", (unsigned int)input, framesHeld));
		}
		#endif

		if (framesHeld == 0) {

			#ifdef LOG_PATH
			if (!loggedDrawingInputsOnce) {
				logwrap(fprintf(logfile, "InputRingBufferStored::update: issued a break because framesHeld 0\n"));
			}
			#endif

			break;
		}

		#ifdef LOG_PATH
		if (!loggedDrawingInputsOnce) {
			logwrap(fprintf(logfile, "InputRingBufferStored::update: destinationIndex: %u\n", destinationIndex));
		}
		#endif

		data[destinationIndex].framesHeld = framesHeld;
		data[destinationIndex].input = input;

		if (destinationIndex == 0) destinationIndex = data.size() - 1;
		else --destinationIndex;

		if (sourceIndex == 0) sourceIndex = 29;
		else --sourceIndex;
	}
}

void InputRingBufferStored::clear() {
	if (isCleared) return;
	isCleared = true;
	memset(&data.front(), 0, sizeof(InputFramesHeld) * data.size());
}

Input InputRingBufferStored::lastInput() const {
	if (isCleared || !data[index].framesHeld) return Input{0x0000};
	return data[index].input;
}
