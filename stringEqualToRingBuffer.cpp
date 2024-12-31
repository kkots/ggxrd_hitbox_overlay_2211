// ringBuffer - ring buffer storing some read string that you want to compare
//              the size of the ring buffer must be greater than or equal to stringByteSize.
//              The ring buffer need not be null-terminated
// ringBufferHeadPosition - the current position of the head of the ring buffer, that is the last position that
//                          you wrote the last character into
// string - the string to compare to. The string must be null-terminated
// stringByteSize - the size of the string, in bytes, including the null character
int stringEqualToRingBuffer(const char* ringBuffer, int ringBufferHeadPosition, const char* string, int stringByteSize) {
	int ringBufferComparisonPos = ringBufferHeadPosition == stringByteSize - 1 ? 0 : ringBufferHeadPosition + 1;
	bool stringsMatch = false;
	if (ringBufferComparisonPos == 0) {
		return strncmp(string, ringBuffer, stringByteSize);
	} else {
		int firstByteCount = stringByteSize - ringBufferComparisonPos;
		int firstResult = memcmp(string, ringBuffer + ringBufferComparisonPos, firstByteCount);
		if (firstResult == 0) {
			return memcmp(string + firstByteCount, ringBuffer, ringBufferComparisonPos);
		} else {
			return firstResult;
		}
	}
}
