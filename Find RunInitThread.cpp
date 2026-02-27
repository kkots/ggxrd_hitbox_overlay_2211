
struct FileCloser {
	~FileCloser() {
		if (dllFile && dllFile != INVALID_HANDLE_VALUE) CloseHandle(dllFile);
	}
	HANDLE dllFile = NULL;
} fileCloser;

fileCloser.dllFile = CreateFileW(dll_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
if (!fileCloser.dllFile || fileCloser.dllFile == INVALID_HANDLE_VALUE) {
	outputObject << L"Failed to open ggxrd_hitbox_overlay.dll to locate RunInitThread.\n";
	return false;
}

HANDLE file = fileCloser.dllFile;
DWORD bytesRead = 0;

IMAGE_DOS_HEADER dosHeader;
if (!readFile(file, &dosHeader, sizeof IMAGE_DOS_HEADER, &bytesRead)) return false;
if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
	outputObject << L"ggxrd_hitbox_overlay.dll is not a valid DLL.\n";
	return false;
}


SetFilePointer(file, dosHeader.e_lfanew, NULL, FILE_BEGIN);
IMAGE_NT_HEADERS32 ntHeader;
if (!readFile(file, &ntHeader, sizeof IMAGE_NT_HEADERS32, &bytesRead)) return false;
if (ntHeader.Signature != IMAGE_NT_SIGNATURE) {
	outputObject << L"ggxrd_hitbox_overlay.dll is not a valid DLL.\n";
	return false;
}

std::vector<IMAGE_SECTION_HEADER> sections(ntHeader.FileHeader.NumberOfSections);
SetFilePointer(file,
	dosHeader.e_lfanew
	+ offsetof(IMAGE_NT_HEADERS32, OptionalHeader)
	+ ntHeader.FileHeader.SizeOfOptionalHeader, NULL, FILE_BEGIN);
if (!readFile(file, sections.data(), sizeof (IMAGE_SECTION_HEADER) * ntHeader.FileHeader.NumberOfSections, &bytesRead)) return false;

struct AddrStruct {
	DWORD rawToRva(DWORD raw) {
		for (int i = (int)sections.size() - 1; i >= 0; --i) {
			const IMAGE_SECTION_HEADER& section = sections[i];
			if (raw >= section.PointerToRawData) {
				return raw - section.PointerToRawData + section.VirtualAddress;
			}
		}
		return 0;
	}
	DWORD rvaToRaw(DWORD rva) {
		for (int i = (int)sections.size() - 1; i >= 0; --i) {
			const IMAGE_SECTION_HEADER& section = sections[i];
			if (rva >= section.VirtualAddress) {
				return rva - section.VirtualAddress + section.PointerToRawData;
			}
		}
		return 0;
	}
	inline DWORD rawToVa(DWORD raw) {
		return rawToRva(raw) + imageBase;
	}
	inline DWORD vaToRaw(DWORD va) {
		return rvaToRaw(va - imageBase);
	}
	const std::vector<IMAGE_SECTION_HEADER>& sections;
	const DWORD imageBase;
} Addr{sections, ntHeader.OptionalHeader.ImageBase};

SetFilePointer(file, Addr.rvaToRaw(ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), NULL, FILE_BEGIN);
int dirSizeRemaining = (int)ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
if (ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > 0) {
	IMAGE_EXPORT_DIRECTORY exportDir;
	if (!readFile(file, &exportDir, sizeof IMAGE_EXPORT_DIRECTORY, &bytesRead)) return false;
	int numNamesCounter = (int)exportDir.NumberOfNames;
	int numFunctionsCounter = (int)exportDir.NumberOfFunctions;
	int currentLookupIndex = 0;
	while (numNamesCounter >= 0 && numFunctionsCounter >= 0) {
		--numNamesCounter;
		--numFunctionsCounter;
		
		SetFilePointer(file, Addr.rvaToRaw(
			exportDir.AddressOfNames
			+ sizeof (DWORD) * currentLookupIndex
		), NULL, FILE_BEGIN);
		
		DWORD nameRva;
		if (!readFile(file, &nameRva, 4, &bytesRead)) return false;
		
		SetFilePointer(file, Addr.rvaToRaw(nameRva), NULL, FILE_BEGIN);
		std::vector<char> nameData;
		while (true) {
			char nextChar = '\0';
			if (!readFile(file, &nextChar, 1, &bytesRead)) return false;
			if (nextChar == '\0') break;
			nameData.push_back(nextChar);
		}
		nameData.push_back('\0');
		
		if (strcmp(nameData.data(), "RunInitThread") == 0) {
			
			SetFilePointer(file, Addr.rvaToRaw(
				exportDir.AddressOfNameOrdinals
				+ sizeof (WORD) * currentLookupIndex
			), NULL, FILE_BEGIN);
			WORD ordinalUnbiased;
			if (!readFile(file, &ordinalUnbiased, 2, &bytesRead)) return false;
			// the bias stored in exportDir.Base is for outsiders, I guess
			
			
			SetFilePointer(file, Addr.rvaToRaw(
				exportDir.AddressOfFunctions
				+ sizeof (DWORD) * ordinalUnbiased
			), NULL, FILE_BEGIN);
			DWORD functionOff;
			if (!readFile(file, &functionOff, 4, &bytesRead)) return false;
			
			// found the function RVA
			return functionOff;
			
		}
		
		++currentLookupIndex;
	}	
}

// if you end up here, you failed to find the function