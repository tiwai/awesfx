#
# spec file for package awesfx (Version 0.4.4)
# 
# Copyright  (c)  2000  SuSE GmbH  Nuernberg, Germany.
#
# please send bugfixes or comments to feedback@suse.de.
#

# neededforbuild 
# usedforbuild    aaa_base aaa_dir autoconf automake base bash bindutil binutils bison bzip compress cpio cracklib devs diff ext2fs file fileutil find flex gawk gcc gdbm gettext gpm gppshare groff gzip kbd less libc libtool libz lx_suse make mktemp modules ncurses net_tool netcfg nkita nkitb nssv1 pam patch perl pgp ps rcs rpm sendmail sh_utils shadow shlibs strace syslogd sysvinit texinfo textutil timezone unzip util vim xdevel xf86 xshared

Vendor:       SuSE GmbH, Nuernberg, Germany
Distribution: SuSE Linux 7.0 (i386)
Name:         awesfx
Release:      0
Packager:     feedback@suse.de

Summary:      SoundFont utilities for SB AWE32/64 and Emu10k1 drivers
Version:      0.4.4
Copyright: GPL
Group: Applications/Sound
BuildRoot: /tmp/%{version}-%{version}-root
URL: http://members.tripod.de/iwai/awedrv.html
Source: awesfx-%{version}.tar.bz2
Patch: awesfx.dif

%description
SoundFont utilities for SB AWE32/64 and Emu10k1 drivers

Authors:
--------
    Takashi Iwai <tiwai@suse.de>

%prep
%setup

%build
cp Makefile-static Makefile
cp awelib/Makefile-static awelib/Makefile
make

%install
make INSTDIR=$RPM_BUILD_ROOT/usr install.bin
make BANKDIR=$RPM_BUILD_ROOT/usr/share/sfbank install.samples
make MANDIR=$RPM_BUILD_ROOT/%{_mandir} install.man
# normal check for compressed man pages:
%{?suse_check}

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/bin/*
/usr/share/sfbank/*
%doc docs/*
%doc %{_mandir}/*

%changelog -n awesfx
* Tue Aug 01 2000 - tiwai@suse.de
- Initial version: 0.4.4
