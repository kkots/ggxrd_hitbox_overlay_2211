#pragma once
#include <string>
struct StringWithLength {
	const char* txt;
	size_t length;
	StringWithLength(const char* txt, size_t length) : txt(txt), length(length) { }
	StringWithLength() : txt(nullptr), length(0) { }
	StringWithLength(const std::string& source) : txt(source.c_str()), length(source.size()) { }
	template<size_t size> inline StringWithLength(const char(&txt)[size]) : txt(txt), length(size - 1) { }
	
};
