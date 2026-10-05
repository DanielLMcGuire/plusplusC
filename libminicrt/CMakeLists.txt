if (NOT TARGET minicrt)
    set(MINICRT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crt1.c

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
        $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/src/emutls.c>

        ${CMAKE_CURRENT_SOURCE_DIR}/src/dstr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dstr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iarr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/iarr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/parr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/map.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/map.h

        ${CMAKE_CURRENT_SOURCE_DIR}/src/format.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/include/cio.h
    )

    add_library(minicrt STATIC ${MINICRT_SOURCES})
    add_library(minicrt_shared SHARED ${MINICRT_SOURCES})
    
    add_library(xxc::minicrt ALIAS minicrt)
    set_target_properties(minicrt_shared PROPERTIES OUTPUT_NAME minicrt)

    if (LINUX)
        target_link_libraries(minicrt PUBLIC sys_linux)
        target_link_libraries(minicrt_shared PUBLIC sys_linux)

        if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|i.86|x86" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_link_libraries(minicrt PUBLIC gcc)
            target_link_libraries(minicrt_shared PUBLIC gcc)
        endif()
    elseif (WIN32)
        target_link_libraries(minicrt PUBLIC sys_win32 kernel32 ws2_32)
        target_link_libraries(minicrt_shared PUBLIC sys_win32 kernel32 ws2_32)
    endif()

    foreach(tgt minicrt minicrt_shared)
        target_include_directories(${tgt} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal>
        )
        target_compile_definitions(${tgt} PUBLIC _CORECRT_BUILD)
        if (WIN32)
            target_compile_definitions(${tgt} PUBLIC WIN32_LEAN_AND_MEAN)
        endif()
        xxc_freestanding_compile_options(${tgt} PUBLIC)
    endforeach()

    xxc_freestanding_link_options(minicrt_shared)

    # Installation rules for CPack
    install(TARGETS minicrt minicrt_shared
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