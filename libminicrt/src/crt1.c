// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <atexit.h>
#include <cio.h>
#include <parr.h>
#include <alloc.h>

extern int program(parr_t csArgs);

extern void sio_init(unsigned int out, unsigned int err);

extern void xxc_io_init(void);

#ifdef _WIN32
#include <windows.h>

XXC_NORETURN void pluspluscBoot(int argc, char **argv)
{
    sio_init(STD_OUTPUT_HANDLE, STD_ERROR_HANDLE);

    xxc_io_init(); 

    parr_t args = parr_new((const void**)argv, (size_t)argc);

    int ret = program(args);
    
    parr_free(&args);
    /* GCOV_EXCL_START */
    exit(ret);
    __NORETURN__
    /* GCOV_EXCL_STOP */
}

#elif defined(XXC_RAWSYS)
#include <xxc_sys.h>

XXC_NORETURN void pluspluscBoot(int argc, char **argv)
{
    __xxc_platform_init(argc, argv);
    sio_init(0, 0);
    xxc_io_init();
    parr_t args = parr_new((const void**)argv, (size_t)argc);
    int ret = program(args);
    parr_free(&args);
    /* GCOV_EXCL_START */
    exit(ret);
    __NORETURN__
    /* GCOV_EXCL_STOP */
}

#endif