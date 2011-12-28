#
# misc. data files
#
mokomazedir = $(datadir)/mokomaze
dist_mokomaze_DATA = main.levelpack.json config.json \
                     desk.svg wall.svg openmoko.svg filters.svg \
                     prev.svg next.svg settings.svg close.svg \
                     prev-main.svg next-main.svg settings-main.svg close-main.svg \
                     prev-light.svg next-light.svg \
                     prev-grey.svg next-grey.svg

nodist_mokomaze_DATA = background.svg

#
# desktop integration: .desktop file
#
desktopdir = $(datadir)/applications
dist_desktop_DATA = mokomaze.desktop

#
# desktop integration: application icon
#
appicondir = $(datadir)/pixmaps
dist_appicon_DATA = mokomaze.png

EXTRA_DIST = background.sed kde.svg \
             mokomaze-main.svg mokomaze.sed neverlogos.svg

MAINTAINERCLEANFILES = Makefile.in

CLEANFILES = mokomaze.png \
             background.svg mokomaze.svg

background.svg: kde.svg background.sed
	$(SED) -n -f $(srcdir)/background.sed < $< > $@

mokomaze.svg: neverlogos.svg mokomaze.sed
	$(SED) -f $(srcdir)/mokomaze.sed < $< > $@

mokomaze.png: mokomaze-main.svg mokomaze.svg background.svg filters.svg
	$(RSVGCONVERT) --output $@ $<

