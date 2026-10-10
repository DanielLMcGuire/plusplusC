if (NOT TARGET xxc)
    set(XXC_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/argparse.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stream.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileStream.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FSPath.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryStream.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/socket.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Map.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Runnable.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Thread.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessStream.ppc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Process.ppc
    )
    set_source_files_properties(${XXC_SOURCES} PROPERTIES LANGUAGE C)

    add_library(xxc_objects OBJECT ${XXC_SOURCES})
    target_include_directories(xxc_objects PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libminicrt/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/platform>
    )
    if (WIN32)
        target_compile_definitions(xxc_objects PUBLIC WIN32_LEAN_AND_MEAN _WINSOCKAPI_)
    endif()
    xxc_freestanding_compile_options(xxc_objects PUBLIC)

    add_library(xxc STATIC $<TARGET_OBJECTS:xxc_objects>
        ${CMAKE_CURRENT_SOURCE_DIR}/include/class.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/app.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/argparse.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/stream.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/files.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/fs.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mems.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/map.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/runnable.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread.pph
        ${CMAKE_CURRENT_SOURCE_DIR}/include/process.pph
    )
    add_library(xxc::xxc ALIAS xxc)

    set(XXC_TARGETS xxc)
    add_library(xxc_shared SHARED $<TARGET_OBJECTS:xxc_objects>)
    set_target_properties(xxc_shared PROPERTIES
        RUNTIME_OUTPUT_NAME xxc
        LIBRARY_OUTPUT_NAME xxc
        ARCHIVE_OUTPUT_NAME xxc_shared
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    target_compile_definitions(xxc_shared PRIVATE XXC_BUILDING_SHARED_LIB)
    add_library(xxc::xxc_shared ALIAS xxc_shared)
    list(APPEND XXC_TARGETS xxc_shared)

    target_link_libraries(xxc PUBLIC xxc::minicrt)
    if (TARGET xxc_shared)
        target_link_libraries(xxc_shared PUBLIC minicrt_shared)
        if (WIN32)
            target_link_libraries(xxc_shared PUBLIC sys_win32_dll)
        endif()
    endif()
    target_link_libraries(minicrt PUBLIC xxc)

    foreach(tgt IN LISTS XXC_TARGETS)
        target_include_directories(${tgt} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
        if (WIN32)
            target_compile_definitions(${tgt} PUBLIC WIN32_LEAN_AND_MEAN _WINSOCKAPI_)
        endif()
        xxc_freestanding_compile_options(${tgt} PUBLIC)
    endforeach()
    
    if (TARGET xxc_shared)
        xxc_freestanding_link_options(xxc_shared)
    endif()

    # Installation rules for CPack
    install(TARGETS ${XXC_TARGETS}
            LIBRARY DESTINATION lib COMPONENT libxxc
            ARCHIVE DESTINATION lib COMPONENT libxxc-headers
            RUNTIME DESTINATION bin COMPONENT libxxc)
            
    install(DIRECTORY include/ DESTINATION include/xxc COMPONENT libxxc-headers)
endif()