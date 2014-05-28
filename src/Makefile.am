AM_CPPFLAGS = \
  -I$(top_srcdir) \
  -DDATADIR=\""$(datadir)"\" \
  -D_GNU_SOURCE \
  -DPIXMAPDIR=\""$(datadir)/pixmaps"\" \
  -DFONTDIR=\""@MAZEFONTDIR@"\"

if RGBSWAP
AM_CPPFLAGS += -DRGBSWAP
endif

AM_CPPFLAGS += \
  -Wall \
  -pedantic \
  @SDL_CFLAGS@
AM_CFLAGS = \
  -Wall \
  -pedantic \
  -std=c99 \
  @PNG_CFLAGS@ \
  @SDL_CFLAGS@ \
  @GLIB_CFLAGS@ \
  @GLIBJSON_CFLAGS@ \
  @RSVG_CFLAGS@

# add the name of the application
bin_PROGRAMS = mokomaze

# add the sources to compile for the application
mokomaze_SOURCES = \
  main.c \
  logging.c \
  paramsloader.c \
  mainwindow.c \
  render.c \
  matrix.c \
  mazecore/mazecore.c \
  mazecore/mazehelpers.c \
  vibro/vibro_freerunner.c \
  vibro/vibro_dummy.c \
  input/input_calibration.c \
  input/input_dummy.c \
  input/input_keyboard.c \
  input/input_joystick.c \
  input/input_joystick_sdl.c \
  input/input_accel.c \
  misc/IMG_SavePNG.c \
  gui/gui_settings.cpp \
  gui/gui_font.cpp \
  gui/gui_msgbox.cpp \
  types.h \
  logging.h \
  paramsloader.h \
  mainwindow.h \
  render.h \
  matrix.h \
  mazecore/mazecore.h \
  mazecore/mazetypes.h \
  mazecore/mazehelpers.h \
  input/input.h \
  input/inputtypes.h \
  input/input_calibration.h \
  input/input_dummy.h \
  input/input_keyboard.h \
  input/input_joystick.h \
  input/input_joystick_sdl.h \
  input/input_accel.h \
  misc/IMG_SavePNG.h \
  vibro/vibro.h \
  vibro/vibrotypes.h \
  vibro/vibro_freerunner.h \
  vibro/vibro_dummy.h \
  gui/gui_settings.h \
  gui/gui_font.h \
  gui/gui_msgbox.h \
  fonts.h \
  dirs.h

mokomaze_LDADD  = \
  @PNG_LIBS@ \
  @SDL_LIBS@ \
  @GLIB_LIBS@ \
  @GLIBJSON_LIBS@ \
  @RSVG_LIBS@ \
  -lm

MAINTAINERCLEANFILES  = \
  config.h.in \
  Makefile.in

