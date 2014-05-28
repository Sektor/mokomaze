AC_PREREQ(2.53)
AC_INIT(mokomaze, 0.7.0+, http://mokomaze.sourceforge.net/)
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE([enable])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CXX
AC_HEADER_STDC

AC_PROG_SED
AC_PATH_PROG(XSLTPROC,xsltproc)
if test -z "$XSLTPROC" ; then
	AC_MSG_ERROR([You need xsltproc to build the images!])
fi

PKG_CHECK_MODULES(SDL, sdl >= 1.0.0)
AC_SUBST(SDL)

AC_CHECK_LIB(SDL_image, IMG_Load, , [AC_MSG_ERROR([*** SDL_image not found!])])
AC_CHECK_LIB(SDL_ttf, TTF_OpenFont, , [AC_MSG_ERROR([*** SDL_ttf not found!])])
AC_CHECK_LIB(SDL_gfx, zoomSurface, , [AC_MSG_ERROR([*** SDL_gfx not found!])])
AC_CHECK_LIB(ode, dInitODE, , [AC_MSG_ERROR([*** ODE not found!])])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.20.1])
AC_SUBST(GLIB)

PKG_CHECK_MODULES(GLIBJSON, [json-glib-1.0 >= 0.6.2])
AC_SUBST(GLIBJSON)

PKG_CHECK_MODULES(RSVG, [librsvg-2.0 >= 2.26.0])
AC_SUBST(RSVG)

AC_CHECK_LIB(argtable2, arg_parse, , [AC_MSG_ERROR([*** argtable2 not found!])])

AC_CHECK_LIB(guichan, gcnGuichanVersion, , [AC_MSG_ERROR([*** guichan not found!])])
AC_CHECK_LIB(guichan_sdl, gcnSDL, , [AC_MSG_ERROR([*** guichan-sdl not found!])])

#--------------------------------------------------------------
if test "x"${FONTDIR} == "x"; then
        DEFAULT_FONTDIR="/usr/share/fonts/truetype/liberation"
        if test ! -d "$DEFAULT_FONTDIR"; then
                DEFAULT_FONTDIR="/usr/share/fonts/truetype/ttf-liberation"
        fi
        AC_SUBST(MAZEFONTDIR, ${DEFAULT_FONTDIR})
else
        AC_SUBST(MAZEFONTDIR, ${FONTDIR})
fi

AC_ARG_ENABLE(rgb-swap,
        [  --enable-rgb-swap       swap color components (R<->B) ],
        [use_rgb_swap="yes"], [use_rgb_swap="no"])
AM_CONDITIONAL(RGBSWAP, test x$use_rgb_swap != xno)
#--------------------------------------------------------------

AC_CONFIG_FILES([
Makefile
src/Makefile
doc/Makefile
data/Makefile
])
AC_OUTPUT

