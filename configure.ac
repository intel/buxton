
AC_PREREQ([2.68])
AC_INIT([buxton],[2],[william.douglas@intel.com],[buxton],[https://github.com/sofar/buxton])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability silent-rules subdir-objects color-tests no-dist-gzip dist-xz])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SRCDIR([src/core/main.c])
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)

LT_PREREQ(2.2)
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([gdbm], [gdbm_open])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([m], [sqrt])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strndup])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([__secure_getenv secure_getenv])

AC_MSG_CHECKING([[whether preprocessor supports #pragma once]])
AC_PREPROC_IFELSE(
        [AC_LANG_PROGRAM([[#pragma once]])],
        [
                AC_MSG_RESULT([yes])
                AC_DEFINE([HAVE_PRAGMA_ONCE], [1], [Preprocessor support for #pragma once])
        ],
        [AC_MSG_RESULT([no])])

AH_VERBATIM([_GNU_SOURCE],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif])

AH_TOP([#ifndef CONFIG_H_INCLUDED
	#define CONFIG_H_INCLUDED 1])
AH_BOTTOM([#endif])

# PkgConfig tests
PKG_CHECK_MODULES([CHECK], [check])
PKG_CHECK_MODULES([SYSTEMD], [libsystemd-daemon])

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([attr/xattr.h], [], [AC_MSG_ERROR([Unable to fin xattr headers])])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([gdbm.h], [], [AC_MSG_ERROR([Unable to find gdbm headers])])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([math.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/signalfd.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([linux/inotify.h])
AC_CHECK_FUNC(inotify_init)

# Options
AC_ARG_WITH([systemdsystemunitdir], AS_HELP_STRING([--with-systemdsystemunitdir=DIR],
	[path to systemd system service directory]), [path_systemdsystemunit=${withval}],
	[path_systemdsystemunit="`$PKG_CONFIG --variable=systemdsystemunitdir systemd`"])
SYSTEMD_SYSTEMUNITDIR="${path_systemdsystemunit}"
AC_SUBST(SYSTEMD_SYSTEMUNITDIR)
AM_CONDITIONAL(SYSTEMD, test -n "${path_systemdsystemunit}")

AC_ARG_WITH([user], AS_HELP_STRING([--with-user=USER],
	[user to run buxton as]), [username=${withval}],
	[username="buxton"])
BUXTON_USERNAME="${username}"
AC_SUBST(BUXTON_USERNAME)

INIPARSER_CFLAGS="-I${srcdir}/src/shared/"
AC_ARG_WITH([local-iniparser],
	AS_HELP_STRING([--with-local-iniparser[default=yes]],
		[Use built-in iniparser for config parsing]),
	[use_local_iniparser="true"],
	[PKG_CHECK_MODULES([INIPARSER], [iniparser >= 3.1],
		[use_local_iniparser="false"],
		[use_local_iniparser="true"; AC_SUBST(INIPARSER_CFLAGS)])])
AM_CONDITIONAL([USE_LOCAL_INIPARSER], [test x$use_local_iniparser = x"true"])

AC_ARG_WITH([module-dir], AS_HELP_STRING([--with-module-dir=MODULEDIR],
	[path to buxton modules]), [moduledir=${withval}],
	[moduledir="${libdir}/buxton"])
MODULEDIR="${moduledir}"
AC_SUBST(MODULEDIR)

AC_ARG_WITH([config-path], AS_HELP_STRING([--with-config-path=CONFPATH],
	[path to buxton configuration file]), [confpath=${withval}],
	[confpath="${sysconfdir}/buxton.conf"])
CONFPATH="${confpath}"
AC_SUBST(CONFPATH)

AC_ARG_WITH([db-path], AS_HELP_STRING([--with-db-path=DBPATH],
	[path to buxton db files]), [dbpath=${withval}],
	[dbpath="${localstatedir}/lib/buxton"])
DB_PATH="${dbpath}"
AC_SUBST(DB_PATH)

AC_ARG_WITH([socket-path], AS_HELP_STRING([--with-socket-path=SOCKETPATH],
	[path to buxton socket file]), [socketpath=${withval}],
	[socketpath="/run/buxton-0"])
BUXTON_SOCKET="${socketpath}"
AC_SUBST(BUXTON_SOCKET)

AC_ARG_WITH([smack-load-file], AS_HELP_STRING([--with-smack-load-file=SMACKLOADFILE],
	[path to smack load2 file]), [smack_load_file=${withval}],
	[smack_load_file="/sys/fs/smackfs/load2"])
SMACK_LOAD_FILE="${smack_load_file}"
AC_SUBST(SMACK_LOAD_FILE)

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debug mode @<:@default=no@:>@]),
	      [], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"],
	[AC_DEFINE([DEBUG], [1], [Debugging mode enabled])],
	[AC_DEFINE([NDEBUG], [1], [Debugging and assertions disabled])])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = x"yes"])

AC_ARG_ENABLE(manpages, AS_HELP_STRING([--enable-manpages], [enable man pages @<:@default=yes@:>@]),
	      [], [enable_manpages=yes])
AS_IF([test "x$enable_manpages" = "xyes"],
	[AC_DEFINE([MANPAGE], [1], [Man pages will be included])],
	[AC_DEFINE([NMANPAGE], [1], [Man pages will not be included])])
AM_CONDITIONAL([MANPAGE], [test x$enable_manpages = x"yes"])

have_coverage=no
AC_ARG_ENABLE(coverage, AS_HELP_STRING([--enable-coverage], [enable test coverage]))
if test "x$enable_coverage" = "xyes" ; then
        AC_CHECK_PROG(lcov_found, [lcov], [yes], [no])
        if test "x$lcov_found" = xno ; then
                AC_MSG_ERROR([*** lcov support requested but the program was not found])
        else
                lcov_version_major="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 1`"
                lcov_version_minor="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 2`"
                if test "$lcov_version_major" -eq 1 -a "$lcov_version_minor" -lt 10; then
                        AC_MSG_ERROR([*** lcov version is too old. 1.10 required])
                else
			have_coverage=yes
			AC_DEFINE([COVERAGE], [1], [Coverage enabled])
                fi
        fi
fi
AM_CONDITIONAL([COVERAGE], [test "$have_coverage" = "yes"])

AC_ARG_ENABLE(demos, AS_HELP_STRING([--enable-demos], [enable demos @<:@default=no@:>@]),
	      [], [enable_demos=no])
AS_IF([test "x$enable_demos" = "xyes"],
	[AC_DEFINE([DEMOS], [1], [Building demos])],
	[])
AM_CONDITIONAL([BUILD_DEMOS], [test x$enable_demos = x"yes"])

# GTK3 demo
AC_ARG_ENABLE(gtk-demo, AS_HELP_STRING([--enable-gtk-demo], [enable demos @<:@default=no@:>@]),
	      [], [enable_gtk_demo=no])
AS_IF([test "x$enable_gtk_demo" = "xyes"],
	[AC_DEFINE([DEMOS], [1], [Building GTK demo])],
	[])
AM_CONDITIONAL([BUILD_GTK_DEMO], [test x$enable_gtk_demo = x"yes"])

if test "x$enable_gtk_demo" = "xyes"; then
	# Require GTK3 for client demonstration
	PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.10])
fi

AC_CONFIG_COMMANDS([mkdir], [$MKDIR_P test/databases])
AC_CONFIG_FILES([
data/buxton.service
data/buxton.socket
data/libbuxton.pc
test/test-pass.ini
test/test-fail.ini
test/test.conf
])
AC_OUTPUT

AC_MSG_RESULT([
        buxton $VERSION
        ========

        prefix:                 ${prefix}
        libdir:                 ${libdir}
        sysconfdir:             ${sysconfdir}
        exec_prefix:            ${exec_prefix}
        bindir:                 ${bindir}
        sbindir:                ${sbindir}
        datarootdir:            ${datarootdir}
        mandir:                 ${mandir}
        modules:                ${MODULEDIR}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}

        debug:                  ${enable_debug}
        demos:                  ${enable_demos}
        coverage:               ${have_coverage}
        manpages:               ${enable_manpages}
])
