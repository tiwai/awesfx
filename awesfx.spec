#
# spec file for package awesfx (Version 0.5.0)
#
# Copyright (c) 2004 SuSE Linux AG, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://www.suse.de/feedback/
#

# norootforbuild
# neededforbuild  alsa alsa-devel
# usedforbuild    aaa_base acl attr bash bind-utils bison bzip2 coreutils cpio cpp cracklib cvs cyrus-sasl db devs diffutils e2fsprogs file filesystem fillup findutils flex gawk gdbm-devel glibc glibc-devel glibc-locale gpm grep groff gzip info insserv kbd less libacl libattr libgcc libstdc++ libxcrypt m4 make man mktemp modutils ncurses ncurses-devel net-tools netcfg openldap2-client openssl pam pam-modules patch permissions popt ps rcs readline sed sendmail shadow strace syslogd sysvinit tar texinfo timezone unzip util-linux vim zlib zlib-devel alsa alsa-devel autoconf automake binutils gcc gdbm gettext libtool perl rpm

Name:         awesfx
Summary:      SoundFont utilities for SB AWE32/64 and Emu10k1 drivers
Version:      0.5.0
Release:      0
License:      GPL
Group:        Productivity/Multimedia/Sound/Midi
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
URL:          http://www.alsa-project.org/~iwai/awedrv.html
Source:       awesfx-%{version}.tar.bz2

%description
The AWESFX package includes utility programs for controlling the
wavetable function on SB AWE32/64 and Emu10k1 soundcards.



Authors:
--------
    Takashi Iwai <tiwai@suse.de>

%prep
%setup

%build
%{?suse_update_config:%{suse_update_config -f}}
autoreconf -fi
CFLAGS="$RPM_OPT_FLAGS" \
./configure --prefix=%{_prefix} --mandir=%{_mandir}
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

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

%changelog -n awesfx
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
