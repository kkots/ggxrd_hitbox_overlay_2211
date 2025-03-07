// pch.h: This is NOT a precompiled header file.
// The patcher does not use precompiled headers, because it has to also be compiled for Linux.

#ifndef PCH_H
#define PCH_H

#ifndef FOR_LINUX
#include <Windows.h>
#else
typedef unsigned int DWORD;
typedef void* LPVOID;
#endif

#endif //PCH_H
