# declare vars
AM_CFLAGS = -std=gnu99 -fstack-protector -Wall -pedantic \
        -Wstrict-prototypes -Wundef -fno-common \
        -Werror-implicit-function-declaration \
        -Wformat -Wformat-security -Werror=format-security \
	-Wconversion -Wunreachable-code
if DEBUG
AM_CFLAGS += -ggdb3 -O0
endif
if COVERAGE
AM_CFLAGS += --coverage
endif

AM_CPPFLAGS = \
	-I $(top_srcdir)/src/include \
	-I $(top_srcdir)/src/core \
	-I $(top_srcdir)/src/security \
	-I $(top_srcdir)/src/shared \
	-D_MODULE_DIRECTORY=\"$(MODULEDIR)\" \
	-D_DEFAULT_CONFIGURATION_FILE=\"$(CONFPATH)\" \
	-D_DB_PATH=\"$(DB_PATH)\" \
	-D_BUXTON_SOCKET=\"$(BUXTON_SOCKET)\" \
	-D_SMACK_LOAD_FILE=\"$(SMACK_LOAD_FILE)\"

AM_LDFLAGS = \
	-rdynamic

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AUTOMAKE_OPTIONS = color-tests parallel-tests
SUBDIRS = .
noinst_LTLIBRARIES =
include_HEADERS =
lib_LTLIBRARIES =
pkglib_LTLIBRARIES =
DISTCHECK_CONFIGURE_FLAGS =  \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir) --enable-debug

systemdsystemunitdir = @SYSTEMD_SYSTEMUNITDIR@
systemdsystemunit_DATA = data/buxton.service data/buxton.socket

systemdsystemunit-install-hook:
	mkdir -p $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants
	ln -sf ../buxton.socket $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants/buxton.socket

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(localstatedir)/lib/buxton

install-data-hook: systemdsystemunit-install-hook

systemdsystemunit-uninstall-hook:
	rm -f $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants/buxton.socket

uninstall-hook: systemdsystemunit-uninstall-hook

