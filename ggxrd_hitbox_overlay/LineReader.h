#pragma once
#include "pch.h"
#include <string>
#include <vector>

class LineReader
{
public:
	LineReader(HANDLE file);
	// Reads the next line in the file.
	// Returns true if a line was read.
	// Returns false if there is no next line.
	// If it returns false, use error to check if there was an error.
	// If that is the case, you can obtain the error using GetLastError() Windows API function.
	bool readLine(std::string& line);
	bool rCharDetected = false;
	bool error = false;
private:
	HANDLE file = NULL;
	static const size_t bufSize = 20;
	std::vector<char> buf;
	std::vector<char> accum;
	size_t accumPos = 0;
	bool reachedEnd = false;
	size_t numLinesReturned = 0;
	bool lastReturnedLineEndedWithNewline = false;
	
	// returns true if a line was successfully found
	bool extractNearestLineFromAccum(std::string& line);
};

