#pragma once
#include "pch.h"
#include <vector>
#include "StringWithLength.h"
#include "Entity.h"

struct JSONParseContext {
	char c;
	const BYTE* data;
	size_t dataSize;
	int lineCount;
	std::vector<BYTE> stringLit;
	float parsedValue;
};

struct JSONParsedBox {
	HitboxType type;
	int x;
	int y;
	int w;
	int h;
};

struct JSONParsedSprite {
	char name[32] { '\0' };
	BYTE* jonbin = nullptr;  // I tried to add a destructor, but when std::vector::emplace, it deletes the old ones, so I'd also need a copy or move constructor, etc, etc, etc *fades into distance*
	DWORD size;
};

bool JSONReadSomethingExtremelySpecific(JSONParseContext& context, const StringWithLength& specificThing);

inline bool JSONReadTrue(JSONParseContext& context);

inline bool JSONReadFalse(JSONParseContext& context);

inline bool JSONReadNull(JSONParseContext& context);

bool JSONReadPropertyValue(JSONParseContext& context);

// starts on an opening {
bool JSONReadObject(JSONParseContext& context);

// starts on an opening [
bool JSONReadArray(JSONParseContext& context);

// starts on the first character of the number or -
bool JSONReadNumberLiteral(JSONParseContext& context);

// starts on the opening "
bool JSONReadStringLiteral(JSONParseContext& context);

bool checkValidSpriteName(JSONParseContext& context);

std::vector<char> joinErrors(const char* const* errorList, size_t errorListSize, const StringWithLength& separator);

template<size_t size>
inline std::vector<char> joinErrors(const char* const (&errorList)[size], const StringWithLength& separator) {
	return joinErrors(errorList, size, separator);
}

inline std::vector<char> joinErrors(const std::vector<const char*>& errorList, const StringWithLength& separator) {
	return joinErrors(errorList.data(), errorList.size(), separator);
}

bool parseJson(const BYTE* data, size_t* dataSizePtr, std::vector<JSONParsedSprite>& parsedSprites);
