Name:       libmedia-service
Summary:    Media information service library for multimedia applications.
Version: 0.2.0
Release:    6
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/libmedia-service.manifest 

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRequires:  cmake
BuildRequires:  edje-tools
BuildRequires:  expat-devel 
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mmutil-jpeg)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libexif)
BuildRequires:  pkgconfig(xdmcp)
BuildRequires:  pkgconfig(xrender)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libpng12)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(mm-fileinfo)
BuildRequires:  pkgconfig(media-thumbnail)
BuildRequires:  pkgconfig(drm-service)
BuildRequires:  pkgconfig(aul)

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

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build
cp %{SOURCE1001} .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

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
%{_libdir}/libmedia-svc-plugin.so
%{_libdir}/libmedia-svc-plugin.so.1
%{_libdir}/libmedia-svc-plugin.so.1.0.0

%files devel
%manifest libmedia-service.manifest
%{_libdir}/pkgconfig/libmedia-service.pc
%{_includedir}/media-service/*.h
