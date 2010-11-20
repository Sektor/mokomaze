#
# misc. data files
#
mokomazedir = $(datadir)/mokomaze
dist_mokomaze_DATA = desk.png wall.png \
                     main.levelpack.json config.json

nodist_mokomaze_DATA = fin.png \
                     prev.png next.png reset.png close.png \
                     prev-i.png next-i.png reset-i.png \
                     prev-p.png next-p.png reset-p.png

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

EXTRA_DIST = filters.svg \
             openmoko.svg \
             background.sed kde.svg \
             mokomaze-main.svg mokomaze.sed neverlogos.svg \
             close.svg close-main.svg \
             reset.svg reset-main.svg reset-grey.svg reset-light.svg \
             prev.svg prev-main.svg prev-grey.svg prev-light.svg \
             next.svg next-main.svg next-grey.svg next-light.svg

MAINTAINERCLEANFILES = Makefile.in

CLEANFILES = fin.png mokomaze.png \
             background.svg mokomaze.svg \
             close.png prev.png next.png reset.png \
             prev-i.png next-i.png reset-i.png \
             prev-p.png next-p.png reset-p.png

background.svg: kde.svg background.sed
	$(SED) -n -f $(srcdir)/background.sed < $< > $@

mokomaze.svg: neverlogos.svg mokomaze.sed
	$(SED) -f $(srcdir)/mokomaze.sed < $< > $@

%.png: %-main.svg %.svg background.svg
	$(RSVGCONVERT) --width 96 --height 96 --output $@ $<

%-i.png: %-grey.svg %.svg filters.svg background.svg
	$(RSVGCONVERT) --width 96 --height 96 --output $@ $<

%-p.png: %-light.svg %.svg filters.svg background.svg
	$(RSVGCONVERT) --width 96 --height 96 --output $@ $<

fin.png: openmoko.svg
	$(RSVGCONVERT) --width 48 --height 48 --output $@ $<

mokomaze.png: mokomaze-main.svg mokomaze.svg background.svg filters.svg
	$(RSVGCONVERT) --output $@ $<

