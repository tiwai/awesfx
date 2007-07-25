#
# spec file for package awesfx (Version 0.5.1)
#
# Copyright (c) 2004 SUSE LINUX AG, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://www.suse.de/feedback/
#

# norootforbuild

Name:         awesfx
BuildRequires: alsa-devel
Summary:      SoundFont utilities for SB AWE32/64 and Emu10k1 drivers
Version:      0.5.1
Release:      1
License:      GPL
Group:        Productivity/Multimedia/Sound/Midi
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://www.alsa-project.org/~iwai/awedrv.html
Source:       awesfx-%{version}.tar.bz2
Source1:      udev-soundfont
Source2:      load-soundfont
Source3:      41-soundfont.rules

%description
The AWESFX package includes utility programs for controlling the
wavetable function on SB AWE32/64 and Emu10k1 soundcards.



Authors:
--------
    Takashi Iwai <tiwai@suse.de>

%prep
%setup
%{?suse_update_config:%{suse_update_config -f}}

%build
autoreconf -fi
%configure
make

%install
make DESTDIR=$RPM_BUILD_ROOT install
# install udev and helper scripts
mkdir -p $RPM_BUILD_ROOT/etc/udev/rules.d
cp $RPM_SOURCE_DIR/*.rules $RPM_BUILD_ROOT/etc/udev/rules.d
mkdir -p $RPM_BUILD_ROOT/etc/alsa.d
cp %{SOURCE1} $RPM_BUILD_ROOT/etc/alsa.d
cp %{SOURCE2} $RPM_BUILD_ROOT/etc/alsa.d

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS COPYING README ChangeLog NEWS
%doc *.txt
%{_bindir}/*
%dir %{_datadir}/sounds/sf2
%{_datadir}/sounds/sf2/*
%doc %{_mandir}/*/*
/etc/udev
/etc/alsa.d

%changelog -n awesfx
* Wed Jan 25 2006 - mls@suse.de
- converted neededforbuild to BuildRequires
* Tue Aug 31 2004 - tiwai@suse.de
- updated to version 0.5.0d.  (misc fixes only)
* Fri Feb 27 2004 - tiwai@suse.de
- updated to version 0.5.0b.
* Wed Feb 04 2004 - tiwai@suse.de
- updated to version 0.5.0a.
* Fri Jan 23 2004 - tiwai@suse.de
- updated to version 0.5.0.  using auto-tools now.
  ALSA emux loader is added.
* Sun Jan 11 2004 - adrian@suse.de
- add %%defattr
* Fri Mar 08 2002 - kukuk@suse.de
- Add /usr/share/sounds/sf2 to filelist
* Tue Jan 08 2002 - tiwai@suse.de
- fixed buffer overflow with a long HOME env. variable.
* Fri Sep 21 2001 - tiwai@suse.de
- Fixed a bug in incremental loading: duplication of instrument
  layers are avoided.
* Tue Feb 27 2001 - tiwai@suse.de
- Apply the forgotten patch.
* Fri Jan 05 2001 - tiwai@suse.de
- Removed CVS directories from source archive.
- Strip installed binaries.
* Tue Dec 05 2000 - tiwai@suse.de
- Move default soundfont directory to /usr/share/sounds/sf2.
* Thu Sep 14 2000 - tiwai@suse.de
- Fixed version number and documents.
* Tue Aug 01 2000 - tiwai@suse.de
- Initial version: 0.4.4
