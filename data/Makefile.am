#
# misc. data files
#
mokomazedir = $(datadir)/mokomaze
dist_mokomaze_DATA = \
  main.levelpack.json \
  config.json \
  desk.svg \
  wall.svg \
  openmoko.svg \
  prev-main.svg \
  next-main.svg \
  settings-main.svg \
  close-main.svg \
  prev-light.svg \
  next-light.svg \
  prev-grey.svg \
  next-grey.svg

#
# desktop integration: .desktop file
#
desktopdir = $(datadir)/applications
dist_desktop_DATA = mokomaze.desktop

#
# desktop integration: application icon
#
appicondir = $(datadir)/pixmaps
dist_appicon_DATA = mokomaze.svg

EXTRA_DIST = \
  logo.xsl \
  buttons.xsl \
  clean.xsl \
  background.sed \
  ball.sed \
  kde.svg \
  neverlogos.svg \
  filters.svg \
  prev.svg \
  next.svg \
  settings.svg \
  close.svg

MAINTAINERCLEANFILES = Makefile.in

CLEANFILES = \
  mokomaze.svg \
  background.svg \
  ball.svg \
  *-main.svg \
  *-light.svg \
  *-grey.svg

background.svg: kde.svg background.sed
	$(SED) -n -f $(srcdir)/background.sed < $< | $(XSLTPROC) -o $@ $(srcdir)/clean.xsl -

ball.svg: neverlogos.svg ball.sed
	$(SED) -f $(srcdir)/ball.sed < $< | $(XSLTPROC) -o $@ $(srcdir)/clean.xsl -

%-main.svg : %.svg background.svg filters.svg buttons.xsl
	$(XSLTPROC) -o $@ --stringparam filter_name "" $(srcdir)/buttons.xsl $<

%-light.svg : %.svg background.svg filters.svg buttons.xsl
	$(XSLTPROC) -o $@ --stringparam filter_name "light-filter" $(srcdir)/buttons.xsl $<

%-grey.svg : %.svg background.svg filters.svg buttons.xsl
	$(XSLTPROC) -o $@ --stringparam filter_name "grey-filter" $(srcdir)/buttons.xsl $<

mokomaze.svg : ball.svg background.svg filters.svg logo.xsl
	$(XSLTPROC) -o $@ --stringparam filter_name "green-filter" $(srcdir)/logo.xsl ball.svg

