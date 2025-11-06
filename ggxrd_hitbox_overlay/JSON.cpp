#include "pch.h"
#include "JSON.h"
#include <stdio.h>
#include <malloc.h>
#include <vector>
#include "StringWithLength.h"
#include "Entity.h"
#include <unordered_map>
#include <stdexcept>

extern void showErrorDlgS(const char* error);

char strbuf[1024];

extern const char* hitboxTypeName[17];

static void showJsonErr(const char* msg, int lineNumber) {
	int len = strlen(msg);
	static const char fmt[] { ". On line -2147483648" };
	char* buf = (char*)alloca(len + sizeof (fmt));
	sprintf_s(buf, len + sizeof (fmt), "%s. On line %d", msg, lineNumber);
	showErrorDlgS(buf);
}

bool JSONReadSomethingExtremelySpecific(JSONParseContext& context, const StringWithLength& specificThing) {
	if (specificThing.length == 1
			&& context.c == *specificThing.txt
			|| specificThing.length > 1
			&& context.dataSize + 1 >= specificThing.length
			&& memcmp(context.data - 1, specificThing.txt, specificThing.length) == 0) {
		if (specificThing.length > 1) {
			context.data += specificThing.length - 1;
			context.dataSize -= specificThing.length - 1;
		}
		return true;
	}
	sprintf_s(strbuf, "Expected to read '%s', got something else", specificThing.txt);
	showJsonErr(strbuf, context.lineCount);
	return false;
}

inline bool JSONReadTrue(JSONParseContext& context) {
	static const StringWithLength thing { "true" };
	return JSONReadSomethingExtremelySpecific(context, thing);
}

inline bool JSONReadFalse(JSONParseContext& context) {
	static const StringWithLength thing { "false" };
	return JSONReadSomethingExtremelySpecific(context, thing);
}

inline bool JSONReadNull(JSONParseContext& context) {
	static const StringWithLength thing { "null" };
	return JSONReadSomethingExtremelySpecific(context, thing);
}

bool JSONReadPropertyValue(JSONParseContext& context) {
	char c = context.c;
	if (c == '"') {
		if (!JSONReadStringLiteral(context)) return false;
	} else if (c == '-' || c >= '0' && c <= '9') {
		if (!JSONReadNumberLiteral(context)) return false;
	} else if (c == 't') {
		if (!JSONReadTrue(context)) return false;
	} else if (c == 'f') {
		if (!JSONReadFalse(context)) return false;
	} else if (c == 'n') {
		if (!JSONReadNull(context)) return false;
	} else if (c == '{') {
		if (!JSONReadObject(context)) return false;
	} else if (c == '[') {
		if (!JSONReadArray(context)) return false;
	} else {
		showJsonErr("Unknown property type", context.lineCount);
		return false;
	}
	return true;
}

// starts on an opening {
bool JSONReadObject(JSONParseContext& context) {
	enum Mode {
		MODE_START,
		MODE_COLON,
		MODE_PROPERTY_VALUE,
		MODE_COMMA,
		MODE_PROPERTY_NAME
	} mode = MODE_START;
	while (*context.data != '\0' && context.dataSize) {
		context.c = *context.data;
		++context.data;
		--context.dataSize;
		char c = context.c;
		
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if (c == '\n') ++context.lineCount;
			continue;
		}
		
		if (mode == MODE_START) {
			if (c == '}') {
				return true;
			} else if (c == '"') {
				if (!JSONReadStringLiteral(context)) return false;
				mode = MODE_COLON;
			} else {
				showJsonErr("Unexpected character. Expected either a } or a \"", context.lineCount);
				return false;
			}
		} else if (mode == MODE_COLON) {
			if (c == ':') {
				mode = MODE_PROPERTY_VALUE;
			} else {
				showJsonErr("Unexpected character. Expected a :", context.lineCount);
				return false;
			}
		} else if (mode == MODE_PROPERTY_VALUE) {
			if (!JSONReadPropertyValue(context)) return false;
			mode = MODE_COMMA;
		} else if (mode == MODE_COMMA) {
			if (c == '}') {
				return true;
			} else if (c == ',') {
				mode = MODE_PROPERTY_NAME;
			} else {
				showJsonErr("Unexpected character. Expected a } or a ,", context.lineCount);
				return false;
			}
		} else if (mode == MODE_PROPERTY_NAME) {
			if (c == '"') {
				if (!JSONReadStringLiteral(context)) return false;
				mode = MODE_COLON;
			} else {
				showJsonErr("Unexpected character. Expected a \"", context.lineCount);
				return false;
			}
		}
	}
	showJsonErr("Unexpected end of JSON data", context.lineCount);
	return false;
}

