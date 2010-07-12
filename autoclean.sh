#!/bin/sh -xu

if [ -f Makefile ]; then
    make distclean
fi

rm -rf autom4te.cache
rm -rf Makefile
rm -rf Makefile.in
rm -rf configure
rm -rf config.*
rm -rf stamp*
rm -rf depcomp
rm -rf install-sh
rm -rf missing
rm -rf src/Makefile
rm -rf src/Makefile.in
rm -rf src/plugins/Makefile
rm -rf src/plugins/Makefile.in
rm -rf aclocal.m4
rm -rf ltmain.sh
rm -rf compile
rm -rf libtool
rm -rf mkinstalldirs
rm -rf config.rpath
rm -rf data/Makefile.in
rm -rf data/Makefile
rm -rf data/{images,kmconf}/Makefile.in
rm -rf data/{images,kmconf}/Makefile
rm -rf data/images/flags/Makefile.in
rm -rf data/images/flags/Makefile
rm -rf po/stamp-it
rm -rf intltool-{extract,merge,update}
rm -rf intltool-{extract,merge,update}.in
rm -rf po/POTFILES
rm -rf po/Makefile{.in,.in.in}
rm -rf po/Makefile
