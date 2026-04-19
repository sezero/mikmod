cmake_minimum_required(VERSION 3.23)

project(miktest C)

add_executable(miktest
    MIKTEST.c
)

target_include_directories(miktest PRIVATE
    "${CMAKE_SOURCE_DIR}/libmikmod/include"
)

if(TARGET mikmod-static)
    target_link_libraries(miktest PRIVATE mikmod-static)
    target_compile_definitions(miktest PRIVATE MIKMOD_STATIC)
elseif(TARGET mikmod)
    target_link_libraries(miktest PRIVATE mikmod)
else()
    message(FATAL_ERROR "miktest requires libmikmod to be built in-tree.")
endif()

set_target_properties(miktest PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

set_target_properties(miktest PROPERTIES
    OUTPUT_NAME "miktest"
)

if(CMAKE_C_COMPILER_ID STREQUAL "OpenWatcom")
    set_target_properties(miktest PROPERTIES
        SUFFIX ".exe"
    )
endif()

if(BUILD_TESTING)
    add_test(NAME miktest-runs COMMAND miktest)
endif()

# ------------------------------------------------------------
# DOS / Watcom deployment
# Mirrors the pattern used by the main RMP build
# ------------------------------------------------------------
set(MIKTEST_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/BIN" CACHE PATH
    "Output directory for deployed miktest executable")

set(DOS32A_EXE "C:/WATCOM/binw/dos32a.exe" CACHE FILEPATH
    "Path to dos32a.exe from Open Watcom")

function(miktest_deploy_dos_exe tgt)
    if(NOT TARGET ${tgt})
        message(FATAL_ERROR "miktest_deploy_dos_exe: target '${tgt}' does not exist")
    endif()

    add_custom_command(TARGET ${tgt} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MIKTEST_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${tgt}>"
            "${MIKTEST_BIN_DIR}/$<TARGET_FILE_NAME:${tgt}>"
        COMMENT "Deploying $<TARGET_FILE_NAME:${tgt}> to ${MIKTEST_BIN_DIR}"
        VERBATIM
    )

    if(EXISTS "${DOS32A_EXE}")
        add_custom_command(TARGET ${tgt} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DOS32A_EXE}"
                "${MIKTEST_BIN_DIR}/dos32a.exe"
            COMMENT "Deploying DOS32A as dos32a.exe to ${MIKTEST_BIN_DIR}"
            VERBATIM
        )
    else()
        message(WARNING "DOS32A_EXE not found: ${DOS32A_EXE}")
    endif()
endfunction()

if(CMAKE_SYSTEM_NAME STREQUAL "DOS" OR CMAKE_C_COMPILER_ID STREQUAL "OpenWatcom")
    miktest_deploy_dos_exe(miktest)
endif()