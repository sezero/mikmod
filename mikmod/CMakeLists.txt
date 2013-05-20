configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(src)
add_subdirectory(dos)
add_subdirectory(os2)
add_subdirectory(win32)


########### install files ###############

install(FILES  mikmodrc DESTINATION )



#original Makefile.am contents follow:

#AUTOMAKE_OPTIONS = foreign
#SUBDIRS = src dos os2 win32
#
#pkgdata_DATA = mikmodrc
#
#EXTRA_DIST = mikmod-3.2.2.lsm \
#             convert_playlist mikmod.cfg $(pkgdata_DATA)
