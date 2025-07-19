#pragma once
class LineReaderFromString
{
public:
	LineReaderFromString(const char* start);
	bool readLine(const char** lineStart, const char** lineEnd);
	inline bool readLine(char** lineStart, char** lineEnd) {
		return readLine(const_cast<const char**>(lineStart), const_cast<const char**>(lineEnd));
	}
	// the result is only valid if called before the first readLine call
	size_t numLines() const;
	bool rCharDetected = false;
private:
	const char* linePtr = nullptr;
	bool lastLineEndedWithNewline = false;
};
