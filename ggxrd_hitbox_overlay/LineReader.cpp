#include "pch.h"
#include "LineReader.h"

LineReader::LineReader(HANDLE file) : file(file) {
	buf.resize(bufSize, '\0');
	accum.resize(1);
	accum[0] = '\0';
}

bool LineReader::readLine(std::string& line) {
	
	if (extractNearestLineFromAccum(line)) {
		++numLinesReturned;
		lastReturnedLineEndedWithNewline = true;
		return true;
	}
	
	while (!reachedEnd) {
		DWORD bytesRead = 0;
		if (!ReadFile(file, buf.data(), bufSize, &bytesRead, NULL)) {
			// don't call anything here, preserve GetLastError()
			error = true;
			return false;
		}
		
		if (bytesRead < bufSize) {
			reachedEnd = true;
		}
		
		if (bytesRead == 0) break;
		
		while (bytesRead > 0 && buf[bytesRead - 1] == '\0') {
			--bytesRead;
		}
		
		if (accumPos > 0) {
			const size_t remainingSize = accum.size() - accumPos;
			if (remainingSize > 1) {
				memmove(accum.data(), accum.data() + accumPos, remainingSize);
				accum.resize(remainingSize);
			} else {
				accum.resize(1);
				accum[0] = '\0';
			}
			accumPos = 0;
		}
		
		// accum.size() can't be 0
		size_t oldSize = accum.size();
		size_t newSize = oldSize - 1 + bytesRead + 1;
		accum.resize(newSize);
		memcpy(accum.data() + oldSize - 1, buf.data(), bytesRead);
		accum[newSize - 1] = '\0';
		
		if (extractNearestLineFromAccum(line)) {
			++numLinesReturned;
			lastReturnedLineEndedWithNewline = true;
			return true;
		}
		
	}
	
	bool old_lastReturnedLineEndedWithNewline = lastReturnedLineEndedWithNewline;
	lastReturnedLineEndedWithNewline = false;
	
	size_t remainingLength = accum.size() - 1 - accumPos;
	if (remainingLength == 0) {
		if (numLinesReturned > 0 && old_lastReturnedLineEndedWithNewline) {
			line.clear();
			return true;
		} else {
			return false;
		}
	}
	
	line.assign(accum.data() + accumPos, remainingLength);
	accumPos = accum.size() - 1;
	return true;
	
}

bool LineReader::extractNearestLineFromAccum(std::string& line) {
	
	const char* accumEnd = nullptr;
	const char* accumPtr = nullptr;
	if (accumPos < accum.size() - 1) {
		accumPtr = accum.data() + accumPos;
		accumEnd = accum.data() + accum.size() - 1;
	}
	while (accumPtr < accumEnd) {
		const char* newlinePos = strchr(accumPtr, '\n');
		if (!newlinePos) {
			return false;
		}
		const char* nextPtr = newlinePos + 1;
		if (newlinePos > accumPtr && *(newlinePos - 1) == '\r') {
			rCharDetected = true;
			--newlinePos;
		}
		if (newlinePos == accumPtr) {
			line.clear();
		} else {
			line.assign(accumPtr, newlinePos - accumPtr);
		}
		accumPos += nextPtr - accumPtr;
		accumPtr = nextPtr;
		return true;
	}
	return false;
	
}