// starts on an opening [
bool JSONReadArray(JSONParseContext& context) {
	enum Mode {
		MODE_START,
		MODE_VALUE,
		MODE_COMMA
	} mode = MODE_START;
	while (*context.data != '\0' && context.dataSize) {
		context.c = *context.data;
		++context.data;
		--context.dataSize;
		char c = context.c;
		
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if (c == '\n') ++context.lineCount;
			continue;
		}
		
		if (mode == MODE_START) {
			if (c == ']') {
				return true;
			} else {
				if (!JSONReadPropertyValue(context)) return false;
				mode = MODE_COMMA;
			}
		} else if (mode == MODE_COMMA) {
			if (c == ']') {
				return true;
			} else if (c == ',') {
				mode = MODE_VALUE;
			} else {
				showJsonErr("Unexpected character. Expected either a ] or a ,", context.lineCount);
				return false;
			}
		} else if (mode == MODE_VALUE) {
			if (!JSONReadPropertyValue(context)) return false;
			mode = MODE_COMMA;
		}
	}
	showJsonErr("Unexpected end of JSON data", context.lineCount);
	return false;
}

// starts on the first character of the number or -
bool JSONReadNumberLiteral(JSONParseContext& context) {
	const BYTE* numberLiteralStart = context.data - 1;
	bool parsingMinus = true;
	bool parsingDigits;
	bool encounteredDot;
	bool encounteredExponent;
	bool encounteredExponentSign;
	do {
		char c = context.c;
		if (parsingMinus) {
			parsingMinus = false;
			encounteredDot = false;
			encounteredExponent = false;
			if (c == '-') {
				parsingDigits = true;
			} else if (c >= '0' && c <= '9') {
				parsingDigits = false;
			} else {
				showJsonErr("Invalid number literal", context.lineCount);
				return false;
			}
		} else if (c >= '0' && c <= '9'
				&& !(encounteredExponent && !encounteredExponentSign)) {
			parsingDigits = false;
		} else if (c == '.'
				&& !encounteredExponent
				&& !parsingDigits
				&& !encounteredDot) {
			encounteredDot = true;
			parsingDigits = true;
		} else if ((c == 'e' || c == 'E')
				&& !parsingDigits
				&& !encounteredExponent) {
			encounteredExponent = true;
			encounteredExponentSign = false;
		} else if (encounteredExponent && !encounteredExponentSign) {
			encounteredExponentSign = true;
			if (c == '+' || c == '-') {
				parsingDigits = true;
			} else if (!(
				c >= '0' && c <= '9'
			)) {
				showJsonErr("Invalid number literal", context.lineCount);
				return false;
			}
		} else {
			break;
		}
		
		if (*context.data != '\0' && context.dataSize) {
			context.c = (char)*context.data;
			++context.data;
			--context.dataSize;
		} else {
			break;
		}
	} while (true);
	
	if (parsingMinus || parsingDigits
			|| encounteredExponent && !encounteredExponentSign) {
		showJsonErr("Invalid number literal", context.lineCount);
		return false;
	}
	
	size_t numberLiteralLength = context.data - numberLiteralStart;
	void* myBuf = alloca(numberLiteralLength + 1);
	memcpy(myBuf, numberLiteralStart, numberLiteralLength);
	((BYTE*)myBuf)[numberLiteralLength] = '\0';
	int parsedElementCount = sscanf((const char*)myBuf, "%g", &context.parsedValue);
	if (!parsedElementCount) {
		showJsonErr("Failed to parse number", context.lineCount);
		return false;
	}
	
	--context.data;
	++context.dataSize;
	return true;
}

