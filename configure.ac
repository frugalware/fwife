ENV_CFLAGS=$CFLAGS

AC_PREREQ([2.63])
AC_INIT([Fwife], 0.9.5, [elentir@frugalware.org], fwife)
AC_CONFIG_SRCDIR([src/fwife.c])
AC_LANG(C)
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.21])
AC_PROG_LIBTOOL

AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug], [Enable debugging support]), [debug="yes"])

dnl Build with Werror
AC_ARG_ENABLE(werror,
	AC_HELP_STRING([--enable-werror],  [Enable building with Werror]),
	[werror=$enableval], [werror=no])

dnl Build with strict
AC_ARG_ENABLE(strict,
	AC_HELP_STRING([--enable-strict],  [Enable building with 'Strict' flags]),
	[strict=$enableval], [strict=no])

# Check for debug enabled or not
AC_MSG_CHECKING(for debug request)
if test "x$debug" = "xyes" ; then
    AC_DEFINE(FWIFE_DEBUG, 1, [Debug support: enabled])
    CFLAGS="${CFLAGS} -Wall"
    debug=yes
    AC_MSG_RESULT(yes)
else
    AC_DEFINE(FWIFE_DEBUG, 0, [Debug support: disabled])
    CFLAGS="${CFLAGS} -Wall -DNDEBUG"
    debug=no
    AC_MSG_RESULT(no)
fi

dnl Check for Werror request
AC_MSG_CHECKING(for werror usage)
if test "x$werror" = "xyes" ; then
    CFLAGS="${CFLAGS} -Werror"
    werror=yes
    AC_MSG_RESULT(yes)
else
    werror=no
    AC_MSG_RESULT(no)
fi

dnl Enable or disable building with 'strict' flags
AC_MSG_CHECKING(for building with 'strict' flags)
if test "x$strict" = "xyes" ; then
	CFLAGS="$CFLAGS -Wextra -Wall -pedantic -Wshadow \
	    -Wmissing-declarations -Wpointer-arith -Wcast-align \
	    -Wfloat-equal -Wstrict-prototypes -Wmissing-prototypes \
	    -Wredundant-decls -Wunused-variable -Wunused-parameter \
	    -Wparentheses -Wno-long-long -Wwrite-strings -Wimplicit \
	    -Wnested-externs -Wno-unused -Wundef -Wpacked \
	    -Wbad-function-cast -Wunused-function -Wunused-label \
	    -Wunused-value -Wfloat-equal -Wchar-subscripts -Wswitch \
	    -Wmissing-braces -Wsequence-point -Wunknown-pragmas \
	    -Wdeclaration-after-statement -Wsign-compare"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

dnl Check for architecture
case "$host" in
	i686-*) CARCH="i686"
		;;
	x86_64-*) CARCH="x86_64"
		;;
	ppc-* | powerpc-*)
		CARCH="ppc"
		;;
	*)
		AC_MSG_ERROR([Your architecture is not supported])
		;;
esac

AC_DEFINE_UNQUOTED(ARCH, ["${CARCH}"], [Architecture])

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([fwgrubconfig], [fwgrub_install], [PLUGINS_LIBS="$PLUGINS_LIBS -lfwgrubconfig"])
AC_CHECK_LIB([fwmouseconfig], [fwmouse_writeconfig], [PLUGINS_LIBS="$PLUGINS_LIBS -lfwmouseconfig"])
AC_CHECK_LIB([fwnetconfig], [fwnet_iflist], [PLUGINS_LIBS="$PLUGINS_LIBS -lfwnetconfig"])
AC_CHECK_LIB([fwtimeconfig], [fwtime_find], [PLUGINS_LIBS="$PLUGINS_LIBS -lfwtimeconfig"])
AC_CHECK_LIB([fwutil], [fwutil_init], [PLUGINS_LIBS="$PLUGINS_LIBS -lfwutil"])
AC_CHECK_LIB([pacman], [pacman_initialize], [PLUGINS_LIBS="$PLUGINS_LIBS -lpacman"])
AC_CHECK_LIB([parted], [ped_device_read], [PLUGINS_LIBS="$PLUGINS_LIBS -lparted"])
AC_CHECK_LIB([blkid], [blkid_new_probe], [PLUGINS_LIBS="$PLUGINS_LIBS -lblkid"])

AC_SUBST(PLUGINS_LIBS)

ALL_LINGUAS=""
GETTEXT_PACKAGE=fwife
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [needed for bindtextdomain() and textdomain()])
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(LOCALEDIR)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h limits.h locale.h stdlib.h string.h sys/mount.h unistd.h pacman.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([getdelim memset mkdir rmdir setenv setlocale strdup strerror strrchr strstr])

PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.12.0])

CFLAGS="${CFLAGS}"

AC_OUTPUT([
src/Makefile
src/plugins/Makefile
data/Makefile
data/images/Makefile
data/images/flags/Makefile
data/kmconf/Makefile
data/desktop/Makefile
po/Makefile.in
Makefile
])

echo "
Fwife $VERSION configuration:

        prefix                   : ${prefix}
        source code location     : ${srcdir}
        compiler                 : ${CC}
        compiler flags           : ${CFLAGS}

	Architecture compiled    : ${CARCH}

	debug support            : ${debug}
	Werror build		 : ${werror}
	Strict flags             : ${strict}
"
