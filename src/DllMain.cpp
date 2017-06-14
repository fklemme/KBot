#include <BWAPI.h>

#include "KBot.h"

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
// Windows Header Files
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void gameInit(BWAPI::Game *game) {
    BWAPI::BroodwarPtr = game;
}

extern "C" __declspec(dllexport) BWAPI::AIModule *newAIModule() {
    return new KBot::KBot;
}
