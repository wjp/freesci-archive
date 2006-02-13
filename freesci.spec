Summary: A portable interpreter for SCI games
Name: freesci
Version: 0.6.2
Release: 1
Group: Games/Adventure
License: GPL
Source: http://www-plan.cs.colorado.edu/creichen/freesci/freesci-%{version}.tar.bz2
Buildroot: /var/tmp/%{name}-%{version}-%{release}-root
#Prereq: ggi, SDL >= 1.1.8
#BuildPrereq:  ggi-devel, SDL-devel >= 1.1.8

%description
FreeSCI is a portable interpreter for SCI games, such as the Space Quest
series (starting with SQ3) or Leisure Suit Larry (2 and sequels).

This is the unstable/development branch of FreeSCI.  Expect breakages not present
in the stable version.  Also note that no savegames are supported at this point.

It has the following improvements over Sierra SCI:
- SCI0 background pictures can be rendered in higher resolutions and 256 colors
- Better debugger (we believe :-)
- More portable
- It's Free software :-)


%prep
%setup -q

%build
./configure --prefix=/usr --mandir=%{_mandir}
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING INSTALL NEWS 
%doc THANKS TODO doc/sci.tex
%doc README*
%{_bindir}/sciv
%{_bindir}/scidisasm
%{_bindir}/sciunpack
%{_bindir}/freesci
%{_bindir}/freesci-setup
%{_mandir}/man6/
/usr/bin/bdftofont
/usr/bin/fonttoc
/usr/bin/scipack
/usr/share/applnk/Games/Adventure/FreeSCI.desktop
/usr/share/games/freesci/config.template
/usr/share/icons/hicolor/48x48/apps/freesci.png

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Thu May 10 2001 Solomon Peachy <pizza@shaftnet.org>
- Mangled through autoconf now.
- got rid of freesci-bugs
- workarounds for manpath issues
- got rid of SDL/ggi dependencies

* Thu Apr 12 2001 Solomon Peachy <pizza@shaftnet.org>
- initial package for 0.3.1

