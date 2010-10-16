AC_PREREQ(2.53)
AC_INIT(mokomaze, 0.5.5+, http://mokomaze.projects.openmoko.org/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_PROG_CC_STDC
AC_HEADER_STDC

PKG_CHECK_MODULES(SDL, sdl >= 1.0.0)
AC_CHECK_LIB(SDL_image, IMG_Load, , [AC_MSG_ERROR([*** SDL_image not found!])])
AC_CHECK_LIB(SDL_ttf, TTF_OpenFont, , [AC_MSG_ERROR([*** SDL_ttf not found!])])
AC_CHECK_LIB(ode, dInitODE, , [AC_MSG_ERROR([*** ODE not found!])])

AC_SUBST(SDL)

#--------------------------------------------------------------
if test "x"${FONTDIR} == "x"; then
AC_SUBST(MAZEFONTDIR, /usr/share/fonts/truetype/ttf-liberation)
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

