// ++C C Runtime Library (libminicrt) | Platform (win32)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef CL_WIN32_H
#define CL_WIN32_H

#include <windows.h>
#include <shellapi.h>

#include <def.h>

// alloc.h
extern void* malloc(size_t size);
extern void free(void *ptr);

static inline char** GetArgvA(int* pArgc)
{
    int argc = 0;
    LPWSTR cmdLine = GetCommandLineW();
    LPWSTR* argvW = CommandLineToArgvW(cmdLine, &argc);
    if (!argvW) 
    {
        if (pArgc) *pArgc = 0;
        return NULL; 
    }

    char** argv = (char**)malloc((argc + 1) * sizeof(char*));

    if (!argv)
    { 
        LocalFree(argvW); 
        if (pArgc) *pArgc = 0; return NULL; 
    }

    for (int i = 0; i < argc; i++)
    {
        int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, NULL, 0, NULL, NULL);
        argv[i] = (char*)malloc(size);
        if (argv[i]) WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], size, NULL, NULL);
    }

    argv[argc] = NULL;
    LocalFree(argvW);
    if (pArgc) *pArgc = argc;
    return argv;
}

static inline void FreeArgvA(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
}

#endif