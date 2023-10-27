#pragma once
class Direct3DVTable
{
public:
	bool onDllMain();
	char** getDirect3DVTable() const;
private:
	char** vTableAddr = nullptr;
};

extern Direct3DVTable direct3DVTable;