// starts on the opening "
bool JSONReadStringLiteral(JSONParseContext& context) {
	bool escaped = false;
	bool parsingHexCode = false;
	DWORD hexCode;
	int digitsRead;
	context.stringLit.clear();
	while (*context.data != '\0' && context.dataSize) {
		context.c = (char)*context.data;
		char c = context.c;
		++context.data;
		--context.dataSize;
		if (!parsingHexCode) {
			if (!escaped) {
				if (c == '\\') {
					escaped = true;
				} else if (c < 32) {
					showJsonErr("Invalid character in string literal in JSON", context.lineCount);
					return false;
				} else if (c == '"') {
					context.stringLit.push_back('\0');
					return true;
				} else if (context.stringLit.size() < 128) {
					context.stringLit.push_back(c);
				}
			} else {
				escaped = false;
				if (c == '"' || c == '\\' || c == '/') {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back(c);
					}
				} else if (c == 'b') {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back('\b');
					}
				} else if (c == 'f') {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back('\f');
					}
				} else if (c == 'n') {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back('\n');
					}
				} else if (c == 'r') {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back('\r');
					}
				} else if (c == 't') {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back('\t');
					}
				} else if (c == 'u') {
					parsingHexCode = true;
					hexCode = 0;
					digitsRead = 0;
				} else {
					showJsonErr("Invalid escape sequence in a string literal", context.lineCount);
					return false;
				}
			}
		} else {
			if (c >= '0' && c <= '9') {
				hexCode = (hexCode << 4) | (c - '0');
			} else if (c >= 'a' && c <= 'f') {
				hexCode = (hexCode << 4) | (c - 'a' + 10);
			} else if (c >= 'A' && c <= 'F') {
				hexCode = (hexCode << 4) | (c - 'A' + 10);
			} else {
				showJsonErr("Expected 4 hex digits after a \\u in a string literal, got something else", context.lineCount);
				return false;
			}
			++digitsRead;
			if (digitsRead == 4) {
				if (hexCode <= 0x7f) {
					if (hexCode == 0) {
						showJsonErr("Invalid hex code in \\u in a string literal", context.lineCount);
						return false;
					}
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back((BYTE)hexCode);
					}
				} else if (hexCode <= 0x7ff) {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back((BYTE)(
							0b11000000
							| (hexCode >> 6)));
						context.stringLit.push_back((BYTE)(
							0b10000000
							| (hexCode & 0b111111)));
					}
				} else if (hexCode <= 0xffff) {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back((BYTE)(
							0b11100000
							| (hexCode >> 12)));
						context.stringLit.push_back((BYTE)(
							0b10000000
							| (
								(hexCode & 0xfff) >> 6
							)));
						context.stringLit.push_back((BYTE)(
							0b10000000
							| (hexCode & 0b111111)));
					}
				} else if (hexCode <= 0x10ffff) {
					if (context.stringLit.size() < 128) {
						context.stringLit.push_back((BYTE)(
							0b11110000
							| (hexCode >> 18)));
						context.stringLit.push_back((BYTE)(
							0b10000000
							| (
								(hexCode >> 12) & 0b111111
							)));
						context.stringLit.push_back((BYTE)(
							0b10000000
							| (
								(hexCode >> 6) & 0b111111
							)));
						context.stringLit.push_back((BYTE)(
							0b10000000
							| (hexCode & 0b111111)));
					}
				} else {
					showJsonErr("Invalid hex code in \\u in a string literal", context.lineCount);
					return false;
				}
			}
		}
	}
	
	showJsonErr("Couldn't find closing \" of the string literal", context.lineCount);
	return false;
}

bool checkValidSpriteName(JSONParseContext& context) {
	if (!context.stringLit.empty()) {
		const BYTE* ptr = context.stringLit.data();
		while (*ptr != '\0') {
			BYTE c = *ptr;
			if (c <= 32 || c > 126) {
				showJsonErr("Invalid characters in sprite or anim name. Only ASCII allowed, and no whitespace, not even spaces", context.lineCount);
				return false;
			}
			++ptr;
		}
		return true;
	}
	return false;
}

std::vector<char> joinErrors(const char* const* errorList, size_t errorListSize, const StringWithLength& separator) {
	std::vector<char> result;
	int totalLength = 0;
	const char* const* errorListIter = errorList;
	for (size_t i = 0; i < errorListSize; ++i) {
		if (totalLength == 0) {
			totalLength = strlen(*errorListIter);
		} else {
			totalLength += strlen(*errorListIter) + separator.length;
		}
		++errorListIter;
	}
	result.resize(totalLength + 1);
	totalLength = 0;
	char* errorMsgPtr = result.data();
	errorListIter = errorList;
	for (size_t i = 0; i < errorListSize; ++i) {
		if (totalLength != 0) {
			strcpy_s(errorMsgPtr,
				result.size() - (errorMsgPtr - result.data()),
				separator.txt);
			errorMsgPtr += separator.length;
		} else {
			totalLength = 1;
		}
		strcpy_s(errorMsgPtr,
			result.size() - (errorMsgPtr - result.data()),
			*errorListIter);
		errorMsgPtr += strlen(*errorListIter);
		++errorListIter;
	}
	result.back() = '\0';
	return result;
}

