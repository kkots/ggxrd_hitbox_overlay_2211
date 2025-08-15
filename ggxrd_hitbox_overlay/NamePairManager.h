#pragma once
#include <string>
#include <list>
#include <unordered_map>
#include "Moves.h"
class NamePairManager
{
public:
	static const NamePair* getPair(const char* name);
private:
	static const int nameArSize = 100;
	struct NamePairMapElement {
		std::string nameAr[nameArSize] { };
		NamePair pairAr[nameArSize];
		int count = 0;
		void add(const char* name);
	};
	static unsigned int hashString(const char* str);
	struct NamePairMapHashFunction {
		inline std::size_t operator()(const char* key) const {
			return hashString(key);
		}
	};
	struct NamePairMapCompareFunction {
		inline bool operator()(const char* key, const char* other) const {
			return strcmp(key, other) == 0;
		}
	};
	// static basically means extern here
	static std::list<NamePairMapElement> namePairList;
	static std::unordered_map<const char*, const NamePair*, NamePairMapHashFunction, NamePairMapCompareFunction> namePairMap;
};
