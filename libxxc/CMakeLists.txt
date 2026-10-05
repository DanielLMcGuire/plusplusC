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
    )
    set_source_files_properties(${XXC_SOURCES} PROPERTIES LANGUAGE C)

    add_library(xxc STATIC
        ${XXC_SOURCES}
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
    )
    
    add_library(xxc_shared SHARED ${XXC_SOURCES})
    add_library(xxc::xxc ALIAS xxc)
    set_target_properties(xxc_shared PROPERTIES OUTPUT_NAME xxc)

    target_link_libraries(xxc PUBLIC xxc::minicrt)
    target_link_libraries(xxc_shared PUBLIC minicrt_shared)
    target_link_libraries(minicrt PUBLIC xxc)

    foreach(tgt xxc xxc_shared)
        target_include_directories(${tgt} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
        xxc_freestanding_compile_options(${tgt} PUBLIC)
    endforeach()
    
    xxc_freestanding_link_options(xxc_shared)

    # Installation rules for CPack
    install(TARGETS xxc xxc_shared
            LIBRARY DESTINATION lib COMPONENT libxxc
            ARCHIVE DESTINATION lib COMPONENT libxxc-headers
            RUNTIME DESTINATION bin COMPONENT libxxc)
            
    install(DIRECTORY include/ DESTINATION include/xxc COMPONENT libxxc-headers)
endif()