SUBDIRS = kkstrtext-0.1 kkconsui-0.1 intl po src
dist-hook:
	cd $(top_distdir); subs=`egrep "^[:space:]*SUBDIRS" Makefile.am | sed 's/SUBDIRS[ =]\+//g'`; \
	echo "SUBDIRS = kkstrtext-0.1 kkconsui-0.1 $$subs" >>.makefile.am; \
	egrep -v "^[:space:]*SUBDIRS" Makefile.am >>.makefile.am; \
	mv .makefile.am Makefile.am;  autoconf && automake

# END OF MOTOR DIST TARGETS #
EXTRA_DIST = orpheus.motor config.rpath orpheus.1 NEWS TODO README AUTHORS INSTALL ABOUT-NLS ChangeLog COPYING FAQ orpheus.spec kkstrtext-0.1/* kkconsui-0.1/*
AUTOMAKE_OPTIONS = foreign
