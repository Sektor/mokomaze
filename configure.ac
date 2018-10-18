AC_PREREQ(2.53)
AC_INIT(mokomaze, 0.7.0+, https://mokomaze.sourceforge.io/)
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

PKG_CHECK_MODULES([PNG], [libpng])
AC_SUBST([PNG])

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

font_name='Liberation Mono'
font_file='LiberationMono-Regular.ttf'
font_dirs='/usr/share/fonts/truetype/liberation /usr/share/fonts/truetype/ttf-liberation'

AC_CHECK_PROGS([FCMATCH],[fc-match])

AC_ARG_WITH(font-name,
        AS_HELP_STRING([--with-font-name="$font_name"],
                [Name of the font to lookup font file paths using fc-match ($font_name is default)]),
        [FONT_NAME="$withval"],
        [FONT_NAME="$font_name"])

AC_ARG_WITH(font-path,
        AS_HELP_STRING([--with-font-path="/usr/share/fonts/$font_file"],
                [Path to the TTF font to use at runtime ($font_name recommended)]),
        [FONT_PATH="$withval"],
        [FONT_PATH=""])

AC_MSG_CHECKING([fc-match path for $FONT_NAME font])
AS_IF([test "x$FONT_PATH" = x -a "x$FCMATCH" != x], [FONT_PATH="$("$FCMATCH" --format %{file} "$FONT_NAME")" ; ])
AC_MSG_RESULT([$FONT_PATH])

for font_dir in $font_dirs ; do
AS_IF([test "x$FONT_PATH" = x -a -f "$font_dir/$font_file"], [FONT_PATH="$font_dir/$font_file"])
done
AS_IF([test "x$FONT_PATH" = x], [AC_MSG_ERROR([No font file found])])

AC_SUBST(FONT_PATH, ${FONT_PATH})

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

