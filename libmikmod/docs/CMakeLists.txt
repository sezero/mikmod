# Excerpt taken from http://cmake.3232098.n2.nabble.com/Building-Texinfo-with-CMake-td5609561.html
find_program(MAKEINFO_EXECUTABLE makeinfo)

set(info_in ${CMAKE_CURRENT_SOURCE_DIR}/mikmod.texi)
set(info_out ${CMAKE_CURRENT_BINARY_DIR}/mikmod.info)
SET(info_out_html ${CMAKE_CURRENT_BINARY_DIR}/mikmod.html)
add_custom_command(OUTPUT ${info_out}
  COMMAND ${MAKEINFO_EXECUTABLE} -o ${info_out} ${info_in}
  DEPENDS ${info_in}
  COMMENT "Creating Info file ${info_out}"
  VERBATIM)

add_custom_command(OUTPUT ${info_out_html}
  COMMAND ${MAKEINFO_EXECUTABLE} --no-split --html -o ${info_out_html} ${info_in}
  DEPENDS ${info_in}
  COMMENT "Creating HTML file ${info_out_html}"
  VERBATIM)

add_custom_target(info ALL DEPENDS ${info_out} ${info_out_html})
