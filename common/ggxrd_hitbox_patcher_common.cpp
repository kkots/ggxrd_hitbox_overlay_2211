#include "pch.h"
#include "ggxrd_hitbox_patcher_common.h"
// The purpose of this program is to patch GuiltyGearXrd.exe to add instructions to it so that
// it loads the ggxrd_hitbox_overlay.dll on startup automatically
#include <iostream>
#include <string>
#ifndef FOR_LINUX
#include "InjectorCommonOut.h"
#include <commdlg.h>
#else
#include <fstream>
#include <string.h>
#include <stdint.h>
#endif
#include <vector>

#ifndef FOR_LINUX
InjectorCommonOut outputObject;
#define CrossPlatformString std::wstring
#define CrossPlatformChar wchar_t
#define CrossPlatformPerror _wperror
#define CrossPlatformText(txt) L##txt
#define CrossPlatformCout outputObject
#define CrossPlatformNumberToString std::to_wstring
#else
#define CrossPlatformString std::string
#define CrossPlatformChar char
#define CrossPlatformPerror perror
#define CrossPlatformText(txt) txt
#define CrossPlatformCout std::cout
#define CrossPlatformNumberToString std::to_string
typedef unsigned int DWORD;
#endif

extern void GetLine(CrossPlatformString& line);

char exeName[] = "\x3d\x6b\x5f\x62\x6a\x6f\x3d\x5b\x57\x68\x4e\x68\x5a\x24\x5b\x6e\x5b\xf6";

bool hasAtLeastOneBackslash(const CrossPlatformString& path) {
    
    for (auto it = path.cbegin(); it != path.cend(); ++it) {
        if (*it == (CrossPlatformChar)'\\') return true;
    }
    return false;
}

CrossPlatformChar determineSeparator(const CrossPlatformString& path) {
    if (hasAtLeastOneBackslash(path)) {
        return (CrossPlatformChar)'\\';
    }
    return (CrossPlatformChar)'/';
}

int findLast(const CrossPlatformString& str, CrossPlatformChar character) {
    if (str.empty()) return -1;
    auto it = str.cend();
    --it;
    while (true) {
        if (*it == character) return it - str.cbegin();
        if (it == str.cbegin()) return -1;
        --it;
    }
    return -1;
}

// Does not include trailing slash
CrossPlatformString getParentDir(const CrossPlatformString& path) {
    CrossPlatformString result;
    int lastSlashPos = findLast(path, determineSeparator(path));
    if (lastSlashPos == -1) return result;
    result.insert(result.begin(), path.cbegin(), path.cbegin() + lastSlashPos);
    return result;
}

CrossPlatformString getFileName(const CrossPlatformString& path) {
    CrossPlatformString result;
    int lastSlashPos = findLast(path, determineSeparator(path));
    if (lastSlashPos == -1) return path;
    result.insert(result.begin(), path.cbegin() + lastSlashPos + 1, path.cend());
    return result;
}

