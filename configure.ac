AC_PREREQ([2.68])
AC_INIT([buxton],[1],[william.douglas@intel.com])
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
AC_CHECK_HEADERS([attr/xattr.h])
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
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
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
	[dbpath="${localstatedir}/buxton"])
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

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debug mode]))
AS_IF([test "x$enable_debug" = "xyes"],
	[AC_DEFINE([DEBUG], [1], [Debugging mode enabled])],
	[])
AM_CONDITIONAL([DEBUG], [test x$enable_debug = x"yes"])

AC_ARG_ENABLE(demos, AS_HELP_STRING([--enable-demos], [enable demos]))
AS_IF([test "x$enable_demos" = "xyes"],
	[AC_DEFINE([DEMOS], [1], [Building demos])],
	[])
AM_CONDITIONAL([BUILD_DEMOS], [test x$enable_demos = x"yes"])

AC_CONFIG_FILES([
data/buxton.service
data/buxton.socket
test/test-pass.ini
test/test-fail.ini
src/shared/constants.c
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
])
