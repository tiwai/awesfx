SUBDIRS = awelib include samples etc

bin_PROGRAMS = sfxload asfxload aweset gusload agusload setfx sf2text text2sf sfxtest
LDADD = awelib/libawe.a

AUTOMAKE_OPTIONS = foreign

INCLUDES = -Iinclude

asfxload_SOURCES = asfxload.c alsa.c
asfxload_LDADD = awelib/libawe.a @ALSA_LIBS@

sfxload_SOURCES = sfxload.c seq.c
aweset_SOURCES = aweset.c seq.c
gusload_SOURCES = gusload.c seq.c
agusload_SOURCES = agusload.c alsa.c
agusload_LDADD = awelib/libawe.a @ALSA_LIBS@
sfxtest_SOURCES = sfxtest.c seq.c
setfx_SOURCES = setfx.c seq.c
sf2text_SOURCES = sf2text.c
text2sf_SOURCES = text2sf.c

noinst_HEADERS = seq.h guspatch.h

man_MANS = sfxload.1

EXTRA_DIST = sfxload.1 SBKtoSF2.txt

install-data-hook:
	rm -f $(DESTDIR)$(mandir)/man1/asfxload.1
	(cd $(DESTDIR)$(mandir)/man1 && $(LN_S) sfxload.1 asfxload.1)
