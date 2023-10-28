#pragma once
#include <vector>

class Detouring
{
public:
	bool beginTransaction();
	bool attach(PVOID* ppPointer, PVOID pDetour, const char* name = nullptr);
	bool endTransaction();
	bool cancelTransaction();
	void onDllDetach();
private:
	struct ThingToBeUndetouredAtTheEnd {
		PVOID* ppPointer = nullptr;
		PVOID pDetour = nullptr;
		const char* name = nullptr;
	};
	std::vector<ThingToBeUndetouredAtTheEnd> thingsToUndetourAtTheEnd;
	void printDetourTransactionBeginError(LONG err);
	void printDetourUpdateThreadError(LONG err);
	void printDetourAttachError(LONG err);
	void printDetourDetachError(LONG err);
	void printDetourTransactionCommitError(LONG err);
	void detachAll();
	bool beganTransaction = false;
};

extern Detouring detouring;
