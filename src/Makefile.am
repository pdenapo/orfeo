INCLUDES = -I$(top_srcdir)/kkconsui-0.1/include -I$(top_srcdir)/kkstrtext-0.1 -I$(top_srcdir)/intl
bin_PROGRAMS = orpheus
orpheus_SOURCES = mp3track.cc main.cc cdtrack.cc orpheusconf.cc uitext.cc playlist.cc oggtrack.cc vcedit.c mixerctl.cc streamtrack.cc
orpheus_LDFLAGS = -L$(top_srcdir)/kkstrtext-0.1 -L$(top_srcdir)/kkconsui-0.1
orpheus_LDADD =  -lkkstrtext -lkkconsui @INTLLIBS@
SUBDIRS = abstract
EXTRA_DIST = mp3track.cc main.cc cdtrack.cc orpheusconf.cc uitext.cc playlist.cc oggtrack.cc vcedit.c mixerctl.cc streamtrack.cc playlist.h mp3track.h uitext.h cdtrack.h orpheusconf.h oggtrack.h vcedit.h mixerctl.h streamtrack.h
AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -DSHARE_DIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\"
