#include "pch.h"
#include "SplitStringIterator.h"

SplitStringIterator::SplitStringIterator(const char* strStart, const char* strEnd, char splitChar)
	: strPtr(strStart), strEnd(strEnd), splitChar(splitChar) { }

bool SplitStringIterator::getNext(const char** partStart, const char** partEnd) {
	if (strPtr == strEnd) {
		if (lastPartHadSplitterAtTheEnd) {
			*partStart = strEnd;
			*partEnd = strEnd;
			lastPartHadSplitterAtTheEnd = false;
			return true;
		} else {
			return false;
		}
	}
	const char* found = (const char*)memchr(strPtr, splitChar, strEnd - strPtr);
	if (!found) {
		*partStart = strPtr;
		*partEnd = strEnd;
		strPtr = strEnd;
		lastPartHadSplitterAtTheEnd = false;
		return true;
	}
	
	*partStart = strPtr;
	*partEnd = found;
	strPtr = found + 1;
	return true;
	
}

// Result is based on current iterator position and is always correct in that respect
size_t SplitStringIterator::numLines() const {
	size_t result = 0;
	const char* ptr = strPtr;
	bool lastEndedWithNewline = lastPartHadSplitterAtTheEnd;
	while (true) {
		if (ptr == strEnd) {
			if (lastEndedWithNewline) {
				return result + 1;
			} else {
				return result;
			}
		}
		const char* found = (const char*)memchr(ptr, splitChar, strEnd - ptr);
		if (!found) {
			return result + 1;
		}
		
		ptr = found + 1;
		++result;
	}
}
