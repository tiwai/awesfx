%define ver	0.4.3c

Summary: AWESFX - library and utilities for AWE driver
Name: awesfx
Version: %ver
Release: 1
Copyright: GPL
Group: System/Libraries
Source: http://members.tripod.de/iwai/awesfx-%{ver}.tgz
BuildRoot: /tmp/rpmtest
Packager: Takashi Iwai <iwai@ww.uni-erlangen.de>
URL: http://members.tripod.de/iwai/awedrv.html
%description
================================================================
    AWE32 Sound Driver Utility Programs for Linux / FreeBSD
		version 0.4.3c; Jan. 16, 2000

			Takashi Iwai
		iwai@ww.uni-erlangen.de
		http://members.tripod.de/iwai
================================================================

----------------------------------------------------------------
* GENERAL NOTES

Thie package includes a couple of utilities for AWE32 sound driver on
Linux and FreeBSD systems.  You need to use these utilities to enable
sounds on the driver properly.

Ver.0.4.3 improves the parameter calculation as almost compatible with
the DOS/Win drivers.  This is more effective in the case of ROM sounds.
If you prefer the old type sounds, use --compatible option.

This packaing contains the following programs:

 - sfxload	SoundFont file loader
 - setfx	Chorus/reverb effect loader
 - aweset	Change the running mode of AWE driver
 - sf2text	Convert SoundFont to readable text
 - text2sf	Revert from text to SoundFont file
 - gusload	GUS PAT file loader
 - sfxtest	Example program to control AWE driver

The package includes the correction of SoundFont managing routines,
called AWElib.  As default, the AWElib is installed as a shared
library.

%prep
%setup
%build
cp Makefile-shared Makefile
cp awelib/Makefile-shared awelib/Makefile
make

%install
make INSTDIR="$RPM_BUILD_ROOT"/usr install

%post
ldconfig

%postun
ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/include/awe/*
/usr/bin/*
/usr/man/*
/usr/lib/libawe.so
/usr/lib/libawe.so.0.4.3
/usr/lib/sfbank/*

%doc docs/*

