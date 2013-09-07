AUTOMAKE_OPTIONS = foreign
SUBDIRS = drivers include libmikmod loaders mmio playercode \
          docs dlapi macintosh os2 dos win32

bin_SCRIPTS = libmikmod-config

EXTRA_DIST = libmikmod-config.in libmikmod.m4 libmikmod.pc.in \
             COPYING.LESSER

m4datadir = $(datadir)/aclocal
m4data_DATA = libmikmod.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmikmod.pc

ACLOCAL_AMFLAGS = -I m4

