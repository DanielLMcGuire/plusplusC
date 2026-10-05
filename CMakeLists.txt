cmake_minimum_required(VERSION 3.25)

project(plusplusc
    VERSION 0.1.0
    DESCRIPTION "++C"
    LANGUAGES C ASM
)

if (NOT (WIN32 OR LINUX))
    message(FATAL_ERROR "++C only supports Linux and Windows targets (got: ${CMAKE_SYSTEM_NAME})")
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(XXC_BUILD_TEST "Build the ++C test executable" ${PROJECT_IS_TOP_LEVEL})

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompileOptions.cmake")

add_subdirectory(platform)
add_subdirectory(libminicrt)
add_subdirectory(libxxc)

if (XXC_BUILD_TEST)
    add_custom_target(check)

    function(xxc_add_test name source)
        set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${source}" PROPERTIES LANGUAGE C)
        add_executable(xxc_${name} "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        set_target_properties(xxc_${name} PROPERTIES OUTPUT_NAME ${name})
        target_link_libraries(xxc_${name} PRIVATE xxc::xxc)
        xxc_freestanding_compile_options(xxc_${name} PRIVATE)
        xxc_freestanding_link_options(xxc_${name})
        add_custom_target(run_${name} COMMAND xxc_${name} DEPENDS xxc_${name} USES_TERMINAL)
        add_dependencies(check run_${name})
    endfunction()

    xxc_add_test(test         test/test.ppc)
    xxc_add_test(leak_test    test/leak.ppc)
endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "plusplusc")
set(CPACK_PACKAGE_NAME "plusplusc")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_LIBXXC_PACKAGE_NAME "libxxc")
set(CPACK_DEBIAN_LIBXXC-HEADERS_PACKAGE_NAME "libxxc-headers")
set(CPACK_DEBIAN_LIBXXC-HEADERS_PACKAGE_DEPENDS "libxxc (= ${PROJECT_VERSION})")
include(CPack)