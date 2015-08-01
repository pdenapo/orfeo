Summary: A text-mode player for CDs and MP3 files
Name: orpheus
Version: 1.6
Release: 1
Group: Applications/Multimedia
License: GPL
URL: http://konst.org.ua/orpheus/
Packager: Konstantin Klyagin
Source: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot/

%description
Orpheus is a text-mode player for CDs and files of MP3 format. It can
retrieve CDDB information for compact-discs, and save and load
playlists. Nice interface to modify MP3 ID tags is provided.

%prep
%setup

%build
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT/usr sysconfdir=$RPM_BUILD_ROOT/etc install

find $RPM_BUILD_ROOT/usr/ -type f -print | \
    grep -v '\/(README|COPYING|INSTALL|TODO|ChangeLog)$' | \
    sed "s@^$RPM_BUILD_ROOT@@g" | \
    sed 's/^\(.\+\/man.\+\)$/\1*/g' \
    > %{name}-%{version}-filelist

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}-%{version}-filelist
%defattr(-, root, root)

%doc README COPYING INSTALL TODO ChangeLog

%changelog
