// ++C C Runtime Library (libminicrt) | Platform (win32)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#define va_start vastart_guard
#include <windows.h>
#undef va_start

volatile int _fltused = 0;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)hinst;
    (void)reason;
    (void)reserved;
    return TRUE;
}

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    return DllMain(hinst, reason, reserved);
}