// should I break this up?
bool parseJson(const BYTE* data, size_t* dataSizePtr, std::vector<JSONParsedSprite>& parsedSprites) {
	try {
		const BYTE* const dataStart = data;
		size_t dataSize = *dataSizePtr;
		
		JSONParseContext context;
		context.data = data;
		context.dataSize = dataSize;
		context.lineCount = 1;
		
		enum Mode {
			MODE_START,
			MODE_SPRITE_START,
			MODE_SPRITE_PROPERTIES_START,
			MODE_SPRITE_PROPERTY_NAME,
			MODE_SPRITE_PROPERTY_COLON,
			MODE_SPRITE_PROPERTY_VALUE,
			MODE_SPRITE_PROPERTY_COMMA,
			MODE_BOXES_FIRST_START,
			MODE_BOXES_FIND_NEXT_BOX_START,
			MODE_BOXES_COMMA,
			MODE_BOX_FIRST_PROPERTY_NAME,
			MODE_BOX_PROPERTY_NAME,
			MODE_BOX_COLON,
			MODE_BOX_PROPERTY_VALUE,
			MODE_BOX_COMMA,
			MODE_SPRITE_DELIMITER,
			MODE_AFTER_END
		} mode = MODE_START;
		bool spriteNameRead = false;
		bool spriteAnimRead = false;
		bool spriteBoxesRead = false;
		std::vector<BYTE> spritePropertyName;
		std::vector<BYTE> boxPropertyName;
		bool boxTypeSpecified;
		bool boxXSpecified;
		bool boxYSpecified;
		bool boxWSpecified;
		bool boxHSpecified;
		std::unordered_map<DWORD, std::pair<const BYTE*, int>> spriteHashToJSONStringLiteral;
		char anim[32] { '\0' };
		std::vector<JSONParsedBox> boxes;
		while (*context.data != '\0' && context.dataSize) {
			char c = (char)*context.data;
			context.c = c;
			++context.data;
			--context.dataSize;
			if (
				c == ' '
				|| c == '\t'
				|| c == '\r'
				|| c == '\n'
			) {
				if (c == '\n') {
					++context.lineCount;
				}
				continue;
			}
			if (mode == MODE_START){
				if (c == '[') {
					mode = MODE_SPRITE_START;
				} else {
					showErrorDlgS("Couldn't find opening [ of the entire JSON.");
					return false;
				}
			} else if (mode == MODE_SPRITE_START) {
				if (c == '{') {
					if (parsedSprites.size() == parsedSprites.capacity()) {
						if (parsedSprites.size() < 100) {
							parsedSprites.reserve(300);
						} else {
							parsedSprites.reserve(parsedSprites.capacity() * 2);
						}
					}
					parsedSprites.emplace_back();
					JSONParsedSprite& newSprite = parsedSprites.back();
					mode = MODE_SPRITE_PROPERTIES_START;
					spriteNameRead = false;
					spriteAnimRead = false;
					anim[0] = '\0';
					spriteBoxesRead = false;
					boxes.clear();
				} else if (c == ']') {
					if (parsedSprites.empty()) {
						showErrorDlgS("The JSON is empty. Not sure you would want to load that.");
					} else {
						showErrorDlgS("Unexpected ] after a ,");
					}
					return false;
				} else {
					showJsonErr("Unexpected character. Expected either a { or a ]", context.lineCount);
					return false;
				}
			} else if (mode == MODE_SPRITE_PROPERTIES_START || mode == MODE_SPRITE_PROPERTY_NAME) {
				if (c == '"') {
					if (!JSONReadStringLiteral(context)) return false;
					mode = MODE_SPRITE_PROPERTY_COLON;
					if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "sprite") == 0) {
						if (spriteNameRead) {
							showJsonErr("Duplicate 'sprite' property", context.lineCount);
							return false;
						}
						spriteNameRead = true;
					}
					if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "anim") == 0) {
						if (spriteAnimRead) {
							showJsonErr("Duplicate 'anim' property", context.lineCount);
							return false;
						}
						spriteAnimRead = true;
					}
					if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "boxes") == 0) {
						if (spriteBoxesRead) {
							showJsonErr("Duplicate 'boxes' property", context.lineCount);
							return false;
						}
						spriteBoxesRead = true;
					}
					spritePropertyName = context.stringLit;
				} else if (c == '}') {
					if (mode == MODE_SPRITE_PROPERTY_NAME) {
						showJsonErr("Unexpected } after a ,", context.lineCount);
					} else {
						showJsonErr("Didn't find the \"sprite\" property in a sprite, holding the sprite's name", context.lineCount);
					}
					return false;
				} else if (mode == MODE_SPRITE_PROPERTIES_START) {
					showJsonErr("Unexpected character in a sprite object."
						" Expected either an opening \" for a property name's start or a closing } of the sprite object (and the latter would be invalid anyway)",
						context.lineCount);
					return false;
				} else {
					showJsonErr("Unexpected character in a sprite object."
						" Expected an opening \" for a property name's start",
						context.lineCount);
					return false;
				}
			} else if (mode == MODE_SPRITE_PROPERTY_COLON) {
				if (c == ':') {
					mode = MODE_SPRITE_PROPERTY_VALUE;
				} else {
					showJsonErr("Expected a : after a property name", context.lineCount);
					return false;
				}
			} else if (mode == MODE_SPRITE_PROPERTY_VALUE) {
				if (!spritePropertyName.empty() && strcmp((const char*)spritePropertyName.data(), "sprite") == 0) {
					if (c == '"') {
						const BYTE* stringLiteralStart = context.data - 1;
						if (!JSONReadStringLiteral(context)) return false;
						if (context.stringLit.size() > 32) {
							showJsonErr("Sprite names cannot be more than 31 characters long", context.lineCount);
							return false;
						}
						DWORD hash = Entity::hashStringLowercase((const char*)context.stringLit.data());
						auto found = spriteHashToJSONStringLiteral.find(hash);
						if (found != spriteHashToJSONStringLiteral.end()) {
							int backupLineCount = context.lineCount;
							std::vector<BYTE> thisSpriteName = context.stringLit;
							context.dataSize = context.dataSize - (found->second.first + 1 - context.data);
							context.data = found->second.first + 1;
							context.c = *found->second.first;
							context.lineCount = found->second.second;
							if (!JSONReadStringLiteral(context)) return false;
							sprintf_s(strbuf, "Sprite name conflict. The hash of sprite name '%s', declared on line %d,"
								" matches the hash of sprite name '%s', declared on line %d",
								thisSpriteName.data(),
								backupLineCount,
								context.stringLit.data(),
								found->second.second);
							showErrorDlgS(strbuf);
							return false;
						}
						spriteHashToJSONStringLiteral[hash] = std::pair<const BYTE*, int>{
							stringLiteralStart,
							context.lineCount
						};
						if (!checkValidSpriteName(context)) return false;
						JSONParsedSprite& currentSprite = parsedSprites.back();
						strcpy_s(currentSprite.name,
							32,
							(const char*)context.stringLit.data());
					} else {
						showJsonErr("Sprite name must be a string literal", context.lineCount);
						return false;
					}
					mode = MODE_SPRITE_PROPERTY_COMMA;
				} else if (!spritePropertyName.empty() && strcmp((const char*)spritePropertyName.data(), "anim") == 0) {
					if (c == '"') {
						if (!JSONReadStringLiteral(context)) return false;
						if (context.stringLit.size() > 32) {
							showJsonErr("'anim' cannot be longer than 31 characters long", context.lineCount);
							return false;
						}
						if (!checkValidSpriteName(context)) return false;
						JSONParsedSprite& currentSprite = parsedSprites.back();
						strcpy_s(anim,
							32,
							(const char*)context.stringLit.data());
					} else if (c == 'n') {
						if (!JSONReadNull(context)) return false;
					} else {
						showJsonErr("Anim must be a string literal or null", context.lineCount);
						return false;
					}
					mode = MODE_SPRITE_PROPERTY_COMMA;
				} else if (!spritePropertyName.empty() && strcmp((const char*)spritePropertyName.data(), "boxes") == 0) {
					if (c == '[') {
						mode = MODE_BOXES_FIRST_START;
					} else if (c == 'n') {
						if (!JSONReadNull(context)) return false;
						mode = MODE_SPRITE_PROPERTY_COMMA;
					} else {
						showJsonErr("Boxes must be an array or null", context.lineCount);
						return false;
					}
				} else {
					if (!JSONReadPropertyValue(context)) return false;
					mode = MODE_SPRITE_PROPERTY_COMMA;
				}
			} else if (mode == MODE_SPRITE_PROPERTY_COMMA) {
				if (c == '}') {
					if (!spriteNameRead) {
						showJsonErr("Didn't find the \"sprite\" property in a sprite, holding the sprite's name", context.lineCount);
						return false;
					}
					int jonbinSize = 4  // JONB
						+ 2  // name count
						+ (anim[0] ? 32 : 0)
						+ 1  // number of types
						+ 2 * 0x14  // number of boxes per type
						+ boxes.size() * sizeof (Hitbox);
					JSONParsedSprite& lastSprite = parsedSprites.back();
					lastSprite.jonbin = (BYTE*)malloc(jonbinSize);
					if (!lastSprite.jonbin) {
						throw std::bad_alloc();
					}
					lastSprite.size = jonbinSize;
					#if _DEBUG
					BYTE* jonbinEnd = lastSprite.jonbin + jonbinSize;
					#define writeShort(v) \
						if (jonbin + 1 >= jonbinEnd) throw std::out_of_range("parseJson writing outside of new jonbin"); \
						else *(short*)jonbin = v;
					#else
					#define writeShort(v) *(short*)jonbin = v;
					#endif
					BYTE* jonbin = lastSprite.jonbin;
					#if _DEBUG
					if (jonbin + 3 >= jonbinEnd) throw std::out_of_range("parseJson writing outside of new jonbin");
					#endif
					memcpy(jonbin, "JONB", 4);
					jonbin += 4;
					writeShort(anim[0] ? 1 : 0)
					jonbin += 2;
					if (anim[0]) {
						#if _DEBUG
						if (jonbin + 31 >= jonbinEnd) throw std::out_of_range("parseJson writing outside of new jonbin");
						#endif
						int len = strlen(anim);
						if (len > 31) len = 31;
						memcpy(jonbin, anim, len);
						memset(jonbin + len, 0, 32 - len);
						jonbin += 32;
					}
					#if _DEBUG
					if (jonbin >= jonbinEnd) throw std::out_of_range("parseJson writing outside of new jonbin");
					#endif
					*jonbin = 0x14;
					++jonbin;
					writeShort(0)
					jonbin += 2;
					writeShort(0)
					jonbin += 2;
					writeShort(0)
					jonbin += 2;
					for (int hitboxType = 0; hitboxType < 17; ++hitboxType) {
						writeShort(0)
						for (JSONParsedBox& box : boxes) {
							if (box.type == hitboxType) {
								#if _DEBUG
								if (jonbin >= jonbinEnd) throw std::out_of_range("parseJson writing outside of new jonbin");
								#endif
								++*(short*)jonbin;
							}
						}
						jonbin += 2;
					}
					for (int hitboxType = 0; hitboxType < 17; ++hitboxType) {
						for (JSONParsedBox& box : boxes) {
							if (box.type == hitboxType) {
								#if _DEBUG
								if (jonbin + sizeof (Hitbox) > jonbinEnd) throw std::out_of_range("parseJson writing outside of new jonbin");
								#endif
								Hitbox& hitbox = *(Hitbox*)jonbin;
								hitbox.type = 0;
								hitbox.offX = (float)box.x;
								hitbox.offY = (float)box.y;
								hitbox.sizeX = (float)box.w;
								hitbox.sizeY = (float)box.h;
								jonbin += sizeof Hitbox;
							}
						}
					}
					mode = MODE_SPRITE_DELIMITER;
				} else if (c == ',') {
					mode = MODE_SPRITE_PROPERTY_NAME;
				} else {
					showJsonErr("Unexpected character in a sprite object. Expected either a } or a , after a sprite property", context.lineCount);
					return false;
				}
			} else if (mode == MODE_BOXES_FIRST_START || mode == MODE_BOXES_FIND_NEXT_BOX_START) {
				if (c == ']') {
					if (mode == MODE_BOXES_FIRST_START) {
						mode = MODE_SPRITE_PROPERTY_COMMA;
					} else {
						showJsonErr("Unexpected ] after a ,", context.lineCount);
						return false;
					}
				} else if (c == '{') {
					mode = MODE_BOX_FIRST_PROPERTY_NAME;
					boxTypeSpecified = false;
					boxXSpecified = false;
					boxYSpecified = false;
					boxWSpecified = false;
					boxHSpecified = false;
					if (boxes.size() == boxes.capacity()) {
						if (boxes.size() < 20) boxes.reserve(20);
						else boxes.reserve(boxes.capacity() * 2);
					}
					boxes.emplace_back();
					JSONParsedBox& currentBox = boxes.back();
					memset(&currentBox, 0, sizeof currentBox);
				} else {
					showJsonErr("Incorrect type of an element in the 'boxes' array of a sprite. A box can only be an object", context.lineCount);
					return false;
				}
			} else if (mode == MODE_BOXES_COMMA) {
				if (c == ']') {
					mode = MODE_SPRITE_PROPERTY_COMMA;
				} else if (c == ',') {
					mode = MODE_BOXES_FIND_NEXT_BOX_START;
				} else {
					showJsonErr("Unexpected character. Expected a ] or a , after a previous box", context.lineCount);
					return false;
				}
			} else if (mode == MODE_BOX_FIRST_PROPERTY_NAME || mode == MODE_BOX_PROPERTY_NAME) {
				if (c == '}') {
					if (mode == MODE_BOX_FIRST_PROPERTY_NAME) {
						showJsonErr("A box is supposed to specify a 'type', an 'x', a 'y', a 'w' and an 'h'", context.lineCount);
						return false;
					} else {
						showJsonErr("Unexpected } after a ,", context.lineCount);
						return false;
					}
				} else if (c == '"') {
					if (!JSONReadStringLiteral(context)) return false;
					if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "type") == 0) {
						if (boxTypeSpecified) {
							showJsonErr("Duplicate 'type' property in a box", context.lineCount);
							return false;
						}
						boxTypeSpecified = true;
					} else if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "x") == 0) {
						if (boxXSpecified) {
							showJsonErr("Duplicate 'x' property in a box", context.lineCount);
							return false;
						}
						boxXSpecified = true;
					} else if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "y") == 0) {
						if (boxYSpecified) {
							showJsonErr("Duplicate 'y' property in a box", context.lineCount);
							return false;
						}
						boxYSpecified = true;
					} else if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "w") == 0) {
						if (boxWSpecified) {
							showJsonErr("Duplicate 'w' property in a box", context.lineCount);
							return false;
						}
						boxWSpecified = true;
					} else if (!context.stringLit.empty() && strcmp((const char*)context.stringLit.data(), "h") == 0) {
						if (boxHSpecified) {
							showJsonErr("Duplicate 'h' property in a box", context.lineCount);
							return false;
						}
						boxHSpecified = true;
					}
					boxPropertyName = context.stringLit;
					mode = MODE_BOX_COLON;
				} else if (mode == MODE_BOX_FIRST_PROPERTY_NAME) {
					showJsonErr("Unexpected character in a box object. Expected an opening \" or a } to close the whole box"
						" (don't do that, specify 'type', 'x', 'y', 'w', 'h' first)",
						context.lineCount);
					return false;
				} else {
					showJsonErr("Unexpected character in a box object. Expected an opening \" for a property name",
						context.lineCount);
					return false;
				}
			} else if (mode == MODE_BOX_COLON) {
				if (c == ':') {
					mode = MODE_BOX_PROPERTY_VALUE;
				} else {
					showJsonErr("Unexpected character. Expected : after a box property name", context.lineCount);
					return false;
				}
			} else if (mode == MODE_BOX_PROPERTY_VALUE) {
				enum WhichProperty {
					WHICH_PROPERTY_TYPE,
					WHICH_PROPERTY_X,
					WHICH_PROPERTY_Y,
					WHICH_PROPERTY_W,
					WHICH_PROPERTY_H,
					WHICH_PROPERTY_OTHER
				} whichProp = WHICH_PROPERTY_OTHER;
				if (!boxPropertyName.empty()) {
					const char* str = (const char*)boxPropertyName.data();
					if (strcmp(str, "type") == 0) {
						whichProp = WHICH_PROPERTY_TYPE;
					} else if (strcmp(str, "x") == 0) {
						whichProp = WHICH_PROPERTY_X;
					} else if (strcmp(str, "y") == 0) {
						whichProp = WHICH_PROPERTY_Y;
					} else if (strcmp(str, "w") == 0) {
						whichProp = WHICH_PROPERTY_W;
					} else if (strcmp(str, "h") == 0) {
						whichProp = WHICH_PROPERTY_H;
					}
				}
				if (whichProp == WHICH_PROPERTY_TYPE) {
					if (c == '"') {
						if (!JSONReadStringLiteral(context)) return false;
						int i;
						for (i = 0; i < 17; ++i) {
							if (strcmp(hitboxTypeName[i], (const char*)context.stringLit.data()) == 0) {
								break;
							}
						}
						if (i == 17) {
							std::vector<char> allBoxTypes = joinErrors(hitboxTypeName, "', '");
							sprintf_s(strbuf, "Invalid box type name. Must be one of '%s'", allBoxTypes.data());
							showJsonErr(strbuf, context.lineCount);
							return false;
						} else {
							JSONParsedBox& currentBox = boxes.back();
							currentBox.type = (HitboxType)i;
						}
						mode = MODE_BOX_COMMA;
					} else {
						showJsonErr("Invalid type of 'type' property. It must be a string", context.lineCount);
						return false;
					}
				} else if (whichProp != WHICH_PROPERTY_OTHER) {
					if (!JSONReadNumberLiteral(context)) return false;
					JSONParsedBox& currentBox = boxes.back();
					if (whichProp == WHICH_PROPERTY_X) {
						currentBox.x = (int)context.parsedValue;
					} else if (whichProp == WHICH_PROPERTY_Y) {
						currentBox.y = (int)context.parsedValue;
					} else if (whichProp == WHICH_PROPERTY_W) {
						currentBox.w = (int)context.parsedValue;
					} else {  // h
						currentBox.h = (int)context.parsedValue;
					}
					mode = MODE_BOX_COMMA;
				} else {
					if (!JSONReadPropertyValue(context)) return false;
					mode = MODE_BOX_COMMA;
				}
			} else if (mode == MODE_BOX_COMMA) {
				if (c == '}') {
					std::vector<const char*> myErrors;
					if (!boxTypeSpecified) {
						myErrors.push_back("'type' property missing from a box");
					}
					if (!boxXSpecified) {
						myErrors.push_back("'x' property missing from a box");
					}
					if (!boxYSpecified) {
						myErrors.push_back("'y' property missing from a box");
					}
					if (!boxWSpecified) {
						myErrors.push_back("'w' property missing from a box");
					}
					if (!boxHSpecified) {
						myErrors.push_back("'h' property missing from a box");
					}
					if (!myErrors.empty()) {
						std::vector<char> errorMsg = joinErrors(myErrors, ". ");
						showJsonErr(errorMsg.data(), context.lineCount);
						return false;
					}
					mode = MODE_BOXES_COMMA;
				} else if (c == ',') {
					mode = MODE_BOX_PROPERTY_NAME;
				} else {
					showJsonErr("Unexpected character. Expected a } or , after a box property value", context.lineCount);
					return false;
				}
			} else if (mode == MODE_SPRITE_DELIMITER) {
				if (c == ',') {
					mode = MODE_SPRITE_START;
				} else if (c == ']') {
					mode = MODE_AFTER_END;
				} else {
					showJsonErr("Expected either a , after the previous sprite or a closing ] of the entire JSON", context.lineCount);
					return false;
				}
			} else if (mode == MODE_AFTER_END) {
				showErrorDlgS("Unexpected data after the closing ] of the entire JSON.");
				return false;
			}
		}
		if (mode == MODE_START) {
			showErrorDlgS("Couldn't find opening [ of the entire JSON.");
			return false;
		}
		*dataSizePtr = context.data - dataStart;
		return true;
	} catch (std::length_error& err) {
		(err);
		showErrorDlgS("The JSON is too big?");
		return false;
	} catch (std::bad_alloc& err) {
		(err);
		showErrorDlgS("Ran out of memory while prasing JSON. The JSON is too big?");
		return false;
	}
}
