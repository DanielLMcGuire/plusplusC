if (NOT TARGET minicrt)
    set(MINICRT_CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/def.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/alloc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/alloc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/atexit.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atexit.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/str.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/str.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mem.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mem.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/sig.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sig.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/sock.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sock.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/mutex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mutex.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/atomic.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/thread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cond.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cond.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sem.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sem.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/clock.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/clock.h
        $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../platform/win32/emutls.c>

        ${CMAKE_CURRENT_SOURCE_DIR}/src/dstr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dstr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iarr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/iarr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/parr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/map.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/map.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/format.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cio.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/path.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/path.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/proc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/proc.h
    )

    add_library(minicrt_objects OBJECT ${MINICRT_CORE_SOURCES})
    target_include_directories(minicrt_objects PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/platform>
    )
    target_compile_definitions(minicrt_objects PUBLIC _CORECRT_BUILD)
    if (WIN32)
        target_compile_definitions(minicrt_objects PUBLIC WIN32_LEAN_AND_MEAN _WINSOCKAPI_)
    endif()
    xxc_freestanding_compile_options(minicrt_objects PUBLIC)

    add_library(minicrt STATIC
        $<TARGET_OBJECTS:minicrt_objects>
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crt1.c
    )
    add_library(xxc::minicrt ALIAS minicrt)

    set(MINICRT_TARGETS minicrt)
    add_library(minicrt_shared SHARED
        $<TARGET_OBJECTS:minicrt_objects>
        $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/src/crt1.c>
    )
    set_target_properties(minicrt_shared PROPERTIES
        RUNTIME_OUTPUT_NAME minicrt
        LIBRARY_OUTPUT_NAME minicrt
        ARCHIVE_OUTPUT_NAME minicrt_shared
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    target_compile_definitions(minicrt_shared PRIVATE XXC_BUILDING_SHARED_LIB)
    add_library(xxc::minicrt_shared ALIAS minicrt_shared)
    list(APPEND MINICRT_TARGETS minicrt_shared)

    if (LINUX)
        target_link_libraries(minicrt PUBLIC sys_linux)
        if (TARGET minicrt_shared)
            target_link_libraries(minicrt_shared PUBLIC sys_linux)
        endif()

        if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|i.86|x86" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_link_libraries(minicrt PUBLIC gcc)
            if (TARGET minicrt_shared)
                target_link_libraries(minicrt_shared PUBLIC gcc)
            endif()
        endif()
    elseif (XXC_FREEBSD)
        target_link_libraries(minicrt PUBLIC sys_freebsd)
        if (TARGET minicrt_shared)
            target_link_libraries(minicrt_shared PUBLIC sys_freebsd)
        endif()

        if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(XXC_BUILTINS_LIB "compiler_rt" CACHE STRING "Compiler builtins library to link on FreeBSD aarch64")
            target_link_libraries(minicrt PUBLIC ${XXC_BUILTINS_LIB})
            set(XXC_BUILTINS_LIB_SHARED "" CACHE STRING "PIC compiler builtins library for minicrt_shared (FreeBSD aarch64)")
            if (TARGET minicrt_shared AND XXC_BUILTINS_LIB_SHARED)
                target_link_libraries(minicrt_shared PUBLIC ${XXC_BUILTINS_LIB_SHARED})
            endif()
        endif()
    elseif (WIN32)
        target_link_libraries(minicrt PUBLIC sys_win32 kernel32 ws2_32)
        if (TARGET minicrt_shared)
            target_link_libraries(minicrt_shared PUBLIC sys_win32_dll)
        endif()
    endif()

    foreach(tgt IN LISTS MINICRT_TARGETS)
        target_include_directories(${tgt} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal>
        )
        target_compile_definitions(${tgt} PUBLIC _CORECRT_BUILD)
        if (WIN32)
            target_compile_definitions(${tgt} PUBLIC WIN32_LEAN_AND_MEAN _WINSOCKAPI_)
        endif()
        xxc_freestanding_compile_options(${tgt} PUBLIC)
    endforeach()

    if (TARGET minicrt_shared)
        xxc_freestanding_link_options(minicrt_shared)
    endif()

    # Installation rules for CPack
    install(TARGETS ${MINICRT_TARGETS}
            LIBRARY DESTINATION lib COMPONENT libxxc
            ARCHIVE DESTINATION lib COMPONENT libxxc-headers
            RUNTIME DESTINATION bin COMPONENT libxxc)

    install(DIRECTORY include/ 
            DESTINATION include/minicrt 
            COMPONENT libxxc-headers)
            
    install(DIRECTORY internal/ 
            DESTINATION include/minicrt/internal 
            COMPONENT libxxc-headers)
endif()