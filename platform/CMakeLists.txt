if (LINUX AND NOT TARGET sys_linux)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(CMAKE_SYSTEM_PROCESSOR_NORM x86_64)
        set(ARCH_SRCS
            linux/crt0_x86_64.s
            linux/syscall_x86_64.s
            linux/thread_x86_64.s
        )
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i.86|x86")
        set(CMAKE_SYSTEM_PROCESSOR_NORM i386)
        set(ARCH_SRCS
            linux/crt0_i386.s
            linux/syscall_i386.s
            linux/thread_i386.s
        )
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CMAKE_SYSTEM_PROCESSOR_NORM aarch64)
        set(ARCH_SRCS
            linux/crt0_aarch64.s
            linux/syscall_aarch64.s
            linux/thread_aarch64.s
        )
    else()
        message(FATAL_ERROR "++C: unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    add_library(sys_linux OBJECT linux/syscall.c linux/tls.c ${ARCH_SRCS} linux/alloc.c)

    target_include_directories(sys_linux PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    target_include_directories(sys_linux PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/internal
    )

    xxc_freestanding_compile_options(sys_linux PRIVATE)

    install(DIRECTORY linux/ DESTINATION include/minicrt COMPONENT libxxc-headers FILES_MATCHING PATTERN "*.h")

    add_library(minicrt_crt0 OBJECT linux/crt0_${CMAKE_SYSTEM_PROCESSOR_NORM}.s)
    install(FILES $<TARGET_OBJECTS:minicrt_crt0>
            DESTINATION lib/minicrt
            RENAME crt0.o
            COMPONENT libxxc-headers)
elseif (WIN32 AND NOT TARGET sys_win32)
    add_library(sys_win32 OBJECT win32/crt0.c)

    target_include_directories(sys_win32 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    target_compile_definitions(sys_win32 PRIVATE
        _CORECRT_BUILD
        WIN32_LEAN_AND_MEAN
    )

    xxc_freestanding_compile_options(sys_win32 PRIVATE)
endif()