function(xxc_freestanding_compile_options target visibility)
    if (WIN32)
        if (MSVC)
            target_compile_options(${target} ${visibility}
                /O2
                /GS-
                /Gs9999999
                /guard:cf-
                /d2CastGuard-
            )
        elseif (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target} ${visibility}
                -O3
                -ffreestanding
                -fno-builtin
                -fno-stack-protector
                -mno-stack-arg-probe
            )
        endif()
    elseif (LINUX)
        if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target} ${visibility}
                -O3
                -flto
                -ffreestanding
                -fno-builtin
                -fno-stack-protector
                -ftls-model=initial-exec
            )

            if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
                target_compile_options(${target} ${visibility}
                    -mno-outline-atomics
                )
            endif()
        endif()
    endif()
endfunction()

function(xxc_freestanding_link_options target)
    get_target_property(tgt_type ${target} TYPE)

    if (WIN32)
        target_link_libraries(${target} PRIVATE kernel32 ws2_32)

        if (MSVC)
            target_link_options(${target} PRIVATE
                /OPT:REF
                /OPT:ICF
                /GS-
                /guard:cf-
                /d2CastGuard-
                /sdl-
                /RTCc- /RTCs- /RTCu- /RTC1-
                /Zl
                /Gs9999999
            )
            if (NOT tgt_type STREQUAL "SHARED_LIBRARY")
                target_link_options(${target} PRIVATE /ENTRY:start /SUBSYSTEM:CONSOLE)
            endif()
        elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            target_link_options(${target} PRIVATE
                -O3
                -nostdlib
            )
            if (NOT tgt_type STREQUAL "SHARED_LIBRARY")
                target_link_options(${target} PRIVATE /ENTRY:start /SUBSYSTEM:CONSOLE)
            endif()
        elseif (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(${target} PRIVATE -O3 -nostdlib)
            if (NOT tgt_type STREQUAL "SHARED_LIBRARY")
                if (MINGW OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
                    target_link_options(${target} PRIVATE
                        "LINKER:-e,start"
                        "LINKER:--subsystem,console"
                    )
                else()
                    target_link_options(${target} PRIVATE
                        "LINKER:/ENTRY:start"
                        "LINKER:/SUBSYSTEM:CONSOLE"
                    )
                endif()
            endif()
        endif()
    elseif (LINUX)
        if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(${target} PRIVATE
                -O3
                -flto
                -nostdlib
                -fno-builtin
            )
            if (NOT tgt_type STREQUAL "SHARED_LIBRARY")
                target_link_options(${target} PRIVATE -static -no-pie)
            endif()
        endif()
    endif()
endfunction()