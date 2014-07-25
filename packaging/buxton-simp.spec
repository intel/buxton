Name:           buxton-simp
Version:        2
Release:        1
License:        LGPL-2.1+
Summary:        A simple security-enabled configuration system
Url:            https://github.com/sofar/buxton
Group:          System/Configuration
Source0:        %{name}-%{version}.tar.xz
Source1:        tizen.conf
Source1001:     %{name}.manifest
#Add the following back in when building with Tizen
#BuildRequires:  libattr-devel
#BuildRequires:  gdbm-devel
#BuildRequires:  pkgconfig(check)
#BuildRequires:  pkgconfig(systemd)
#BuildRequires:  pkgconfig(libsystemd-daemon)
#Requires(post): buxton
#Requires(post): smack
#Requires(post): /bin/chown

%description
Buxton is a security-enabled configuration management system. It
features a layered approach to configuration storage, with each
layer containing an arbitrary number of groups, each of which may
contain key-value pairs.  Mandatory Access Control (MAC) is
implemented at the group level and at the key-value level.

This version of Buxton provides a simpler version of Buxton's
C library (libbuxtonsimp) for client applications to
use.  Internally, buxton uses a daemon (buxtond) for processing
client requests and enforcing MAC. Also, a CLI (buxtonctl) is
provided for interactive use and for use in shell scripts.

%package devel
Summary: A security-enabled configuration system - development files
Requires: %{name} = %{version}

%description devel
Buxton is a security-enabled configuration management system. It
features a layered approach to configuration storage, with each
layer containing an arbitrary number of groups, each of which may
contain key-value pairs.  Mandatory Access Control (MAC) is
implemented at the group level and at the key-value level.

This version of Buxton provides a simpler version of Buxton's
C library (libbuxtonsimp) for client applications to
use.  Internally, buxton uses a daemon (buxtond) for processing
client requests and enforcing MAC. Also, a CLI (buxtonctl) is
provided for interactive use and for use in shell scripts.

This package provides development files for Buxton.

%package demos
Summary: demo files
Requires: %{name} = %{version}

%description demos

%prep
%setup -q
cp %{SOURCE1001} .

%build
%configure --enable-debug --enable-demos --with-systemdsystemunitdir=%{_libdir}/systemd/system/ --with-module-dir=%{_libdir}/buxton-simp/

make %{?_smp_mflags}

%install
%make_install
# TODO: need to define needed layers for Tizen in tizen.conf
install -m 0644 %{SOURCE1} %{buildroot}%{_sysconfdir}/buxton.conf

%post
/sbin/ldconfig
buxtonctl create-db base
buxtonctl create-db isp
if [ "$1" -eq 1 ] ; then
    # The initial DBs will not have the correct labels and
    # permissions when created in postinstall during image
    # creation, so we set these file attributes here.
    chsmack -a System %{_localstatedir}/lib/buxton/*.db
    chown buxton:buxton %{_localstatedir}/lib/buxton/*.db
fi

%postun -p /sbin/ldconfig

%docs_package
%license docs/LICENSE.MIT

%files
#Add this back in when building with Tizen
#%manifest %{name}.manifest
%license ./LICENSE.LGPL2.1
%config(noreplace) %{_sysconfdir}/buxton.conf
%{_bindir}/buxtonctl
%{_libdir}/buxton-simp/*.so
%{_libdir}/buxton-simp/*.la
%{_libdir}/libbuxton.so.*
%{_libdir}/libbuxtonsimp.so.*
%{_libdir}/systemd/system/buxton.service
%{_libdir}/systemd/system/buxton.socket
%{_libdir}/systemd/system/sockets.target.wants/buxton.socket
%{_sbindir}/buxtond
%attr(0700,buxton,buxton) %dir %{_localstatedir}/lib/buxton
#added these b/c rpmbuild was complaining
#should be able to take this out
#/sockets.target.wants/buxton.socket
%{_mandir}/man1/buxtonctl.1.gz
%{_mandir}/man3/buxton_client_handle_response.3.gz
%{_mandir}/man3/buxton_close.3.gz
%{_mandir}/man3/buxton_create_group.3.gz
%{_mandir}/man3/buxton_get_value.3.gz
%{_mandir}/man3/buxton_key_create.3.gz
%{_mandir}/man3/buxton_key_free.3.gz
%{_mandir}/man3/buxton_key_get_group.3.gz
%{_mandir}/man3/buxton_key_get_layer.3.gz
%{_mandir}/man3/buxton_key_get_name.3.gz
%{_mandir}/man3/buxton_key_get_type.3.gz
%{_mandir}/man3/buxton_open.3.gz
%{_mandir}/man3/buxton_register_notification.3.gz
%{_mandir}/man3/buxton_remove_group.3.gz
%{_mandir}/man3/buxton_response_key.3.gz
%{_mandir}/man3/buxton_response_status.3.gz
%{_mandir}/man3/buxton_response_type.3.gz
%{_mandir}/man3/buxton_response_value.3.gz
%{_mandir}/man3/buxton_set_conf_file.3.gz
%{_mandir}/man3/buxton_set_label.3.gz
%{_mandir}/man3/buxton_set_value.3.gz
%{_mandir}/man3/buxton_unregister_notification.3.gz
%{_mandir}/man3/buxton_unset_value.3.gz
%{_mandir}/man5/buxton.conf.5.gz
%{_mandir}/man7/buxton-api.7.gz
%{_mandir}/man7/buxton-protocol.7.gz
%{_mandir}/man7/buxton-security.7.gz
%{_mandir}/man7/buxton.7.gz
%{_mandir}/man8/buxtond.8.gz
%{_mandir}/man7/buxton-simp-api.7.gz
%{_mandir}/man3/buxtond_get_int32.3.gz
%{_mandir}/man3/buxtond_get_uint32.3.gz
%{_mandir}/man3/buxtond_get_string.3.gz
%{_mandir}/man3/buxtond_get_int64.3.gz
%{_mandir}/man3/buxtond_get_uint64.3.gz
%{_mandir}/man3/buxtond_get_float.3.gz
%{_mandir}/man3/buxtond_get_double.3.gz
%{_mandir}/man3/buxtond_get_bool.3.gz
%{_mandir}/man3/buxtond_set_int32.3.gz
%{_mandir}/man3/buxtond_set_uint32.3.gz
%{_mandir}/man3/buxtond_set_string.3.gz
%{_mandir}/man3/buxtond_set_int64.3.gz
%{_mandir}/man3/buxtond_set_uint64.3.gz
%{_mandir}/man3/buxtond_set_float.3.gz
%{_mandir}/man3/buxtond_set_double.3.gz
%{_mandir}/man3/buxtond_set_bool.3.gz
%{_mandir}/man3/buxtond_set_group.3.gz
%{_mandir}/man3/buxtond_remove_group.3.gz

%files devel
#Add this back in when building with Tizen
#%manifest %{name}.manifest
%{_includedir}/buxton.h
%{_includedir}/buxton-simp.h
%{_libdir}/libbuxton.so
%{_libdir}/libbuxtonsimp.so
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.la

%files demos
%{_bindir}/bxt_hello_create_group
%{_bindir}/bxt_hello_get
%{_bindir}/bxt_hello_notify
%{_bindir}/bxt_hello_notify_multi
%{_bindir}/bxt_hello_remove_group
%{_bindir}/bxt_hello_set
%{_bindir}/bxt_hello_set_label
%{_bindir}/bxt_hello_unset
%{_bindir}/bxt_timing
%{_bindir}/bxt_hello_buxton_simp
