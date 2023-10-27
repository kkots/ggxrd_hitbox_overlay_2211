// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "logging.h"
#include "Detouring.h"
#include "Graphics.h"
#include "Game.h"
#include "Direct3DVTable.h"
#include "EndScene.h"
#include "HitDetector.h"
#include "Entity.h"
#include "AltModes.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        #ifdef LOG_PATH
        {
            errno_t err;
            err = _wfopen_s(&logfile, LOG_PATH, L"wt");
            if (err != 0 || logfile == NULL) {
                return FALSE;
            }
            fputs("DllMain called with fdwReason DLL_PROCESS_ATTACH\n", logfile);
            fclose(logfile);
        }
        #endif
        if (!detouring.beginTransaction()) break;
        if (!game.onDllMain()) break;
        if (!entityManager.onDllMain()) break;
        if (!direct3DVTable.onDllMain()) break;
        if (!endScene.onDllMain()) break;
        if (!hitDetector.onDllMain()) break;
        if (!graphics.onDllMain()) break;
        if (!altModes.onDllMain()) break;
        if (!detouring.endTransaction()) break;
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        logwrap(fputs("DLL_PROCESS_DETACH\n", logfile));
        detouring.onDllDetach();
        graphics.onDllDetach();
        break;
    }
    return TRUE;
}
