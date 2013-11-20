# declare vars
AM_CFLAGS = -std=gnu99 -fstack-protector -Wall -pedantic \
        -Wstrict-prototypes -Wundef -fno-common \
        -Werror-implicit-function-declaration \
        -Wformat -Wformat-security -Werror=format-security \
	-Wconversion
if DEBUG
AM_CFLAGS += -ggdb3 -O0
endif

AM_CPPFLAGS = \
	-I $(top_srcdir)/src/include \
	-I $(top_srcdir)/src/shared \
	-D_MODULE_DIRECTORY=\"$(MODULEDIR)\" \
	-D_DEFAULT_CONFIGURATION_FILE=\"$(CONFPATH)\" \
	-D_DB_PATH=\"$(DB_PATH)\" \
	-D_BUXTON_SOCKET=\"$(BUXTON_SOCKET)\" \
	-D_SMACK_LOAD_FILE=\"$(SMACK_LOAD_FILE)\"

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AUTOMAKE_OPTIONS = color-tests parallel-tests
SUBDIRS = .
noinst_LTLIBRARIES =
include_HEADERS =
lib_LTLIBRARIES =
pkglib_LTLIBRARIES =
DISTCHECK_CONFIGURE_FLAGS =  \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

systemdsystemunitdir = @SYSTEMD_SYSTEMUNITDIR@
systemdsystemunit_DATA = data/buxton.service data/buxton.socket

systemdsystemunit-install-hook:
	mkdir -p $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants
	ln -sf ../buxton.socket $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants/buxton.socket

install-data-hook: systemdsystemunit-install-hook

systemdsystemunit-uninstall-hook:
	rm -f $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants/buxton.socket

uninstall-hook: systemdsystemunit-uninstall-hook

distclean-local:
	rm -f log-check-stderr-file

# set library version info
LIBBUXTON_CURRENT=0
LIBBUXTON_REVISION=1
LIBBUXTON_AGE=0

TESTS = check_buxton check_shared_lib check_daemon

# set flags

EXTRA_DIST = \
	src/libbuxton/lbuxton.sym \
	src/shared/constants.h \
	test/test.load2 \
	test/test.conf

dist_sysconf_DATA = \
	data/buxton.conf

sbin_PROGRAMS = \
	bt-daemon

bin_PROGRAMS = \
	buxtonctl

bt_daemon_SOURCES = \
	src/shared/constants.c \
	src/core/main.c

bt_daemon_LDADD = \
	$(SYSTEMD_LIBS) \
	libbuxton.la \
	libbuxton-shared.la

buxtonctl_SOURCES = \
	src/shared/constants.c \
	src/cli/main.c

buxtonctl_LDADD = \
	libbuxton.la \
	libbuxton-shared.la

noinst_LTLIBRARIES += \
	libbuxton-shared.la

libbuxton_shared_la_SOURCES = \
	src/shared/util.c \
	src/shared/list.h \
	src/shared/log.c \
	src/shared/log.h \
	src/shared/backend.h \
	src/shared/hashmap.c \
	src/shared/hashmap.h \
	src/shared/macro.h \
	src/shared/util.h \
	src/shared/serialize.c \
	src/shared/serialize.h \
	src/shared/smack.c \
	src/shared/smack.h \
	src/shared/protocol.c \
	src/shared/protocol.h \
	src/shared/daemon.c \
	src/shared/daemon.h \
	src/shared/client.c \
	src/shared/client.h \
	${NULL}

if USE_LOCAL_INIPARSER
libbuxton_shared_la_SOURCES += \
	src/shared/dictionary.c \
	src/shared/dictionary.h \
	src/shared/iniparser.c \
	src/shared/iniparser.h
endif

include_HEADERS += \
	src/include/bt-daemon.h

lib_LTLIBRARIES += \
	libbuxton.la

libbuxton_la_SOURCES = \
	src/libbuxton/lbuxton.c

libbuxton_la_CFLAGS = \
	$(AM_CFLAGS) \
	@INIPARSER_CFLAGS@ \
	-fvisibility=hidden

libbuxton_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	@INIPARSER_LIBS@ \
	-version-info $(LIBBUXTON_CURRENT):$(LIBBUXTON_REVISION):$(LIBBUXTON_AGE) \
	-Wl,--version-script=$(top_srcdir)/src/libbuxton/lbuxton.sym

libbuxton_la_LIBADD = \
	libbuxton-shared.la \
	-ldl

pkglib_LTLIBRARIES += \
	gdbm.la \
	memory.la

gdbm_la_SOURCES =  \
	src/db/gdbm.c

gdbm_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-fvisibility=hidden \
	-module \
	-avoid-version

gdbm_la_LIBADD = \
	libbuxton-shared.la \
	-lgdbm

memory_la_SOURCES = \
	src/db/memory.c

memory_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-fvisibility=hidden \
	-module \
	-avoid-version

memory_la_LIBADD = \
	libbuxton-shared.la

check_PROGRAMS = \
	check_buxton \
	check_shared_lib \
	check_bt_daemon \
	check_daemon

check_buxton_SOURCES = \
	src/shared/constants.c \
	test/check_utils.c \
	test/check_utils.h \
        test/check_buxton.c
check_buxton_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	-DMAKE_CHECK
check_buxton_LDADD = \
	@CHECK_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_shared_lib_SOURCES = \
	src/shared/constants.c \
	test/check_utils.c \
	test/check_utils.h \
        test/check_shared_lib.c
check_shared_lib_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK
check_shared_lib_LDADD = \
	@CHECK_LIBS@ \
	@INIPARSER_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_bt_daemon_SOURCES = \
	src/shared/constants.c \
	test/check_utils.c \
	test/check_utils.h \
	src/core/main.c
check_bt_daemon_CFLAGS = \
	@CHECK_CFLAGS@ \
	$(AM_CFLAGS) \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK
check_bt_daemon_LDADD = \
	@CHECK_LIBS@ \
	$(SYSTEMD_LIBS) \
	libbuxton.la \
	libbuxton-shared.la

check_daemon_SOURCES = \
	src/shared/constants.c \
	test/check_utils.c \
	test/check_utils.h \
        test/check_daemon.c
check_daemon_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK
check_daemon_LDADD = \
	@CHECK_LIBS@ \
	@INIPARSER_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_DATA = \
	test/test-pass.ini \
	test/test-fail.ini \
	test/test.conf \
	test/test.load2 \
	${NULL}

if BUILD_DEMOS
bin_PROGRAMS += \
	bxt_timing

# Timing test
bxt_timing_SOURCES = \
	demo/timing.c \
	src/shared/constants.c
bxt_timing_LDADD = \
	libbuxton.la \
	libbuxton-shared.la \
	-lrt -lm
endif
