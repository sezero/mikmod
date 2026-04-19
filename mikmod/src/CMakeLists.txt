# -----------------------------------------------------------------------------
# mikmod player executable
# -----------------------------------------------------------------------------

set(MIKMOD_PLAYER_TARGET mikmod-player)

set(mikmod_player_sources
    display.c
    marchive.c
    mikmod.c
    mlist.c
    mconfig.c
    mwindow.c
    mmenu.c
    mwidget.c
    mdialog.c
    mconfedit.c
    mutilities.c
    mplayer.c
    mlistedit.c
    rcfile.c
)

if(NOT HAVE_USLEEP AND NOT WIN32 AND NOT OS2)
    list(APPEND mikmod_player_sources musleep.c)
endif()

if(NOT HAVE_FNMATCH)
    list(APPEND mikmod_player_sources mfnmatch.c)
endif()

if(NOT HAVE_GETOPT_LONG_ONLY)
    list(APPEND mikmod_player_sources getopt_long.c)
endif()

add_executable(${MIKMOD_PLAYER_TARGET}
    ${mikmod_player_sources}
)

target_include_directories(${MIKMOD_PLAYER_TARGET}
    PRIVATE
        ${MIKMOD_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
)

target_link_libraries(${MIKMOD_PLAYER_TARGET}
    PRIVATE
        ${MIKMOD_LIBRARIES}
        ${EXTRA_LIBS}
)

set_target_properties(${MIKMOD_PLAYER_TARGET}
    PROPERTIES
        OUTPUT_NAME mikmod
)

install(TARGETS ${MIKMOD_PLAYER_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES mikmod.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)