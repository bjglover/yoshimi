## Yoshimi

Yoshimi is a software audio synthesizer, currently only available for Linux.

### Experimental Windows LV2 port

This branch contains an experimental native x86-64 Windows LV2 port of Yoshimi 2.3.6.5.

The Windows LV2 has been built with MSYS2/UCRT64 and manually tested in REAPER with the FLTK user interface, MIDI/audio, and factory patch banks.

A ready-to-use Windows LV2 bundle is available from the [experimental GitHub release](https://github.com/bjglover/yoshimi/releases/tag/windows-lv2-experimental-20260930c).

Documentation:

* [Windows LV2 implementation report](YOSHIMI_WINDOWS_LV2_PORT_REPORT.md)
* [Windows build and toolchain setup](YOSHIMI_WINDOWS_TOOLCHAIN_SETUP.md)

This is an experimental port and is not an official Yoshimi Windows release.

### Current version

Version 2.3.6


* New feature: Kit Mode now has a crossfade Volume option as well as Velocity.

* New feature: Yoshimi now recognises old and new versions of MXML and FLTK.

* New feature: Yoshimi car run on the wayland windowng system without issues.

* Various code refinements.

### Building

Full build instructions are in [INSTALL](INSTALL).

### Source

Yoshimi source code is available from either:

* Sourceforge: https://sourceforge.net/projects/yoshimi
* Github: https://github.com/Yoshimi/yoshimi

### Community

Our list archive is at: https://www.freelists.org/archive/yoshimi

To post, email to: yoshimi@freelists.org

### License

GPLv2+ see [COPYING](COPYING) for license details.
