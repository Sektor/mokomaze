INCLUDES = -I$(top_srcdir)

AM_CPPFLAGS = \
  -DDATADIR=\""$(datadir)"\" -D_GNU_SOURCE \
  -DPIXMAPDIR=\""$(datadir)/pixmaps"\" \
  -DFONTDIR=\""@MAZEFONTDIR@"\"

if RGBSWAP
AM_CPPFLAGS += -DRGBSWAP
endif

AM_CFLAGS = -Wall -pedantic -std=c99 @SDL_CFLAGS@

# add the name of the application
bin_PROGRAMS = mokomaze

# add the sources to compile for the application
mokomaze_SOURCES = \
  main.c mainwindow.c paramsloader.c accelerometers.c vibro.c json.c

mokomaze_LDADD  = @SDL_LIBS@

MAINTAINERCLEANFILES  = config.h.in Makefile.in

