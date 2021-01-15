%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclxqilla
Summary:       Tcl extension for XQilla library
Version:       0.1.2
Release:       2
License:       Apache-2.0
Group:         Development/Libraries/Tcl
Source:        %{name}-%{version}.tar.gz
URL:           https://sites.google.com/site/ray2501/tclxqilla
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.6
BuildRequires: xqilla-devel
Requires:      xqilla
Requires:      tcl >= 8.6
BuildRoot:     %{buildroot}

%description
It is a Tcl extension for XQilla library.

This extension is using Tcl_LoadFile to load XQilla library.

%prep
%setup -q -n %{name}-%{version}

%build
CFLAGS="%optflags" ./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{tcl_archdir}
