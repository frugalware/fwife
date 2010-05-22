SUBDIRS = plugins
INCLUDES=@GTK_CFLAGS@
DEFS =	@DEFS@ \
	-DPLUGINDIR="\"$(pkglibdir)/plugins\"" \
	-DIMAGEDIR="\"$(pkgdatadir)/images\""


bin_PROGRAMS=fwife

fwife_SOURCES=				\
			fwife.c 	\
			util.c		\
			fwife-message.c


fwife_LDADD= @GTK_LIBS@
fwife_LDFLAGS = -export-dynamic

