#a new .spec file for buxton-simp, still a work in progress
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
BuildRequires:  libattr-devel
BuildRequires:  gdbm-devel
BuildRequires:  pkgconfig(check)
BuildRequires:  pkgconfig(systemd)
BuildRequires:  pkgconfig(libsystemd-daemon)
Requires(post): buxton
Requires(post): smack
Requires(post): /bin/chown

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
%configure --enable-debug --enable-demos

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
#%license docs/LICENSE.MIT

%files
%manifest %{name}.manifest
#%license /LICENSE.LGPL2.1
%config(noreplace) %{_sysconfdir}/buxton.conf
%{_bindir}/buxtonctl
%{_libdir}/buxton/*.so
%{_libdir}/libbuxton.so.*
%{_libdir}/libbuxtonsimp.so.*
%{_prefix}/lib/systemd/system/buxton.service
%{_prefix}/lib/systemd/system/buxton.socket
%{_prefix}/lib/systemd/system/sockets.target.wants/buxton.socket
%{_sbindir}/buxtond
%attr(0700,buxton,buxton) %dir %{_localstatedir}/lib/buxton

%files devel
%manifest %{name}.manifest
%{_includedir}/buxton.h
%{_includedir}/buxton-simp.h
%{_libdir}/libbuxton.so
%{_libdir}/pkgconfig/*.pc

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
