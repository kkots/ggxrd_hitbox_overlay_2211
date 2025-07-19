#pragma once
class SplitStringIterator
{
public:
	SplitStringIterator(const char* strStart, const char* strEnd, char splitChar);
	bool getNext(const char** partStart, const char** partEnd);
	// Result is based on current iterator position and is always correct in that respect
	size_t numLines() const;
private:
	const char* strPtr;
	const char* strEnd;
	char splitChar;
	bool lastPartHadSplitterAtTheEnd = true;
};
