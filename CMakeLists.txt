cmake_minimum_required(VERSION 3.23)

project(mikmod C)

include(CTest)

option(BUILD_LIBMIKMOD "Build libmikmod" ON)
option(BUILD_MIKMOD_APP "Build the mikmod player application" ON)
option(BUILD_MIKTEST "Build the miktest DOS playback test" ON)

# Optional knob for DOS-oriented trees.
option(MIKMOD_BUILD_DOS "Enable DOS-oriented build paths" OFF)

# Keep outputs grouped.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")  

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(BUILD_LIBMIKMOD)
    add_subdirectory(libmikmod)
endif()

if(BUILD_MIKMOD_APP)
    add_subdirectory(mikmod)
endif()

if(BUILD_MIKTEST)
    add_subdirectory(miktest)
endif()

message(STATUS "----------------------------------------")
message(STATUS "mikmod root configuration")
message(STATUS "  BUILD_LIBMIKMOD : ${BUILD_LIBMIKMOD}")
message(STATUS "  BUILD_MIKMOD_APP: ${BUILD_MIKMOD_APP}")
message(STATUS "  BUILD_MIKTEST   : ${BUILD_MIKTEST}")
message(STATUS "  MIKMOD_BUILD_DOS: ${MIKMOD_BUILD_DOS}")
message(STATUS "  BUILD_TESTING   : ${BUILD_TESTING}")
message(STATUS "----------------------------------------")