bool fileExists(const CrossPlatformString& path) {
    #ifndef FOR_LINUX
    DWORD fileAtrib = GetFileAttributesW(path.c_str());
    if (fileAtrib == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
    #else
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return false;
    fclose(file);
    return true;
    #endif
}

int sigscan(const char* start, const char* end, const char* sig, const char* mask) {
    const char* startPtr = start;
    const size_t maskLen = strlen(mask);
    const size_t seekLength = end - start - maskLen + 1;
    for (size_t seekCounter = seekLength; seekCounter != 0; --seekCounter) {
        const char* stringPtr = startPtr;

        const char* sigPtr = sig;
        for (const char* maskPtr = mask; true; ++maskPtr) {
            const char maskPtrChar = *maskPtr;
            if (maskPtrChar != '?') {
                if (maskPtrChar == '\0') return startPtr - start;
                if (*sigPtr != *stringPtr) break;
            }
            ++sigPtr;
            ++stringPtr;
        }
        ++startPtr;
    }
    return -1;
}

uintptr_t sigscan(FILE* file, const char* sig, const char* mask) {
    class DoThisWhenExiting {
    public:
        DoThisWhenExiting(FILE* file, size_t originalPos) : file(file), originalPos(originalPos) { }
        ~DoThisWhenExiting() {
            fseek(file, originalPos, SEEK_SET);
        }
        FILE* file = nullptr;
        size_t originalPos = 0;
    } doThisWhenExiting(file, ftell(file));

    fseek(file, 0, SEEK_SET);
    char buffer[2048] = { '\0' };
    bool isFirst = true;
    size_t maskLen = strlen(mask);
    uintptr_t currentFilePosition = 0;

    while (true) {
        size_t readBytes;
        if (isFirst) {
            readBytes = fread(buffer, 1, 1024, file);
        } else {
            readBytes = fread(buffer + 1024, 1, 1024, file);
        }
        if (readBytes == 0) {
            if (ferror(file)) {
                CrossPlatformPerror(CrossPlatformText("Error reading from file"));
            }
            // assume it's feof
            break;
        }

        if (isFirst) {
            int pos = sigscan(buffer, buffer + readBytes, sig, mask);
            if (pos != -1) {
                return (uintptr_t)pos;
            }
        } else {
            int pos = sigscan(buffer + 1024 - maskLen + 1, buffer + 1024 + readBytes, sig, mask);
            if (pos != -1) {
                return currentFilePosition + (uintptr_t)pos - maskLen + 1;
            }
        }

        if (readBytes < 1024) {
            if (ferror(file)) {
                CrossPlatformPerror(CrossPlatformText("Error reading from file"));
            }
            // assume it's feof
            break;
        }

        if (!isFirst) {
            memcpy(buffer, buffer + 1024, 1024);
        }
        isFirst = false;

        currentFilePosition += 1024;
    }
    // we didn't open the file, so we're not the ones who should close it
    return 0;
}

bool readWholeFile(FILE* file, std::vector<char>& wholeFile) {
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    wholeFile.resize(fileSize);
    char* wholeFilePtr = &wholeFile.front();
    size_t readBytesTotal = 0;
    while (true) {
        size_t sizeToRead = 1024;
        if (fileSize - readBytesTotal < 1024) sizeToRead = fileSize - readBytesTotal;
        if (sizeToRead == 0) break;
        size_t readBytes = fread(wholeFilePtr, 1, sizeToRead, file);
        if (readBytes != sizeToRead) {
            if (ferror(file)) {
                CrossPlatformPerror(CrossPlatformText("Error reading file"));
                return false;
            }
            // assume feof
            break;
        }
        wholeFilePtr += 1024;
        readBytesTotal += 1024;
    }
    return true;
}

std::string repeatCharNTimes(char charToRepeat, int times) {
    std::string result;
    result.resize(times, charToRepeat);
    return result;
}

bool writeStringToFile(FILE* file, size_t pos, const std::string& stringToWrite, char* fileLocationInRam) {
    memcpy(fileLocationInRam, stringToWrite.c_str(), stringToWrite.size() + 1);
    fseek(file, pos, SEEK_SET);
    size_t writtenBytes = fwrite(stringToWrite.c_str(), 1, stringToWrite.size() + 1, file);
    if (writtenBytes != stringToWrite.size() + 1) {
        CrossPlatformPerror(CrossPlatformText("Error writing to file"));
        return false;
    }
    return true;
}

int calculateRelativeCall(DWORD callInstructionAddress, DWORD calledAddress) {
    return (int)calledAddress - (int)(callInstructionAddress + 5);
}

DWORD followRelativeCall(DWORD callInstructionAddress, const char* callInstructionAddressInRam) {
    int offset = *(int*)(callInstructionAddressInRam + 1);
    return callInstructionAddress + 5 + offset;
}

struct Section {
    std::string name;
	
	// RVA. Virtual address offset relative to the virtual address start of the entire .exe.
	// So let's say the whole .exe starts at 0x400000 and RVA is 0x400.
	// That means the non-relative VA is 0x400000 + RVA = 0x400400.
	// Note that the .exe, although it does specify a base virtual address for itself on the disk,
	// may actually be loaded anywhere in the RAM once it's launched, and that RAM location will
	// become its base virtual address.
    DWORD relativeVirtualAddress = 0;
    
	// VA. Virtual address within the .exe.
	// A virtual address is the location of something within the .exe once it's loaded into memory.
	// An on-disk, file .exe is usually smaller than when it's loaded so it creates this distinction
	// between raw address and virtual address.
    DWORD virtualAddress = 0;
    
	// The size in terms of virtual address space.
    DWORD virtualSize = 0;
    
	// Actual position of the start of this section's data within the file.
    DWORD rawAddress = 0;
    
	// Size of this section's data on disk in the file.
    DWORD rawSize = 0;
};

std::vector<Section> readSections(FILE* file, DWORD* imageBase) {

    std::vector<Section> result;

    DWORD peHeaderStart = 0;
    fseek(file, 0x3C, SEEK_SET);
    fread(&peHeaderStart, 4, 1, file);

    unsigned short numberOfSections = 0;
    fseek(file, peHeaderStart + 0x6, SEEK_SET);
    fread(&numberOfSections, 2, 1, file);

    DWORD optionalHeaderStart = peHeaderStart + 0x18;

    unsigned short optionalHeaderSize = 0;
    fseek(file, peHeaderStart + 0x14, SEEK_SET);
    fread(&optionalHeaderSize, 2, 1, file);

    fseek(file, peHeaderStart + 0x34, SEEK_SET);
    fread(imageBase, 4, 1, file);

    DWORD sectionsStart = optionalHeaderStart + optionalHeaderSize;
    DWORD sectionStart = sectionsStart;
    for (size_t sectionCounter = numberOfSections; sectionCounter != 0; --sectionCounter) {
        Section newSection;
        fseek(file, sectionStart, SEEK_SET);
        newSection.name.resize(8);
        fread(&newSection.name.front(), 1, 8, file);
        newSection.name.resize(strlen(newSection.name.c_str()));
        fread(&newSection.virtualSize, 4, 1, file);
        fread(&newSection.relativeVirtualAddress, 4, 1, file);
        newSection.virtualAddress = *imageBase + newSection.relativeVirtualAddress;
        fread(&newSection.rawSize, 4, 1, file);
        fread(&newSection.rawAddress, 4, 1, file);
        result.push_back(newSection);
        sectionStart += 40;
    }

    return result;
}

DWORD rawToVa(const std::vector<Section>& sections, DWORD rawAddr) {
    if (sections.empty()) return 0;
    auto it = sections.cend();
    --it;
    while (true) {
        const Section& section = *it;
        if (rawAddr >= section.rawAddress) {
            return rawAddr - section.rawAddress + section.virtualAddress;
        }
        if (it == sections.cbegin()) break;
        --it;
    }
    return 0;
}

DWORD vaToRaw(const std::vector<Section>& sections, DWORD va) {
    if (sections.empty()) return 0;
    auto it = sections.cend();
    --it;
    while (true) {
        const Section& section = *it;
        if (va >= section.virtualAddress) {
            return va - section.virtualAddress + section.rawAddress;
        }
        if (it == sections.cbegin()) break;
        --it;
    }
    return 0;
}

DWORD vaToRva(DWORD va, DWORD imageBase) {
	return va - imageBase;
}

DWORD rvaToVa(DWORD rva, DWORD imageBase) {
	return rva + imageBase;
}

uintptr_t rvaToRaw(const std::vector<Section>& sections, uintptr_t rva) {
    if (sections.empty()) return 0;
    auto it = sections.cend();
    --it;
    while (true) {
        const Section& section = *it;
        if (rva >= section.relativeVirtualAddress) {
            return rva - section.relativeVirtualAddress + section.rawAddress;
        }
        if (it == sections.cbegin()) break;
        --it;
    }
    return 0;
}

/// <summary>
/// Writes a relocation entry at the current file position.
/// </summary>
/// <param name="relocType">See macros starting with IMAGE_REL_BASED_</param>
/// <param name="va">Virtual address of the place to be relocated by the reloc.</param>
void writeRelocEntry(FILE* file, char relocType, DWORD va) {
    DWORD vaMemoryPage = va & 0xFFFFF000;
    unsigned short relocEntry = (relocType << 12) | ((va - vaMemoryPage) & 0x0FFF);
    fwrite(&relocEntry, 2, 1, file);
}

#ifdef FOR_LINUX
void trimLeft(std::string& str) {
    if (str.empty()) return;
	auto it = str.begin();
	while (it != str.end() && *it <= 32) ++it;
	str.erase(str.begin(), it);
}

void trimRight(std::string& str) {
    if (str.empty()) return;
    auto it = str.end();
    --it;
    while (true) {
        if (*it > 32) break;
        if (it == str.begin()) {
            str.clear();
            return;
        }
        --it;
    }
    str.resize(it - str.begin() + 1);
}
#endif

bool crossPlatformOpenFile(FILE** file, const CrossPlatformString& path) {
	#ifndef FOR_LINUX
	errno_t errorCode = _wfopen_s(file, path.c_str(), CrossPlatformText("r+b"));
	if (errorCode != 0 || !*file) {
		if (errorCode != 0) {
			wchar_t buf[1024];
			_wcserror_s(buf, errorCode);
			CrossPlatformCout << L"Failed to open file: " << buf << L'\n';
		} else {
			CrossPlatformCout << L"Failed to open file.\n";
		}
		if (*file) {
			fclose(*file);
		}
		return false;
	}
	return true;
	#else
	*file = fopen(path.c_str(), "r+b");
	if (!*file) {
		perror("Failed to open file");
		return false;
	}
	return true;
	#endif
}

#ifdef FOR_LINUX
void copyFileLinux(const std::string& pathSource, const std::string& pathDestination) {
    std::ifstream src(pathSource, std::ios::binary);
    std::ofstream dst(pathDestination, std::ios::binary);

    dst << src.rdbuf();
}
#endif

void meatOfTheProgram() {
    CrossPlatformString ignoreLine;
	#ifndef FOR_LINUX
    CrossPlatformCout << CrossPlatformText("Please select a path to your ") << exeName << CrossPlatformText(" file that will be patched...\n");
	#else
	CrossPlatformCout << CrossPlatformText("Please type in/paste a path to your ") << exeName << CrossPlatformText(" file"
		" (including the file name and extension) that will be patched...\n");
	#endif

    #ifndef FOR_LINUX
    std::wstring szFile;
    szFile.resize(MAX_PATH);

    OPENFILENAMEW selectedFiles{ 0 };
    selectedFiles.lStructSize = sizeof(OPENFILENAMEW);
    selectedFiles.hwndOwner = NULL;
    selectedFiles.lpstrFile = &szFile.front();
    selectedFiles.lpstrFile[0] = L'\0';
    selectedFiles.nMaxFile = szFile.size() + 1;
    // it says "Windows Executable\0*.EXE\0"
	char scramble[] =
		"\x4d\xf6\x5f\xf6\x64\xf6\x5a\xf6\x65\xf6\x6d\xf6\x69\xf6\x16\xf6\x3b\xf6"
		"\x6e\xf6\x5b\xf6\x59\xf6\x6b\xf6\x6a\xf6\x57\xf6\x58\xf6\x62\xf6\x5b\xf6"
		"\xf6\xf6\x20\xf6\x24\xf6\x3b\xf6\x4e\xf6\x3b\xf6\xf6\xf6\xf6\xf6";
	wchar_t filter[(sizeof scramble - 1) / sizeof (wchar_t)];
	int offset = (int)(
		(GetTickCount64() & 0xF000000000000000ULL) >> (63 - 4)
	);
	for (int i = 0; i < sizeof scramble - 1; ++i) {
		char c = scramble[i] + offset + 10;
		((char*)filter)[i] = c;
	}
    selectedFiles.lpstrFilter = filter;
    selectedFiles.nFilterIndex = 1;
    selectedFiles.lpstrFileTitle = NULL;
    selectedFiles.nMaxFileTitle = 0;
    selectedFiles.lpstrInitialDir = NULL;
    selectedFiles.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameW(&selectedFiles)) {
        DWORD errCode = CommDlgExtendedError();
        if (!errCode) {
            std::wcout << "The file selection dialog was closed by the user.\n";
        } else {
            std::wcout << "Error selecting file. Error code: 0x" << std::hex << errCode << std::dec << std::endl;
        }
        return;
    }
    szFile.resize(lstrlenW(szFile.c_str()));
    #else
    std::string szFile;
    GetLine(szFile);
    trimLeft(szFile);
    trimRight(szFile);
    if (!szFile.empty() && (szFile[0] == '\'' || szFile[0] == '"')) {
    	szFile.erase(szFile.begin());
    }
    if (!szFile.empty() && (szFile[szFile.size() - 1] == '\'' || szFile[szFile.size() - 1] == '"')) {
    	szFile.erase(szFile.begin() + (szFile.size() - 1));
    }
    if (szFile.empty()) {
        std::cout << "Empty path provided. Aborting.\n";
        return;
    }
    #endif
    CrossPlatformCout << "Selected file: " << szFile.c_str() << std::endl;

    CrossPlatformChar separator = determineSeparator(szFile);
    CrossPlatformString fileName = getFileName(szFile);
    CrossPlatformString parentDir = getParentDir(szFile);

    CrossPlatformString backupFilePath = parentDir + separator + fileName + CrossPlatformText("_backup");
    int backupNameCounter = 1;
    while (fileExists(backupFilePath)) {
        backupFilePath = parentDir + separator + fileName + CrossPlatformText("_backup") + CrossPlatformNumberToString(backupNameCounter);
        ++backupNameCounter;
    }

    CrossPlatformCout << "Will use backup file path: " << backupFilePath.c_str() << std::endl;

    #ifndef FOR_LINUX
    if (!CopyFileW(szFile.c_str(), backupFilePath.c_str(), true)) {
        std::wcout << "Failed to create a backup copy. Do you want to continue anyway? You won't be able to revert the file to the original. Press Enter to agree...\n";
    	GetLine(ignoreLine);
    } else {
        std::wcout << "Backup copy created successfully.\n";
    }
    #else
    copyFileLinux(szFile, backupFilePath);
    std::wcout << "Backup copy created successfully.\n";
    #endif

    FILE* file = nullptr;
    if (!crossPlatformOpenFile(&file, szFile)) return;

    std::vector<char> wholeFile;
    if (!readWholeFile(file, wholeFile)) return;
    char* wholeFileBegin = &wholeFile.front();
    char* wholeFileEnd = &wholeFile.front() + wholeFile.size();

    // sig for ghidra: b8 ?? ?? ?? ?? eb 05 b8 ?? ?? ?? ?? 50 e8 ?? ?? ?? ?? 83 c4 04 8b f0 39 1d ?? ?? ?? ?? 75 1a eb 06
    int patchingPlace = sigscan(wholeFileBegin, wholeFileEnd,
        "\xb8\x00\x00\x00\x00\xeb\x05\xb8\x00\x00\x00\x00\x50\xe8\x00\x00\x00\x00\x83\xc4\x04\x8b\xf0\x39\x1d\x00\x00\x00\x00\x75\x1a\xeb\x06",
        "x????xxx????xx????xxxxxxx????xxxx");
    if (patchingPlace == -1) {
        CrossPlatformCout << "Failed to find patching place\n";
        return;
    }
    patchingPlace += 13;
    CrossPlatformCout << "Found patching place: 0x" << std::hex << patchingPlace << std::dec << std::endl;

    std::string stringToWrite = "ggxrd_hitbox_overlay.dll";
    // thunk_... functions have huge regions of these
    std::string sig = repeatCharNTimes('\xCC', stringToWrite.size() + 1);
    std::string mask = repeatCharNTimes('x', stringToWrite.size() + 1);

    int stringInsertionPlace = sigscan(wholeFileBegin, wholeFileEnd,
        sig.c_str(),
        mask.c_str());
    if (stringInsertionPlace == -1) {
        CrossPlatformCout << "Failed to find string insertion place\n";
        return;
    }
    CrossPlatformCout << "Found string insertion place: 0x" << std::hex << stringInsertionPlace << std::dec << std::endl;

    if (!writeStringToFile(file, stringInsertionPlace, stringToWrite, wholeFileBegin + stringInsertionPlace)) return;

    // ghidra sig: ff 75 c8 ff 15 ?? ?? ?? ?? 8b f8 85 ff 75 41 ff 15 ?? ?? ?? ?? 89 45 dc a1 ?? ?? ?? ?? 85 c0 74 0e
    int loadLibraryAPlace = sigscan(wholeFileBegin, wholeFileEnd,
        "\xff\x75\xc8\xff\x15\x00\x00\x00\x00\x8b\xf8\x85\xff\x75\x41\xff\x15\x00\x00\x00\x00\x89\x45\xdc\xa1\x00\x00\x00\x00\x85\xc0\x74\x0e",
        "xxxxx????xxxxxxxx????xxxx????xxxx");
    if (loadLibraryAPlace == -1) {
        CrossPlatformCout << "Failed to find LoadLibraryA calling place\n";
        return;
    }
    DWORD loadLibraryAPtr = *(DWORD*)(wholeFileBegin + loadLibraryAPlace + 5);
    CrossPlatformCout << "Found LoadLibraryA pointer value: 0x" << std::hex << loadLibraryAPtr << std::dec << std::endl;

    // JMP rel32: e9 [little endian 4 bytes relative address of the destination from the instruction after the jmp]
    // CALL rel32: e8 [little endian 4 bytes relative address of the destination from the instruction after the call]
    // PUSH stringAddr: 0x68 [little endian 4 bytes absolute address of the string]
    // CALL [KERNEL32.DLL::LoadLibraryA]: 0xff 0x15 THE_VALUE_OF_LoadLibraryA_POINTER
    // RET: 0xC3
    
    size_t requiredCodeSize = 0
        + 5 // CALL what it was going to call with new relative offset (we should have enough int +- range)
        + 5 // PUSH STRING_ADDR
        + 6 // CALL [KERNEL32.DLL::LoadLibraryA]
        + 5 // JMP rel32
        ;

    sig = repeatCharNTimes('\xCC', requiredCodeSize);
    mask = repeatCharNTimes('x', requiredCodeSize);

    int codeInsertionPlace = sigscan(wholeFileBegin, wholeFileEnd,
        sig.c_str(),
        mask.c_str());
    if (codeInsertionPlace == -1) {
        CrossPlatformCout << "Failed to find code insertion place\n";
        return;
    }
    CrossPlatformCout << "Found code insertion place: 0x" << std::hex << codeInsertionPlace << std::dec << std::endl;
	
	DWORD imageBase;
    std::vector<Section> sections = readSections(file, &imageBase);
    if (sections.empty()) {
        CrossPlatformCout << "Failed to read sections\n";
        return;
    }
    CrossPlatformCout << "Read sections: [\n";
    CrossPlatformCout << std::hex;
    bool isFirst = true;
    for (const Section& section : sections) {
        if (!isFirst) {
            CrossPlatformCout << ",\n";
        }
        isFirst = false;
        CrossPlatformCout << "{\n\tname: \"" << section.name.c_str() << "\""
            << ",\n\tvirtualSize: 0x" << section.virtualSize
            << ",\n\tvirtualAddress: 0x" << section.virtualAddress
            << ",\n\trawSize: 0x" << section.rawSize
            << ",\n\trawAddress: 0x" << section.rawAddress
            << "\n}";
    }
    CrossPlatformCout << "\n]\n";
    CrossPlatformCout << std::dec;

    DWORD codeInsertionPlacePtr = (DWORD)codeInsertionPlace;
    fseek(file, codeInsertionPlacePtr, SEEK_SET);
    fwrite("\xE8", 1, 1, file);
    DWORD callAddress = followRelativeCall(patchingPlace, wholeFileBegin + patchingPlace);
    int newOffset = calculateRelativeCall(codeInsertionPlacePtr, callAddress);
    fwrite(&newOffset, 4, 1, file);
    codeInsertionPlacePtr += 5;
    DWORD stringInsertionPlaceVa = rawToVa(sections, stringInsertionPlace);
    fwrite("\x68", 1, 1, file);
    fwrite(&stringInsertionPlaceVa, 4, 1, file);
    codeInsertionPlacePtr += 5;
    fwrite("\xff\x15", 1, 2, file);
    fwrite(&loadLibraryAPtr, 4, 1, file);
    codeInsertionPlacePtr += 6;
    int jumpOffset = calculateRelativeCall(codeInsertionPlacePtr, patchingPlace + 5);
    fwrite("\xE9", 1, 1, file);
    fwrite(&jumpOffset, 4, 1, file);

    fseek(file, patchingPlace, SEEK_SET);
    fwrite("\xE9", 1, 1, file);
    jumpOffset = calculateRelativeCall(patchingPlace, codeInsertionPlace);
    fwrite(&jumpOffset, 4, 1, file);

    DWORD peHeaderStart = 0;
    fseek(file, 0x3C, SEEK_SET);
    fread(&peHeaderStart, 4, 1, file);

    fseek(file, peHeaderStart + 0xA0, SEEK_SET);
    DWORD relocRva = 0;
    fread(&relocRva, 4, 1, file);
    DWORD relocSize = 0;
    fread(&relocSize, 4, 1, file);
    fseek(file, -4, SEEK_CUR);
    // "Each block must start on a 32-bit boundary." - Microsoft
    DWORD relocSizeRoundUp = (relocSize + 3) & ~3;
    DWORD newRelocSize = relocSizeRoundUp + 12;  // into the relocation table we'll insert both the PUSH string and the LoadLibraryA call into a single block
    fwrite(&newRelocSize, 4, 1, file);

    DWORD relocInFile = rvaToRaw(sections, relocRva) + relocSizeRoundUp;
    fseek(file, relocInFile, SEEK_SET);


    DWORD codeInsertionPlaceVa = rawToVa(sections, codeInsertionPlace);
    DWORD newRelocBlockRva = vaToRva(codeInsertionPlaceVa & 0xFFFFF000, imageBase);
    fwrite(&newRelocBlockRva, 4, 1, file);
    DWORD newRelocBlockSize = 12;
    fwrite(&newRelocBlockSize, 4, 1, file);
    writeRelocEntry(file, 3, codeInsertionPlaceVa + 6);  // codeInsertionPlace + 6 is the pointer to string in our PUSH instruction
    writeRelocEntry(file, 3, codeInsertionPlaceVa + 12);  // codeInsertionPlace + 12 is the [KERNEL32.DLL::LoadLibraryA] in our CALL instruction

    CrossPlatformCout << "Patched successfully\n";

    fclose(file);

}

int patcherMain()
{
	
	#ifndef FOR_LINUX
	int offset = (int)(
		(GetTickCount64() & 0xF000000000000000ULL) >> (63 - 4)
	);
	#else
	int offset = 0;
	#endif
	
	for (int i = 0; i < sizeof exeName - 1; ++i) {
		exeName[i] += offset + 10;
	}
	
    CrossPlatformCout <<
                  "This program patches " << exeName << " executable to permanently launch this hitbox overlay mod when it starts.\n"
                  "This cannot be undone, and you should backup your " << exeName << " file before proceeding.\n"
                  "A backup copy will also be automatically created (but that may fail).\n"
                  "(!) The ggxrd_hitbox_overlay.dll must be placed in the same folder as the game executable in order to get loaded by the game. (!)\n"
                  "(!) If the DLL is not present in the folder with the game the game will just run normally, without the mod. (!)\n"
                  "Only Guilty Gear Xrd Rev2 version 2211 supported.\n"
                  "Press Enter when ready...\n";

    CrossPlatformString ignoreLine;
    GetLine(ignoreLine);

    meatOfTheProgram();

    CrossPlatformCout << "Press Enter to exit...\n";
    GetLine(ignoreLine);
    return 0;
}
