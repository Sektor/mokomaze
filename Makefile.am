SUBDIRS = \
  src \
  data \
  doc

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = \
  aclocal.m4 \
  compile \
  config.guess \
  config.sub \
  configure \
  depcomp \
  install-sh \
  ltmain.sh \
  Makefile.in \
  missing \
  config.h.in \
  config.h.in~ \
  mkinstalldirs \
  INSTALL

dist-hook:
	if test -d .git -a \( ! -e $(distdir)/ChangeLog -o -w $(distdir)/ChangeLog \) ; then \
		LC_ALL=C git log --pretty --numstat --summary $(VERSION) | git2cl > $(distdir)/ChangeLog ; \
	fi
