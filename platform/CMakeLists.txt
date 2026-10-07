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

    add_library(sys_linux OBJECT 
        linux/syscall.c 
        shared/tls.c 
        ${ARCH_SRCS} 
        shared/alloc.c
    )

    target_include_directories(sys_linux PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    target_include_directories(sys_linux PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/internal
    )

    xxc_freestanding_compile_options(sys_linux PRIVATE)

    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/xxc_sys.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_linux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_alloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_thread.h
        DESTINATION include/minicrt
        COMPONENT libxxc-headers)
    install(DIRECTORY linux/ DESTINATION include/minicrt COMPONENT libxxc-headers FILES_MATCHING PATTERN "*.h")

    add_library(minicrt_crt0 OBJECT linux/crt0_${CMAKE_SYSTEM_PROCESSOR_NORM}.s)
    install(FILES $<TARGET_OBJECTS:minicrt_crt0>
            DESTINATION lib/minicrt
            RENAME crt0.o
            COMPONENT libxxc-headers)
elseif (XXC_FREEBSD AND NOT TARGET sys_freebsd)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        set(CMAKE_SYSTEM_PROCESSOR_NORM x86_64)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CMAKE_SYSTEM_PROCESSOR_NORM aarch64)
    else()
        message(FATAL_ERROR "++C: unsupported FreeBSD architecture: ${CMAKE_SYSTEM_PROCESSOR} (x86_64 and aarch64 only)")
    endif()

    add_library(sys_freebsd OBJECT
        freebsd/syscall.c
        freebsd/crt0_${CMAKE_SYSTEM_PROCESSOR_NORM}.s
        freebsd/syscall_${CMAKE_SYSTEM_PROCESSOR_NORM}.s
        freebsd/thread_${CMAKE_SYSTEM_PROCESSOR_NORM}.s
        shared/tls.c
        shared/alloc.c
    )

    target_include_directories(sys_freebsd PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    target_include_directories(sys_freebsd PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/internal
    )

    xxc_freestanding_compile_options(sys_freebsd PRIVATE)

    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/xxc_sys.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_freebsd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_alloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sys_thread.h
        DESTINATION include/minicrt
        COMPONENT libxxc-headers)

    add_library(minicrt_crt0 OBJECT freebsd/crt0_${CMAKE_SYSTEM_PROCESSOR_NORM}.s)
    install(FILES $<TARGET_OBJECTS:minicrt_crt0>
            DESTINATION lib/minicrt
            RENAME crt0.o
            COMPONENT libxxc-headers)
elseif (WIN32 AND NOT TARGET sys_win32)
    add_library(sys_win32 OBJECT win32/crt0.c win32/tls.c)
    add_library(sys_win32_dll OBJECT win32/dllmain.c win32/tls.c)

    foreach(target IN ITEMS sys_win32 sys_win32_dll)
        target_include_directories(${target} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../libminicrt/internal
        )

        target_compile_definitions(${target} PRIVATE
            _CORECRT_BUILD
            WIN32_LEAN_AND_MEAN
        )

        if (XXC_DEMO_USES_SHARED OR XXC_TESTS_USES_SHARED)
            target_compile_definitions(${target} PRIVATE XXC_USING_SHARED_LIB)
        endif()

        xxc_freestanding_compile_options(${target} PRIVATE)
    endforeach()
endif()