distclean-local:
	rm -f log-check-stderr-file
	rm -f test/databases/*.db

# set library version info
LIBBUXTON_CURRENT=1
LIBBUXTON_REVISION=0
LIBBUXTON_AGE=0

pkgconfiglibdir=$(libdir)/pkgconfig
pkgconfiglib_DATA = \
	data/libbuxton.pc

if MANPAGE
dist_man_MANS = \
	docs/buxton.7 \
	docs/buxtonctl.1 \
	docs/buxton-api.7 \
	docs/buxton.conf.5 \
	docs/buxtond.8 \
	docs/buxton-protocol.7 \
	docs/buxton-security.7 \
	docs/buxton_client_handle_response.3 \
	docs/buxton_close.3 \
	docs/buxton_create_group.3 \
	docs/buxton_get_value.3 \
	docs/buxton_key_create.3 \
	docs/buxton_key_free.3 \
	docs/buxton_key_get_group.3 \
	docs/buxton_key_get_layer.3 \
	docs/buxton_key_get_name.3 \
	docs/buxton_key_get_type.3 \
	docs/buxton_open.3 \
	docs/buxton_register_notification.3 \
	docs/buxton_remove_group.3 \
	docs/buxton_response_key.3 \
	docs/buxton_response_status.3 \
	docs/buxton_response_type.3 \
	docs/buxton_response_value.3 \
	docs/buxton_set_conf_file.3 \
	docs/buxton_set_label.3 \
	docs/buxton_set_value.3 \
	docs/buxton_unregister_notification.3 \
	docs/buxton_unset_value.3
endif

TESTS = \
	check_db_clean \
	check_buxton \
	check_shared_lib \
	check_daemon \
	check_smack \
	check_configurator

if COVERAGE
coverage:
	mkdir -p coverage
	lcov --compat-libtool --directory . --capture --output-file coverage/report
	genhtml -o coverage/ coverage/report
endif

# set flags

EXTRA_DIST = \
	Doxyfile \
	LICENSE.LGPL2.1 \
	check_db_clean \
	data/libbuxton.pc.in \
	docs/LICENSE.MIT \
	src/libbuxton/lbuxton.sym \
	test/test.load2 \
	test/test.conf \
	test/test-configurator.conf

dist_sysconf_DATA = \
	data/buxton.conf

sbin_PROGRAMS = \
	buxtond

bin_PROGRAMS = \
	buxtonctl

buxtond_SOURCES = \
	src/core/daemon.c \
	src/core/daemon.h \
	src/core/main.c

buxtond_LDADD = \
	$(SYSTEMD_LIBS) \
	libbuxton-shared.la

buxtond_CFLAGS = \
	$(AM_CFLAGS)

buxtonctl_SOURCES = \
	src/cli/main.c \
	src/cli/client.c \
	src/cli/client.h

buxtonctl_CFLAGS = \
	$(AM_CFLAGS)

buxtonctl_LDADD = \
	libbuxton.la \
	libbuxton-shared.la

noinst_LTLIBRARIES += \
	libbuxton-shared.la

libbuxton_shared_la_SOURCES = \
	src/security/smack.c \
	src/security/smack.h \
	src/shared/backend.c \
	src/shared/backend.h \
	src/shared/buxtonarray.c \
	src/shared/buxtonarray.h \
	src/shared/buxtonclient.h \
	src/shared/buxtondata.h \
	src/shared/buxtonkey.h \
	src/shared/buxtonlist.c \
	src/shared/buxtonlist.h \
	src/shared/buxtonresponse.h \
	src/shared/buxtonstring.h \
	src/shared/configurator.c \
	src/shared/configurator.h \
	src/shared/direct.c \
	src/shared/direct.h \
	src/shared/hashmap.c \
	src/shared/hashmap.h \
	src/shared/list.h \
	src/shared/log.c \
	src/shared/log.h \
	src/shared/macro.h \
	src/shared/protocol.c \
	src/shared/protocol.h \
	src/shared/serialize.c \
	src/shared/serialize.h \
	src/shared/util.c \
	src/shared/util.h \
	${NULL}

if USE_LOCAL_INIPARSER
libbuxton_shared_la_SOURCES += \
	src/shared/dictionary.c \
	src/shared/dictionary.h \
	src/shared/iniparser.c \
	src/shared/iniparser.h
endif

libbuxton_shared_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-static

include_HEADERS += \
	src/include/buxton.h

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
	-lgdbm

memory_la_SOURCES = \
	src/db/memory.c

memory_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-fvisibility=hidden \
	-module \
	-avoid-version

check_PROGRAMS = \
	check_buxton \
	check_shared_lib \
	check_buxtond \
	check_daemon \
	check_smack \
	check_configurator

check_buxton_SOURCES = \
	test/check_utils.c \
	test/check_utils.h \
        test/check_buxton.c
check_buxton_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	-DMAKE_CHECK \
	-DABS_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
	-DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
check_buxton_LDADD = \
	@CHECK_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_shared_lib_SOURCES = \
	test/check_utils.c \
	test/check_utils.h \
        test/check_shared_lib.c
check_shared_lib_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK \
	-DABS_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
	-DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
check_shared_lib_LDADD = \
	@CHECK_LIBS@ \
	@INIPARSER_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_buxtond_SOURCES = \
	test/check_utils.c \
	test/check_utils.h \
	src/core/daemon.c \
	src/core/daemon.h \
	src/core/main.c
check_buxtond_CFLAGS = \
	@CHECK_CFLAGS@ \
	$(AM_CFLAGS) \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK \
	-DABS_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
	-DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
check_buxtond_LDADD = \
	@CHECK_LIBS@ \
	$(SYSTEMD_LIBS) \
	libbuxton.la \
	libbuxton-shared.la

check_daemon_SOURCES = \
	test/check_utils.c \
	test/check_utils.h \
	src/core/daemon.c \
	src/core/daemon.h \
        test/check_daemon.c
check_daemon_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK \
	-DABS_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
	-DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
check_daemon_LDADD = \
	@CHECK_LIBS@ \
	@INIPARSER_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_smack_SOURCES = \
	test/check_utils.c \
	test/check_utils.h \
	test/check_smack.c
check_smack_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK \
	-DABS_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
	-DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
check_smack_LDADD = \
	@CHECK_LIBS@ \
	@INIPARSER_LIBS@ \
	libbuxton.la \
	libbuxton-shared.la

check_configurator_SOURCES = \
	src/shared/configurator.c \
	src/shared/configurator.h \
	test/check_configurator.c
check_configurator_CFLAGS = \
	$(AM_CFLAGS) \
	@CHECK_CFLAGS@ \
	@INIPARSER_CFLAGS@ \
	-DMAKE_CHECK \
	-DABS_TOP_SRCDIR=\"$(abs_top_srcdir)\" \
	-DABS_TOP_BUILDDIR=\"$(abs_top_builddir)\"
check_configurator_LDADD = \
	@CHECK_LIBS@ \
	@INIPARSER_LIBS@ \
	libbuxton-shared.la

check_DATA = \
	test/test-pass.ini \
	test/test-fail.ini \
	test/test.conf \
	test/test.load2 \
	${NULL}

if BUILD_DEMOS
bin_PROGRAMS += \
	bxt_timing \
	bxt_hello_get \
	bxt_hello_set \
	bxt_hello_set_label \
	bxt_hello_create_group \
	bxt_hello_remove_group \
	bxt_hello_unset \
	bxt_hello_notify \
	bxt_hello_notify_multi

# Timing test
bxt_timing_SOURCES = \
	demo/timing.c
bxt_timing_LDADD = \
	libbuxton.la \
	libbuxton-shared.la \
	-lrt -lm

bxt_hello_get_SOURCES = \
	demo/helloget.c
bxt_hello_get_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_get_LDADD = \
	libbuxton.la

bxt_hello_set_SOURCES = \
	demo/helloset.c
bxt_hello_set_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_set_LDADD = \
	libbuxton.la

bxt_hello_set_label_SOURCES = \
	demo/hellosetlabel.c
bxt_hello_set_label_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_set_label_LDADD = \
	libbuxton.la

bxt_hello_create_group_SOURCES = \
	demo/hellocreategroup.c
bxt_hello_create_group_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_create_group_LDADD = \
	libbuxton.la

bxt_hello_remove_group_SOURCES = \
	demo/helloremovegroup.c
bxt_hello_remove_group_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_remove_group_LDADD = \
	libbuxton.la

bxt_hello_unset_SOURCES = \
	demo/hellounset.c
bxt_hello_unset_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_unset_LDADD = \
	libbuxton.la

bxt_hello_notify_SOURCES = \
	demo/hellonotify.c
bxt_hello_notify_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_notify_LDADD = \
	libbuxton.la

bxt_hello_notify_multi_SOURCES = \
	demo/notifytest.c
bxt_hello_notify_multi_CFLAGS = \
	$(AM_CFLAGS)
bxt_hello_notify_multi_LDADD = \
	libbuxton.la

if BUILD_GTK_DEMO
bin_PROGRAMS += \
	bxt_gtk_client
# GTK3 client demo
bxt_gtk_client_SOURCES = \
	demo/gtk_client.c \
	demo/gtk_client.h
bxt_gtk_client_LDADD = \
	$(GTK3_LIBS) \
	libbuxton.la \
	libbuxton-shared.la
bxt_gtk_client_CFLAGS = \
	$(GTK3_CFLAGS) \
	$(AM_CFLAGS)
endif

endif
