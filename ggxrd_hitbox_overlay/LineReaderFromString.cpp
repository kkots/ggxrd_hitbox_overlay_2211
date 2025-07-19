#include "pch.h"
#include "LineReaderFromString.h"

LineReaderFromString::LineReaderFromString(const char* start) : linePtr(start) { }

bool LineReaderFromString::readLine(const char** lineStart, const char** lineEnd) {
	
	const char* newlinePos = strchr(linePtr, '\n');
	if (newlinePos) {
		const char* nextPos = newlinePos + 1;
		if (newlinePos != linePtr && *(newlinePos - 1) == '\r') {
			rCharDetected = true;
			--newlinePos;
		}
		*lineStart = linePtr;
		*lineEnd = newlinePos;
		linePtr = nextPos;
		lastLineEndedWithNewline = true;
		return true;
	}
	
	bool old_lastLineEndedWithNewline = lastLineEndedWithNewline;
	lastLineEndedWithNewline = false;
	
	if (*linePtr != '\0' || old_lastLineEndedWithNewline) {
		*lineStart = linePtr;
		const char* end = linePtr + strlen(linePtr);
		*lineEnd = end;
		linePtr = end;
		return true;
	}
	
	return false;
	
}

// the result is only valid if called before the first readLine call
size_t LineReaderFromString::numLines() const {
	
	size_t result = 0;
	const char* linePtrIter = linePtr;
	while (true) {
		const char* newlinePos = strchr(linePtrIter, '\n');
		if (newlinePos) {
			linePtrIter = newlinePos + 1;
			++result;
			continue;
		}
		
		if (*linePtrIter != '\0' || result > 0) {
			return result + 1;
		}
		
		return result;
		
	}
	
}
