# Yoshimi — Experimental Windows LV2 Port

This repository contains an experimental native x86-64 Windows LV2 port
of the Yoshimi software synthesizer, based on Yoshimi 2.3.6.5.

The Windows port uses the existing Yoshimi synth engine and FLTK user
interface and has been built using MSYS2/UCRT64.

## Download

A ready-to-use Windows LV2 bundle is available from the
[experimental GitHub release](https://github.com/bjglover/yoshimi/releases/tag/windows-lv2-experimental-20260930c).

No MSYS2 or development tools are required to use the compiled LV2.

### Installation

1. Download and extract the release ZIP.
2. Copy the complete `yoshimi-windows-resources-20260930c.lv2` folder to:

   `%APPDATA%\LV2\`

3. Restart or rescan your LV2 host.
4. Look for **Yoshimi Windows Resources Test**.

Keep the complete `.lv2` folder intact, including its `resources` directory.

## Current test status

The Windows LV2 has been manually tested in REAPER with:

- plugin discovery and loading
- the FLTK user interface
- MIDI input and audio output
- factory patch banks
- 24 banks / 917 instruments

This remains an experimental port. Wider testing with other Windows
LV2 hosts, project/state restoration, multiple instances, automation
and long-running sessions is still useful.

## Technical documentation

- [Windows LV2 implementation and verification report](YOSHIMI_WINDOWS_LV2_PORT_REPORT.md)
- [Windows build and toolchain setup](YOSHIMI_WINDOWS_TOOLCHAIN_SETUP.md)

The toolchain document contains instructions for building the Windows
LV2 from source using MSYS2/UCRT64.

## Upstream Yoshimi

This project is a Windows LV2 port of
[Yoshimi](https://github.com/Yoshimi/yoshimi).

The original Yoshimi project, Linux build instructions, project history,
community information and current upstream development can be found in
the upstream repository.

This Windows port is experimental and is not an official Yoshimi
Windows release.

## License

Yoshimi is licensed under GPLv2+. See [COPYING](COPYING) for details.
