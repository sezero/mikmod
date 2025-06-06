# Excerpt taken from https://cmake.org/pipermail/cmake/2010-October/040088.html
option(ENABLE_DOC "Build the library documentation" ON)

find_program(MAKEINFO_EXECUTABLE makeinfo)

set(info_in ${CMAKE_CURRENT_SOURCE_DIR}/mikmod.texi)
set(info_out ${CMAKE_CURRENT_BINARY_DIR}/mikmod.info)
set(html_out ${CMAKE_CURRENT_BINARY_DIR}/mikmod.html)
set(man_out ${CMAKE_CURRENT_BINARY_DIR}/libmikmod-config.1)

IF (ENABLE_DOC)
add_custom_command(OUTPUT ${info_out}
  COMMAND ${MAKEINFO_EXECUTABLE} --no-split -o ${info_out} ${info_in}
  DEPENDS ${info_in}
  COMMENT "Creating Info file ${info_out}"
  VERBATIM)

add_custom_command(OUTPUT ${html_out}
  COMMAND ${MAKEINFO_EXECUTABLE} --no-split --html -o ${html_out} ${info_in}
  DEPENDS ${info_in}
  COMMENT "Creating HTML file ${html_out}"
  VERBATIM)

configure_file(libmikmod-config.1.in ${man_out} @ONLY)
install(FILES ${man_out} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

add_custom_target(info ALL DEPENDS ${info_out} ${html_out})
install(FILES ${info_out} DESTINATION ${CMAKE_INSTALL_INFODIR})
install(FILES ${html_out}
    ${PROJECT_SOURCE_DIR}/NEWS
    ${PROJECT_SOURCE_DIR}/README
  DESTINATION ${CMAKE_INSTALL_DOCDIR})
ENDIF()
