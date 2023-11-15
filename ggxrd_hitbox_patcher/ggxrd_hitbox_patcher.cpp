// ggxrd_hitbox_patcher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>

wchar_t szFile[MAX_PATH];
wchar_t szFileBackup[MAX_PATH];

bool hasAtLeastOneBackslash(const wchar_t* str) {
    const wchar_t* ptr = str;
    while (*ptr != L'\0') {
        if (*ptr == L'\\') return true;
        ++ptr;
    }
    return false;
}

wchar_t determineSeparator(const std::wstring& path) {
    if (hasAtLeastOneBackslash(path.c_str())) {
        return L'\\';
    }
    return L'/';
}

int findLast(const std::wstring& str, wchar_t character) {
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
std::wstring getParentDir(const std::wstring& path) {
    std::wstring result;
    int lastSlashPos = findLast(path, determineSeparator(path));
    if (lastSlashPos == -1) return result;
    result.insert(result.begin(), path.cbegin(), path.cbegin() + lastSlashPos);
    return result;
}

std::wstring getFileName(const std::wstring& path) {
    std::wstring result;
    int lastSlashPos = findLast(path, determineSeparator(path));
    if (lastSlashPos == -1) return path;
    result.insert(result.begin(), path.cbegin() + lastSlashPos + 1, path.cend());
    return result;
}

bool fileExists(const std::wstring& path) {
    DWORD fileAtrib = GetFileAttributesW(path.c_str());
    if (fileAtrib == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
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
                _wperror(L"Error reading from file");
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
                _wperror(L"Error reading from file");
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
                _wperror(L"Error reading file");
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
        _wperror(L"Error writing to file");
        return false;
    }
    return true;
}

int calculateRelativeCall(uintptr_t callInstructionAddress, uintptr_t calledAddress) {
    return (int)calledAddress - (int)(callInstructionAddress + 5);
}

uintptr_t followRelativeCall(uintptr_t callInstructionAddress, const char* callInstructionAddressInRam) {
    int offset = *(int*)(callInstructionAddressInRam + 1);
    return callInstructionAddress + 5 + offset;
}

struct Section {
    std::string name;
    uintptr_t relativeVirtualAddress = 0;
    uintptr_t virtualAddress = 0;
    uintptr_t virtualSize = 0;
    uintptr_t rawAddress = 0;
    uintptr_t rawSize = 0;
};

std::vector<Section> readSections(FILE* file) {

    std::vector<Section> result;

    uintptr_t peHeaderStart = 0;
    fseek(file, 0x3C, SEEK_SET);
    fread(&peHeaderStart, 4, 1, file);

    unsigned short numberOfSections = 0;
    fseek(file, peHeaderStart + 0x6, SEEK_SET);
    fread(&numberOfSections, 2, 1, file);

    uintptr_t optionalHeaderStart = peHeaderStart + 0x18;

    unsigned short optionalHeaderSize = 0;
    fseek(file, peHeaderStart + 0x14, SEEK_SET);
    fread(&optionalHeaderSize, 2, 1, file);

    uintptr_t imageBase = 0;
    fseek(file, peHeaderStart + 0x34, SEEK_SET);
    fread(&imageBase, 4, 1, file);

    uintptr_t sectionsStart = optionalHeaderStart + optionalHeaderSize;
    uintptr_t sectionStart = sectionsStart;
    for (size_t sectionCounter = numberOfSections; sectionCounter != 0; --sectionCounter) {
        Section newSection;
        fseek(file, sectionStart, SEEK_SET);
        newSection.name.resize(8);
        fread(&newSection.name.front(), 1, 8, file);
        newSection.name.resize(strlen(newSection.name.c_str()));
        fread(&newSection.virtualSize, 4, 1, file);
        fread(&newSection.relativeVirtualAddress, 4, 1, file);
        newSection.virtualAddress = imageBase + newSection.relativeVirtualAddress;
        fread(&newSection.rawSize, 4, 1, file);
        fread(&newSection.rawAddress, 4, 1, file);
        result.push_back(newSection);
        sectionStart += 40;
    }

    return result;
}

uintptr_t rawToVa(const std::vector<Section>& sections, uintptr_t rawAddr) {
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

uintptr_t vaToRaw(const std::vector<Section>& sections, uintptr_t va) {
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

void writeRelocEntry(FILE* file, char relocType, uintptr_t va) {
    uintptr_t vaMemoryPage = va & 0xFFFFF000;
    unsigned short relocEntry = (relocType << 12) | ((va - vaMemoryPage) & 0x0FFF);
    fwrite(&relocEntry, 2, 1, file);
}

void meatOfTheProgram() {
    std::wstring ignoreLine;
    std::wcout << L"Please select a path to your GuiltyGearXrd.exe file that will be patched...\n";

    std::wstring szFile;
    szFile.resize(MAX_PATH);

    OPENFILENAMEW selectedFiles{ 0 };
    selectedFiles.lStructSize = sizeof(OPENFILENAMEW);
    selectedFiles.hwndOwner = NULL;
    selectedFiles.lpstrFile = &szFile.front();
    selectedFiles.lpstrFile[0] = L'\0';
    selectedFiles.nMaxFile = szFile.size() + 1;
    selectedFiles.lpstrFilter = L"Windows Executable\0*.EXE\0";
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
    std::wcout << "Selected file: " << szFile.c_str() << std::endl;

    wchar_t separator = determineSeparator(szFile);
    std::wstring fileName = getFileName(szFile);
    std::wstring parentDir = getParentDir(szFile);

    std::wstring backupFilePath = parentDir + separator + fileName + L"_backup";
    int backupNameCounter = 1;
    while (fileExists(backupFilePath)) {
        backupFilePath = parentDir + separator + fileName + L"_backup" + std::to_wstring(backupNameCounter);
        ++backupNameCounter;
    }

    std::wcout << "Will use backup file path: " << backupFilePath.c_str() << std::endl;

    if (!CopyFileW(szFile.c_str(), backupFilePath.c_str(), true)) {
        std::wcout << "Failed to create a backup copy. Do you want to continue anyway? You won't be able to revert the file to the original. Press Enter to agree...\n";
        std::getline(std::wcin, ignoreLine);
    } else {
        std::wcout << "Backup copy created successfully.\n";
    }

    FILE* file = nullptr;
    if (_wfopen_s(&file, szFile.c_str(), L"r+b") || !file) {
        _wperror(L"Failed to open file");
        return;
    }

    std::vector<char> wholeFile;
    if (!readWholeFile(file, wholeFile)) return;
    char* wholeFileBegin = &wholeFile.front();
    char* wholeFileEnd = &wholeFile.front() + wholeFile.size();

    // sig for ghidra: b8 ?? ?? ?? ?? eb 05 b8 ?? ?? ?? ?? 50 e8 ?? ?? ?? ?? 83 c4 04 8b f0 39 1d ?? ?? ?? ?? 75 1a eb 06
    uintptr_t patchingPlace = sigscan(wholeFileBegin, wholeFileEnd,
        "\xb8\x00\x00\x00\x00\xeb\x05\xb8\x00\x00\x00\x00\x50\xe8\x00\x00\x00\x00\x83\xc4\x04\x8b\xf0\x39\x1d\x00\x00\x00\x00\x75\x1a\xeb\x06",
        "x????xxx????xx????xxxxxxx????xxxx");
    if (!patchingPlace) {
        std::wcout << "Failed to find patching place\n";
        return;
    }
    patchingPlace += 13;
    std::wcout << "Found patching place: 0x" << std::hex << patchingPlace << std::dec << std::endl;

    std::string stringToWrite = "ggxrd_hitbox_overlay.dll";
    // thunk_... functions have huge regions of these
    std::string sig = repeatCharNTimes('\xCC', stringToWrite.size() + 1);
    std::string mask = repeatCharNTimes('x', stringToWrite.size() + 1);

    uintptr_t stringInsertionPlace = sigscan(wholeFileBegin, wholeFileEnd,
        sig.c_str(),
        mask.c_str());
    if (!stringInsertionPlace) {
        std::wcout << "Failed to find string insertion place\n";
        return;
    }
    std::wcout << "Found string insertion place: 0x" << std::hex << stringInsertionPlace << std::dec << std::endl;

    if (!writeStringToFile(file, stringInsertionPlace, stringToWrite, wholeFileBegin + stringInsertionPlace)) return;

    // ghidra sig: ff 75 c8 ff 15 ?? ?? ?? ?? 8b f8 85 ff 75 41 ff 15 ?? ?? ?? ?? 89 45 dc a1 ?? ?? ?? ?? 85 c0 74 0e
    uintptr_t loadLibraryAPlace = sigscan(wholeFileBegin, wholeFileEnd,
        "\xff\x75\xc8\xff\x15\x00\x00\x00\x00\x8b\xf8\x85\xff\x75\x41\xff\x15\x00\x00\x00\x00\x89\x45\xdc\xa1\x00\x00\x00\x00\x85\xc0\x74\x0e",
        "xxxxx????xxxxxxxx????xxxx????xxxx");
    if (!loadLibraryAPlace) {
        std::wcout << "Failed to find LoadLibraryA calling place\n";
        return;
    }
    uintptr_t loadLibraryAPtr = *(uintptr_t*)(wholeFileBegin + loadLibraryAPlace + 5);
    std::wcout << "Found LoadLibraryA pointer value: 0x" << std::hex << loadLibraryAPtr << std::dec << std::endl;

    // JMP rel32: e9 [little endian 4 bytes relative address of the destination from the instruction after the jmp]
    // CALL rel32: e8 [little endian 4 bytes relative address of the destination from the instruction after the jmp]
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

    uintptr_t codeInsertionPlace = sigscan(wholeFileBegin, wholeFileEnd,
        sig.c_str(),
        mask.c_str());
    if (!codeInsertionPlace) {
        std::wcout << "Failed to find code insertion place\n";
        return;
    }
    std::wcout << "Found code insertion place: 0x" << std::hex << codeInsertionPlace << std::dec << std::endl;

    std::vector<Section> sections = readSections(file);
    if (sections.empty()) {
        std::wcout << "Failed to read sections\n";
        return;
    }
    std::wcout << "Read sections: [\n";
    std::wcout << std::hex;
    bool isFirst = true;
    for (const Section& section : sections) {
        if (!isFirst) {
            std::wcout << ",\n";
        }
        isFirst = false;
        std::wcout << "{\n\tname: \"" << section.name.c_str() << "\""
            << ",\n\tvirtualSize: 0x" << section.virtualSize
            << ",\n\tvirtualAddress: 0x" << section.virtualAddress
            << ",\n\trawSize: 0x" << section.rawSize
            << ",\n\trawAddress: 0x" << section.rawAddress
            << "\n}";
    }
    std::wcout << "\n]\n";
    std::wcout << std::dec;

    uintptr_t codeInsertionPlacePtr = codeInsertionPlace;
    fseek(file, codeInsertionPlacePtr, SEEK_SET);
    fwrite("\xE8", 1, 1, file);
    uintptr_t callAddress = followRelativeCall(patchingPlace, wholeFileBegin + patchingPlace);
    int newOffset = calculateRelativeCall(codeInsertionPlacePtr, callAddress);
    fwrite(&newOffset, 4, 1, file);
    codeInsertionPlacePtr += 5;
    uintptr_t stringInsertionPlaceVa = rawToVa(sections, stringInsertionPlace);
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

    uintptr_t peHeaderStart = 0;
    fseek(file, 0x3C, SEEK_SET);
    fread(&peHeaderStart, 4, 1, file);

    fseek(file, peHeaderStart + 0xA0, SEEK_SET);
    uintptr_t relocRva = 0;
    fread(&relocRva, 4, 1, file);
    size_t relocSize = 0;
    fread(&relocSize, 4, 1, file);
    fseek(file, -4, SEEK_CUR);
    size_t newRelocSize = relocSize + 12;  // we'll insert to relocation table one block for the PUSH string and the LoadLibraryA call
    fwrite(&newRelocSize, 4, 1, file);

    uintptr_t relocInFile = rvaToRaw(sections, relocRva) + relocSize;
    fseek(file, relocInFile, SEEK_SET);


    uintptr_t codeInsertionPlaceVa = rawToVa(sections, codeInsertionPlace);
    uintptr_t newRelocBlockRva = codeInsertionPlaceVa & 0xFFFFF000;
    fwrite(&newRelocBlockRva, 4, 1, file);
    size_t newRelocBlockSize = 12;
    fwrite(&newRelocBlockSize, 4, 1, file);
    writeRelocEntry(file, 3, codeInsertionPlaceVa + 6);  // codeInsertionPlace + 6 is the pointer to string in our PUSH instruction
    writeRelocEntry(file, 3, codeInsertionPlaceVa + 12);  // codeInsertionPlace + 12 is the [KERNEL32.DLL::LoadLibraryA] in our CALL instruction

    std::wcout << "Patched successfully\n";

    fclose(file);

}

int main()
{
    std::wcout << L"This program patches GuiltyGearXrd.exe executable to permanently launch this hitbox overlay mod when it starts.\n"
                  L"This cannot be undone, and you should backup your GuiltyGearXrd.exe file before proceeding.\n"
                  L"A backup copy will also be automatically created (but that may fail).\n"
                  L"(!) The ggxrd_hitbox_overlay.dll must be placed in the same folder as the game executable in order to get loaded by the game. (!)\n"
                  L"(!) If the DLL is not present in the folder with the game the game will just run normally, without the mod. (!)\n"
                  L"Only Guilty Gear Xrd Rev2 version 2211 supported.\n"
                  L"Press Enter when ready...\n";

    std::wstring ignoreLine;
    std::getline(std::wcin, ignoreLine);

    meatOfTheProgram();

    std::wcout << L"Press Enter to exit...\n";
    std::getline(std::wcin, ignoreLine);
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
