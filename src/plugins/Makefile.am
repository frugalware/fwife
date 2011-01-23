INCLUDES = @GTK_CFLAGS@
DEFS = @DEFS@ \
	-DPLUGINDIR="\"$(pkglibdir)/plugins\"" \
	-DIMAGEDIR="\"$(pkgdatadir)/images\"" \
	-DKEYDIR="\"$(pkgdatadir)/kmconf\""

pluginsdir=$(pkglibdir)/plugins/

plugins_LTLIBRARIES = \
		asklang.la 	\
		configsource.la	\
		finish.la	\
		greet.la	\
		grubconf.la	\
		install.la	\
		layout.la	\
		netconf.la	\
		partdisk.la	\
		postconf.la	\
		rootconf.la	\
		select.la	\
		timeconf.la	\
		usersconf.la

asklang_la_SOURCES = asklang.c
asklang_la_LDFLAGS = -module -avoid-version -shared
asklang_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

configsource_la_SOURCES = configsource.c configsource-network.c configsource-discs.c
configsource_la_LDFLAGS = -module -avoid-version -shared
configsource_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

finish_la_SOURCES = finish.c
finish_la_LDFLAGS = -module -avoid-version -shared
finish_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

greet_la_SOURCES = greet.c
greet_la_LDFLAGS = -module -avoid-version -shared
greet_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

grubconf_la_SOURCES = grubconf.c
grubconf_la_LDFLAGS = -module -avoid-version -shared
grubconf_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

install_la_SOURCES = install.c
install_la_LDFLAGS = -module -avoid-version -shared
install_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

layout_la_SOURCES = layout.c
layout_la_LDFLAGS = -module -avoid-version -shared
layout_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

netconf_la_SOURCES = netconf.c
netconf_la_LDFLAGS = -module -avoid-version -shared
netconf_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

partdisk_la_SOURCES = partdisk.c
partdisk_la_LDFLAGS = -module -avoid-version -shared
partdisk_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

postconf_la_SOURCES = postconf.c
postconf_la_LDFLAGS = -module -avoid-version -shared
postconf_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

rootconf_la_SOURCES = rootconf.c
rootconf_la_LDFLAGS = -module -avoid-version -shared
rootconf_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

select_la_SOURCES = select.c select-intermediate.c select-expert.c select-basic.c
select_la_LDFLAGS = -module -avoid-version -shared
select_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

timeconf_la_SOURCES = timeconf.c
timeconf_la_LDFLAGS = -module -avoid-version -shared
timeconf_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@

usersconf_la_SOURCES = usersconf.c
usersconf_la_LDFLAGS = -module -avoid-version -shared
usersconf_la_LIBADD = @LIBS@ @GTK_LIBS@ @PLUGINS_LIBS@


