%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclxqilla
Summary:       Tcl extension for XQilla library
Version:       0.1.1
Release:       2
License:       Apache License, Version 2.0
Group:         Development/Libraries/Tcl
Source:        https://sites.google.com/site/ray2501/tclxqilla/tclxqilla_0.1.1.zip
URL:           https://sites.google.com/site/ray2501/tclxqilla
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.6
BuildRequires: xqilla-devel
BuildRoot:     %{buildroot}

%description
It is a Tcl extension for XQilla library.

This extension is using Tcl_LoadFile to load XQilla library.

%prep
%setup -q -n %{name}

%build
CFLAGS="%optflags" ./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}/tcl
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{directory}/%{_lib}/tcl/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{directory}/%{_lib}/tcl
