Summary: A portable interpreter for SCI games
Name: freesci
Version: 0.3.1
Release: 1
Group: Games/Adventure
Copyright: GPL
Source: ftp://ftp.shaftnet.org/pub/freesci/freesci-%{version}.tar.bz2
Buildroot: /var/tmp/%{name}-%{version}-%{release}-root
Prereq: ggi
BuildPrereq:  ggi-devel

%description
FreeSCI is a portable interpreter for SCI games, such as the Space Quest
series (starting with SQ3) or Leisure Suit Larry (2 and sequels).

It has the following improvements over Sierra SCI:
- SCI0 background pictures can be rendered in higher resolutions and 256 colors
- saving and restoring the game state is possible from more places than the
  Sierra SCI engine allowed (using the debugger functions)
- Better debugger (we believe :-)
- More portable
- It's Free software :-)


%prep
%setup -q

%build
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING INSTALL NEWS 
%doc THANKS TODO doc/sci.sgml
# %doc freesci-bugs
%{_bindir}/sciv
%{_bindir}/sciconsole
%{_bindir}/scidisasm
%{_bindir}/sciunpack
%{_libdir}/libsciengine.a
%{_mandir}/man6/

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Thu Apr 12 2001 Solomon Peachy <pizza@shaftnet.org>
- initial package for 0.3.1
