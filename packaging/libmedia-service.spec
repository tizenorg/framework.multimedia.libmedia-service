Name:       libmedia-service
Summary:    Media information service library for multimedia applications.
Version: 0.2.148
Release:    1
Group:      System/Libraries
License:    Apache-2.0 and PD
Source0:    %{name}-%{version}.tar.gz

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(libexif)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(mm-fileinfo)
BuildRequires:  pkgconfig(media-thumbnail)
BuildRequires:  pkgconfig(libmedia-utils)
BuildRequires:  pkgconfig(uuid)
BuildRequires:  pkgconfig(icu-i18n)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(iniparser)

%description
Media information service library for multimedia applications.

%package devel
Summary:    Media information service library for multimedia applications. (development)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Media information service library for multimedia applications. (development files)

%prep
%setup -q 

%build
export CFLAGS+=" -Wextra -Wno-array-bounds"
export CFLAGS+=" -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow"
export CFLAGS+=" -Wwrite-strings -Wswitch-default"

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

#License
mkdir -p %{buildroot}/%{_datadir}/license
cp -rf %{_builddir}/%{name}-%{version}/LICENSE %{buildroot}/%{_datadir}/license/%{name}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest libmedia-service.manifest
%defattr(-,root,root,-)
%{_libdir}/libmedia-service.so
%{_libdir}/libmedia-service.so.1
%{_libdir}/libmedia-service.so.1.0.0
%{_libdir}/libmedia-svc-hash.so
%{_libdir}/libmedia-svc-hash.so.1
%{_libdir}/libmedia-svc-hash.so.1.0.0
%{_libdir}/libmedia-content-plugin.so
%{_libdir}/libmedia-content-plugin.so.1
%{_libdir}/libmedia-content-plugin.so.1.0.0
#License
%{_datadir}/license/%{name}

%files devel
%{_libdir}/pkgconfig/libmedia-service.pc
%{_includedir}/media-service/*.h
