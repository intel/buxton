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
AC_CHECK_HEADERS([gdbm.h])
MODULEDIR="${libdir}/buxton/modules"
AC_SUBST(MODULEDIR)
AC_DEFINE_UNQUOTED([MODULE_DIRECTORY], ["$MODULEDIR"], [' '])

# Options
AC_ARG_WITH([systemdsystemunitdir], AC_HELP_STRING([--with-systemdsystemunitdir=DIR],
	[path to systemd system service directory]), [path_systemdsystemunit=${withval}],
	[path_systemdsystemunit="`$PKG_CONFIG --variable=systemdsystemunitdir systemd`"])
SYSTEMD_SYSTEMUNITDIR="${path_systemdsystemunit}"
AC_SUBST(SYSTEMD_SYSTEMUNITDIR)
AM_CONDITIONAL(SYSTEMD, test -n "${path_systemdsystemunit}")

AC_ARG_WITH([user], AC_HELP_STRING([--with-user=USER],
	[user to run buxton as]), [username=${withval}],
	[username="buxton"])
BUXTON_USERNAME="${username}"
AC_SUBST(BUXTON_USERNAME)

AC_OUTPUT([
data/buxton.service
data/buxton.socket
test/test-pass.ini
test/test-fail.ini
])

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
