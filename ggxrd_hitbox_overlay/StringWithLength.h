#pragma once
struct StringWithLength {
	const char* txt;
	size_t length;
	StringWithLength(const char* txt, size_t length) : txt(txt), length(length) { }
	StringWithLength() : txt(nullptr), length(0) { }
	template<size_t size> inline StringWithLength(const char(&txt)[size]) : txt(txt), length(size - 1) { }
};