#include "pch.h"
#include "NamePairManager.h"

std::list<NamePairManager::NamePairMapElement> NamePairManager::namePairList;
std::unordered_map<const char*, const NamePair*, NamePairManager::NamePairMapHashFunction, NamePairManager::NamePairMapCompareFunction> NamePairManager::namePairMap;

unsigned int NamePairManager::hashString(const char* str) {
	unsigned int hash = 0;
	for (const char* c = str; *c != '\0'; ++c) {
		hash = hash * 0x89 + *c;
	}
	return hash;
}

void NamePairManager::NamePairMapElement::add(const char* name) {
	nameAr[count] = name;
	pairAr[count] = NamePair {
		nameAr[count].c_str(),
		nullptr
	};
	++count;
}

const NamePair* NamePairManager::getPair(const char* name) {
	auto found = namePairMap.find(name);
	if (found == namePairMap.end()) {
		if (namePairList.empty() || namePairList.back().count >= nameArSize) {
			namePairList.emplace_back();
			NamePairMapElement& newElement = namePairList.back();
			newElement.count = 0;
		}
		
		NamePairMapElement& lastElement = namePairList.back();
		lastElement.add(name);
		
		const NamePair& lastPair = lastElement.pairAr[lastElement.count - 1];
		namePairMap[lastPair.name] = &lastPair;
		return &lastPair;
	} else {
		return found->second;
	}
}
