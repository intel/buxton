AC_PREREQ([2.68])
AC_INIT([buxton],[1],[william.douglas@intel.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability subdir-objects color-tests no-dist-gzip dist-xz])
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

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

# PkgConfig tests
PKG_CHECK_MODULES([CHECK], [check])
PKG_CHECK_MODULES([SYSTEMD], [libsystemd-daemon])

# Checks for header files.
AC_CHECK_HEADERS([gdbm.h], [], [AC_MSG_ERROR([Unable to find gdbm headers])])
MODULEDIR="${libdir}/buxton/modules"
AC_SUBST(MODULEDIR)
AC_DEFINE_UNQUOTED([MODULE_DIRECTORY], ["$MODULEDIR"], [' '])

# Layers are installed in the configuration directory
CONFIGDIR="${sysconfdir}/buxton"
AC_SUBST(CONFIGDIR)
AC_DEFINE_UNQUOTED([CONFIG_DIRECTORY], ["$CONFIGDIR"], [' '])

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

AC_CONFIG_FILES([
data/buxton.service
data/buxton.socket
test/test-pass.ini
test/test-fail.ini
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
