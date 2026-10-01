# Recreating the Yoshimi Windows LV2 build environment

Observed: **1 October 2026**, native Windows x86-64, MSYS2 **UCRT64**, Yoshimi source version **2.3.6.5**. Intended audience: maintainers familiar with Linux/C++ who need a practical Windows build and deployment recipe.

## 0. If you just want to build it

**Source prerequisite:** use the complete Windows-port source tree, including its uncommitted modifications and new helper files. A plain clone of upstream Yoshimi does not currently reproduce this local port. This recipe describes what another developer should do; no installation, update, configuration or build was performed while writing this document.

1. Install the **x86-64 MSYS2** distribution, normally at `C:\msys64`, and open **MSYS2 UCRT64**, not the plain MSYS shell. The current installer requires Windows 10 x64 1809 or newer; that is the development-platform requirement, not a separately established minimum OS for the plugin. See [MSYS2 installation](https://www.msys2.org/).
2. On that new machine, complete a full package update. If pacman requests closing terminals, reopen UCRT64 and repeat the update until complete. MSYS2 supports full upgrades, not arbitrary partial upgrades. See [MSYS2 updating](https://www.msys2.org/docs/updating/).

```sh
# Run on the NEW development machine, not as part of this inspection task.
pacman -Syu
```

3. Install this lean package set. Its dependency closure was checked against the **actual installed database**, with no unresolved package names and with virtual providers accounted for. Git is for obtaining/reviewing source, not for compilation.

```sh
pacman -S --needed git \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-fltk \
  mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-fftw \
  mingw-w64-ucrt-x86_64-mxml \
  mingw-w64-ucrt-x86_64-lv2 \
  mingw-w64-ucrt-x86_64-readline
```

This is **eight native package roots plus Git**, not eight total installed packages. Today CMake pulls Ninja/pkgconf; GCC and Cairo/FLTK pull the runtime/static support; Cairo pulls Fontconfig/GLib; GLib pulls Python, which in turn pulls ncurses. Section 4 provides a less dependency-graph-sensitive explicit command.

4. Enter the complete port checkout and build a fresh **resource-test** identity:

```sh
cd ~/yoshimi
test "$MSYSTEM" = UCRT64 || { echo 'Open MSYS2 UCRT64 first'; exit 1; }
test -f src/Misc/WindowsPaths.cpp || { echo 'Windows port sources are missing'; exit 1; }

cmake -S src -B build-win-fresh -G Ninja \
  -DCMAKE_C_COMPILER="$(cygpath -m /ucrt64/bin/gcc.exe)" \
  -DCMAKE_CXX_COMPILER="$(cygpath -m /ucrt64/bin/g++.exe)" \
  -DCMAKE_INSTALL_PREFIX="$(cygpath -m "$PWD/stage-win-fresh")" \
  -DLV2Plugin=ON -DBuildWithFLTK=ON \
  -DBuildForThisCPU=OFF -DBuildForDebug=OFF -DBuildForDiagnostic=OFF \
  -DLV2_INSTALL_DIR=lib/lv2 \
  -DYOSHIMI_LV2_WINDOWS_DIAGNOSTICS=OFF \
  -DYOSHIMI_LV2_WINDOWS_CLEAN_TEST=OFF \
  -DYOSHIMI_LV2_WINDOWS_RESOURCE_TEST=ON

cmake --build build-win-fresh --target yoshimi_lv2 -j 4
cmake --install build-win-fresh --component lv2 \
  --prefix "$(cygpath -m "$PWD/stage-win-fresh")"
```

Use previously unused build/stage directory names. Do not overwrite preserved test/control bundles. The source-presence check is only an early guard, not verification that every necessary patch has been applied.

5. Copy the **whole** `stage-win-fresh/lib/lv2/yoshimi-windows-resources-20260930c.lv2` folder into `%APPDATA%\LV2`. Expect **Yoshimi Windows Resources Test** in REAPER. This historical identity is the one confirmed working with GUI, MIDI, audio and factory banks. A future production build should disable all three test switches, but that normal identity with the final implementation still requires its own real-host validation.

**Evidence limit:** the commands above are derived from the inspected successful configuration and installed dependency graph. This documentation task did not test a newly installed Windows PC or rerun a fresh checkout build. Section 12 distinguishes previously successful commands from this proposed reproduction recipe.

## 1. Scope, evidence and terminology

This guide complements [YOSHIMI_WINDOWS_LV2_PORT_REPORT.md](YOSHIMI_WINDOWS_LV2_PORT_REPORT.md). It focuses on recreating the **development environment**, not re-explaining every port change.

Inspected locally:

- `pacman -Q`, selected `pacman -Qi`, `pacman -Qdt`, and `/var/lib/pacman/local` package descriptions/file ownership lists.
- Native tool versions/paths, UCRT64 shell/profile definitions and inherited environment variables.
- Current `src/CMakeLists.txt`, `src/LV2_Plugin/CMakeLists.txt`, `build-win/CMakeCache.txt`, compiler-identification file and Ninja link rules.
- Installed `.pc` metadata, fresh read-only `pkg-config` queries, static archive existence/ownership, GCC library search results.
- Retained bundle/import/test artifacts and local Python/npm/Codex package metadata.

Initial MSYS Git/pacman queries failed inside the inspection sandbox with a Windows shared-memory access error. Read-only queries outside that sandbox succeeded; that is an inspection-runner restriction, not evidence of a broken Yoshimi toolchain. No package-management transaction, build, configure or plugin/REAPER execution was run.

Three different meanings of “dependency” must not be confused:

1. **Source/build requirement:** CMake or the linker directly needs a tool/header/library.
2. **Package dependency:** pacman installs a package because another selected package declares it, possibly only to support an installed tool. This is broader than the plugin link graph.
3. **End-user DLL dependency:** Windows must load a DLL when the final plugin is used. Static third-party libraries do not remain separate DLL requirements.

“Explicitly installed” in pacman's database is historical installation state, not proof a package is a direct Yoshimi requirement. `pacman -Qdt` returned no orphan package entries on this machine; that does not mean every installed package is needed by Yoshimi. No orphan cleanup was attempted.

## 2. MSYS2, UCRT64 and Windows path behavior

### 2.1 Why UCRT64

UCRT64 provides **GCC**, **x86-64 MinGW-w64**, **libstdc++** and Windows' newer **Universal C Runtime (UCRT)**. It builds native Windows executables/DLLs. The Unix-like shell helps run familiar development tools, but the resulting plugin does not require MSYS emulation. MSYS2 documents the environments and cautions against mixing static objects/libraries built for different CRT targets. See [MSYS2 environments](https://www.msys2.org/docs/environments/).

This is the environment actually used and validated here; there is no comparative benchmark claiming it is the only possible Windows toolchain. Consistency matters more than a terminal label: do not mix `/ucrt64` libraries with `/mingw64`, `/clang64` or `/usr` binaries/headers. An x86-64 host triple alone does not distinguish UCRT64 from another MinGW environment.

The installed `/etc/msystem.d/UCRT64` sets:

```text
MSYSTEM               UCRT64
MSYSTEM_PREFIX        /ucrt64
MSYSTEM_CARCH         x86_64
MSYSTEM_CHOST         x86_64-w64-mingw32
MINGW_PREFIX          /ucrt64
MINGW_PACKAGE_PREFIX  mingw-w64-ucrt-x86_64
```

These are the expected values; MSYSTEM, MINGW_PREFIX and MINGW_PACKAGE_PREFIX were directly present in the current process. The inspected GCC reports target `x86_64-w64-mingw32`, thread model **posix**, and a UCRT-configured runtime. “posix” here refers to GCC's thread model/winpthreads compatibility, not an MSYS-dependent plugin.

### 2.2 `/usr/bin` versus `/ucrt64/bin`

At the default install root:

| Shell path | Windows path | Purpose |
| --- | --- | --- |
| `/usr/bin` | `C:\msys64\usr\bin` | MSYS Bash, pacman, Git, cygpath and Unix shell utilities; may depend on `msys-2.0.dll` |
| `/ucrt64/bin` | `C:\msys64\ucrt64\bin` | Native Windows GCC/g++, CMake, Ninja, pkgconf, FLUID, Python, binutils and installed library DLLs |
| `/ucrt64/lib` | `C:\msys64\ucrt64\lib` | Native target archives/import libraries and pkg-config data |
| `/ucrt64/include` | `C:\msys64\ucrt64\include` | Native Windows target headers |

It is normal to use **MSYS Git/Bash/pacman/cygpath** to manage sources and invoke the native toolchain. It is not normal to link MSYS libraries into this native LV2. Paths with forward slashes are accepted by many Windows tools; `cygpath -m` emits a Windows-drive path with forward slashes suitable for CMake arguments.

### 2.3 PATH and pkg-config environment

The installed `/etc/profile` prepends the active native prefix before its MSYS utility paths and sets native pkg-config directories. The current session inherited:

```text
PKG_CONFIG_PATH                 /ucrt64/lib/pkgconfig:/ucrt64/share/pkgconfig
PKG_CONFIG_SYSTEM_INCLUDE_PATH  /ucrt64/include
PKG_CONFIG_SYSTEM_LIBRARY_PATH  /ucrt64/lib
```

Current `Get-Command` resolved GCC/g++, CMake, Ninja, pkg-config/pkgconf, objdump, Python and Node under `C:\msys64\ucrt64\bin`; Git under `C:\msys64\usr\bin`. The process PATH contains additional Codex/sandbox wrappers before the standard paths. Those wrappers are specific to the development session and **must not be copied as build requirements**.

`MSYS2_PATH_TYPE` was not set in the inspected process. `/etc/profile` defaults to a minimal Windows path, supports `inherit` and `strict`, and defines UCRT64 PATH plus native pkg-config paths. Inheriting a large global Windows PATH can let another CMake/compiler/pkg-config installation win unexpectedly. Use the UCRT64 launcher, not merely `export MSYSTEM=UCRT64` in an already initialized incorrect shell.

`CC`, `CXX`, `CMAKE_PREFIX_PATH` and `PKG_CONFIG_LIBDIR` were not set in the inspected process. A future environment may differ: inspect them and remove stale foreign overrides **in that future shell** if present. `PKG_CONFIG_PATH` augments lookup; `PKG_CONFIG_LIBDIR` can replace standard lookup locations and cause missing/wrong dependencies.

### 2.4 Read-only shell checks

Run in UCRT64:

```sh
printf 'MSYSTEM=%s\nMINGW_PREFIX=%s\nMINGW_PACKAGE_PREFIX=%s\n' \
  "$MSYSTEM" "$MINGW_PREFIX" "$MINGW_PACKAGE_PREFIX"
command -v gcc g++ cmake ninja pkg-config objdump git
gcc -dumpmachine
gcc -v
pkg-config --variable=prefix cairo
pkg-config --variable=pcfiledir cairo
pkg-config --modversion cairo fontconfig fftw3f mxml zlib lv2
```

Expected native tool paths start `/ucrt64/bin`; Git can be `/usr/bin/git`. Cairo's resolved prefix here is `C:/msys64/ucrt64/bin/..`, and its pcfiledir is the equivalent `.../lib/pkgconfig`. Do not mistake `bin/..` relocation output for a different prefix.

PowerShell alternatives: `Get-Command gcc,g++,cmake,ninja,pkg-config`; `where.exe gcc`; inspect `CMakeCache.txt` and invoke absolute tool paths. Simply finding MSYSTEM=UCRT64 is insufficient if CMake already cached another compiler.

## 3. Compiler and build utilities

Observed executable versions:

| Tool | Version | Role |
| --- | --- | --- |
| gcc / g++ | 16.2.0, MSYS2 Rev4 | Required C/C++ toolchain; C++17 project |
| CMake | 4.4.3 | Required configuration/install orchestration |
| Ninja | 1.13.2 | Required selected generator's build executor |
| pkg-config / pkgconf | 3.0.7 / 3.0.7 | Required `.pc` discovery, static dependency metadata; pkgconf provides pkg-config compatibility |
| GNU objdump / ar | 2.47.20260726 | Binutils: linker/assembler/archive tools are build requirements; objdump's PE analysis is verification work |
| FLUID | 1.4.5 (`fluid v1.4.5`) | Required generation of FLTK UI C++ from `.fl`; supplied by FLTK package |
| Git | 2.56.0 | Source acquisition/review; optional if a complete source archive is supplied |

Binutils is **not only a debugging package**: GCC needs its linker/assembler utilities. It normally arrives via GCC, so it does not need a separate minimal install root. `objdump` happens to be the binutils utility used for dependency/export/architecture investigation. GNU Make is installed but unnecessary for this Ninja build.

Current CMake identification records GNU compiler version 16.2.0 and platform MinGW. CMake's cache points to `/ucrt64/bin/cc.exe` and `c++.exe`; these native GCC driver names are valid aliases. The fresh recipe specifies gcc.exe/g++.exe explicitly to prevent unintended tool selection. The compiler's current default language level is not the project requirement: source CMake explicitly selects C++17/GNU++17.

Package versions can differ from executable banners. Examples: binutils package **2.47-3** versus executable **2.47.20260726**; pkgconf package **1~3.0.7-1** versus executable **3.0.7**. Preserve the package version including release/epoch syntax when recording a reproducible environment.

## 4. Packages: direct requirements versus transitive installation

### 4.1 Required interfaces and how the lean command supplies them

| Build need | Exact native package | Lean-command arrival | Why it is relevant |
| --- | --- | --- | --- |
| GCC C/C++ | `mingw-w64-ucrt-x86_64-gcc` | Explicit | Compiles LV2/shared code |
| MinGW headers/CRT/binutils/winpthreads | `...-headers`, `...-crt`, `...-binutils`, `...-winpthreads` | Via GCC | Native ABI, Win32 APIs, linker/assembler and pthread compatibility |
| CMake | `mingw-w64-ucrt-x86_64-cmake` | Explicit | Required build generator/configuration |
| Ninja/pkgconf | `...-ninja`, `...-pkgconf` | Via CMake today | Direct build requirements even though package installation is transitive |
| FLTK/FLUID/static GUI core | `mingw-w64-ucrt-x86_64-fltk` | Explicit | GUI and generated UI source |
| Cairo | `mingw-w64-ucrt-x86_64-cairo` | Explicit | GUI graph/dial drawing and native Win32 surfaces |
| Fontconfig | `mingw-w64-ucrt-x86_64-fontconfig` | Via Cairo | Direct CMake discovery and explicit LV2 link metadata |
| FFTW single precision | `mingw-w64-ucrt-x86_64-fftw` | Explicit | `fftw3f` module / `libfftw3f.a` |
| Mini-XML | `mingw-w64-ucrt-x86_64-mxml` | Explicit | MXML 3.3.1 / `mxml.pc`; code supports other versions but current static query uses `mxml` |
| zlib | `mingw-w64-ucrt-x86_64-zlib` | Via GCC/Cairo/FLTK | Direct CMake find/link; compression |
| LV2 headers/metadata definitions | `mingw-w64-ucrt-x86_64-lv2` | Explicit | No LV2 runtime DLL is needed; LV2 package supplies headers/specification |
| Readline | `mingw-w64-ucrt-x86_64-readline` | Explicit | Top-level configuration still requires it despite target-only LV2 build |
| ncurses | `mingw-w64-ucrt-x86_64-ncurses` | Via Python in the current Cairo/GLib closure | Top-level Curses find is required; not explicitly linked by final LV2 target |
| iconv / gettext runtime | `...-libiconv`, `...-gettext-runtime` | Via FLTK/GLib/binutils | Static text/i18n closure; CMake explicitly adds iconv |

Here `...-suffix` means the full `mingw-w64-ucrt-x86_64-suffix` name, not an MSYS package. Exact full names/versions are in Section 12.

### 4.2 Important transitive chains

Inspected package descriptions and `.pc` files establish:

```text
gcc -> binutils, headers, crt, winpthreads, libgcc, libstdc++, math/compiler support
cmake -> ninja, pkgconf, curl, libarchive, cppdap, jsoncpp, libuv, rhash, support libs
cairo -> fontconfig, freetype, pixman, glib2, gettext-runtime, libpng, zlib, lzo2
fontconfig -> freetype, expat
freetype <-> harfbuzz -> glib2, graphite2; font compression uses brotli/bzip2/png/zlib
glib2 -> gettext-runtime, libffi, pcre2, python, python-packaging, zlib
gettext-runtime -> libiconv
python -> ncurses, Tcl/Tk, OpenSSL, SQLite, compression and other supporting packages
fltk -> expat, gettext-runtime, iconv, png, JPEG, zlib, runtime support
readline -> termcap; ncurses -> libsystre/libtre and pcre2
fftw -> OpenMP provider; the installed provider is GCC libgomp
```

These are **package** chains; not everything named is linked into Yoshimi. CMake's curl/archive support, Python/Tcl/Tk/OpenSSL, JPEG/lzo2, etc. can arrive on a build machine without becoming plugin imports. The exact LV2 linker line uses the smaller static closure in Section 5. GLib's `.pc` static closure names pcre2/intl/system libs; its package metadata additionally needs Python/libffi utilities.

There is no standalone package named `mingw-w64-ucrt-x86_64-omp` installed here: it is a **virtual dependency provided by `mingw-w64-ucrt-x86_64-libgomp`**, verified with `pacman -Qi`. A naive name-only missing-package check would report a false failure. This Yoshimi target uses single-precision FFTW's base archive, not an explicit FFTW OpenMP link; the package's broader OpenMP requirement is still resolved by pacman.

The local install reason says GLib was installed as a dependency, while GCC/CMake/FFTW/winpthreads were explicitly installed. A toolchain group can mark many group members explicit; that history is not the best minimal recipe. No complete toolchain group, GDB, GNU Make, Node or Codex is needed as an additional compile prerequisite.

### 4.3 A more explicit setup command

For maintainers who prefer every direct configuration/link interface to be visible, use this instead of the lean command:

```sh
pacman -S --needed git \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-fltk \
  mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-fontconfig \
  mingw-w64-ucrt-x86_64-fftw \
  mingw-w64-ucrt-x86_64-mxml \
  mingw-w64-ucrt-x86_64-zlib \
  mingw-w64-ucrt-x86_64-lv2 \
  mingw-w64-ucrt-x86_64-libiconv \
  mingw-w64-ucrt-x86_64-readline \
  mingw-w64-ucrt-x86_64-ncurses
```

This matches the earlier report's core explicit native list, with Git added for source acquisition. It is not a request to install every package currently present. FreeType/HarfBuzz/pixman/GLib/gettext/compression libraries are normally installed transitively. You can explicitly name those packages for an audit/CI declaration, but doing so is redundant under this observed graph; do not freeze only a few library versions while leaving a mismatched runtime/toolchain.

No native JACK, ALSA, argp, X11 server or Visual Studio Build Tools installation is part of this target recipe. The MSYS `libargp` package happens to be installed for MSYS tools; that is not a native UCRT argp solution or a Yoshimi LV2 requirement.

## 5. Static linking and verified library archives

### 5.1 Why this deployment uses static third-party libraries

The earlier dynamic plugin and bundled dependency DLLs were discoverable in REAPER but rejected. A retained normal-loader test reproduced **Windows error 126** with MSYS2 absent from PATH: several imports could not be resolved by normal search. Loading an absolute plugin filename does not necessarily make its own directory a dependency-search location. A host can change that policy, but users should not need to alter global PATH or put DLLs in REAPER's installation directory.

The MinGW LV2 CMake branch therefore uses `-static -static-libgcc -static-libstdc++`, a static FLTK archive, pkg-config's static library closure, a linker group and explicit iconv. Third-party code and toolchain runtimes are embedded in the module. Windows system/UCRT DLL imports remain. The old normal identity's later REAPER failure was a separate unresolved problem; static linking demonstrably fixes the **local loader search issue**, not every possible host rejection.

### 5.2 What `pkg-config --static` does

`pkg-config --libs` normally describes a DLL-oriented client link. `--static --libs` adds the `Requires.private` and `Libs.private` closure necessary to resolve references from static archives. It does **not** itself force the linker to choose static files. The CMake MinGW linker flags and explicit FLTK archive provide that selection.

Inspected ordinary query:

```sh
pkg-config --libs cairo fontconfig fftw3f mxml zlib
```

```text
-LC:/msys64/ucrt64/bin/../lib -lcairo -lfontconfig -lfftw3f -lmxml -lz
```

Actual static query:

```sh
pkg-config --static --libs cairo fontconfig fftw3f mxml zlib
```

```text
-LC:/msys64/ucrt64/bin/../lib -lcairo -lm -lgdi32 -lmsimg32 -ldwrite -ld2d1
-lwindowscodecs -lpixman-1 -lm -pthread -lfontconfig -lm -lfreetype -lbz2 -lpng16
-lm -lharfbuzz -lm -lusp10 -lgdi32 -lrpcrt4 -luser32 -ldwrite -lglib-2.0 -lintl
-lws2_32 -lole32 -lwinmm -lshlwapi -luuid -latomic -lm -lpcre2-8 -lgraphite2
-lbrotlidec -lbrotlicommon -lexpat -lm -lfftw3f -lm -lmxml -lz
```

Line breaks above are for readability. CMake adds `/ucrt64/lib/libfltk.a`, `iconv`, `comctl32`, `ws2_32`, `shell32`, and `CAIRO_WIN32_STATIC_BUILD`. It wraps the static closure in `--start-group/--end-group` because libraries including FreeType/HarfBuzz have cyclic references. Native Win32 libraries are import libraries even if their files end `.a`; “`.a` exists” is not proof that the Windows system runtime was statically embedded.

The inspected Ninja link rule matches this design and contains `-shared -static -static-libgcc -static-libstdc++`. Its final libraries include repeated closure entries, so do not infer dependency count from occurrences of `-l...`. The final PE import table is the deployment check.

### 5.3 Archive existence and package ownership

Every archive below was verified present in `C:\msys64\ucrt64\lib`, and its owner was checked against the local package file database. `libfltk.dll.a` also exists but is an **import library**, not the required static FLTK implementation.

| Static archive(s) | Owning package suffix | Use |
| --- | --- | --- |
| `libfltk.a` | fltk | Explicit GUI core, 3,266,798 bytes |
| `libcairo.a` | cairo | Drawing |
| `libfontconfig.a` | fontconfig | Font discovery |
| `libfreetype.a` | freetype | Font rendering |
| `libharfbuzz.a` | harfbuzz | Text shaping |
| `libpixman-1.a` | pixman | Pixel operations |
| `libfftw3f.a` | fftw | Required single-precision FFT implementation |
| `libmxml.a` | mxml | Mini-XML |
| `libz.a` | zlib | Compression |
| `libglib-2.0.a` | glib2 | Font/text static closure |
| `libintl.a` | gettext-runtime | Translation/i18n runtime |
| `libiconv.a` | libiconv | Character conversion; added explicitly by link rule |
| `libpcre2-8.a` | pcre2 | GLib regex support |
| `libgraphite2.a` | graphite2 | HarfBuzz shaping closure |
| `libbrotlidec.a`, `libbrotlicommon.a` | brotli | Font compression |
| `libexpat.a` | expat | Fontconfig XML |
| `libbz2.a` | bzip2 | Font compression |
| `libpng16.a` | libpng | Font/drawing image closure |
| `libpthread.a`, `libwinpthread.a` | winpthreads | Static pthread implementation |
| `libncurses.a` | ncurses | Present for top-level configuration; not explicitly in LV2 link list |
| `libreadline.a` | readline | Present; configuration selects its DLL import library for standalone, not LV2 |

GCC `-print-file-name=...` also located existing `libstdc++.a`, `libgcc.a`, `libatomic.a`, `libmingw32.a`, `libmingwex.a`, `libucrt.a` and `libwinpthread.a`. The toolchain archive paths include its `lib/gcc/x86_64-w64-mingw32/16.2.0` directory. `libucrt.a` is a system-runtime interface/import archive, not proof of a self-contained UCRT replacement.

The **winpthreads** package owns static `libpthread.a`/`libwinpthread.a`; **libwinpthread** supplies the separate runtime DLL package used by other development tools. The final plugin does not import `libwinpthread-1.dll`.

Read-only checks on a new development machine:

```sh
test -f /ucrt64/lib/libfltk.a
test -f /ucrt64/lib/libfftw3f.a
test -f /ucrt64/lib/libintl.a
test -f /ucrt64/lib/libiconv.a
pacman -Qo /ucrt64/lib/libfltk.a /ucrt64/lib/libintl.a /ucrt64/lib/libpthread.a
pkg-config --print-requires-private cairo
gcc -print-file-name=libstdc++.a
gcc -print-file-name=libgcc.a
```

Never substitute `/usr/lib` archives or another MinGW CRT prefix to satisfy a missing file.

## 6. Yoshimi-specific CMake configuration

| Option/setting | Successful value | Meaning |
| --- | --- | --- |
| `LV2Plugin` | ON | Adds LV2 target; does not remove the standalone target from the project |
| `BuildWithFLTK` | ON | Builds functional GUI and runs FLUID |
| `BuildForThisCPU` | OFF | Avoids `-march=native/-mtune=native` for portable distribution; other architecture-tuning switches also OFF |
| `BuildForDebug` | OFF | Current build uses optimized Release behavior |
| `BuildForDiagnostic` | OFF | General optimized debug-symbol option; **not** the Windows logging option |
| `LV2_INSTALL_DIR` | `lib/lv2` | Relative staging subdirectory, not the final user AppData destination |
| `YOSHIMI_LV2_WINDOWS_DIAGNOSTICS` | OFF | Opt-in diagnostic identity `a`, logger/TLS/DllMain signals |
| `YOSHIMI_LV2_WINDOWS_CLEAN_TEST` | OFF | Opt-in fresh clean identity `b`, no logging implementation |
| `YOSHIMI_LV2_WINDOWS_RESOURCE_TEST` | ON | Historically tested resource identity `c` |
| `CMAKE_GENERATOR` | Ninja | Native Ninja executable from UCRT64 |
| `WINDOWS_LV2_FLTK` | `/ucrt64/lib/libfltk.a` | Explicit static GUI archive found by CMake |

The test switches default OFF and are mutually exclusive on Windows. Production fallback is normal plugin/UI URI, `Yoshimi`/`Yoshimi-Multi`, `yoshimi_lv2.dll`, `yoshimi.lv2`. Do not confuse experimental names with a permanent fork of the LV2 interface.

Current `CMAKE_BUILD_TYPE` cache text is empty, but top-level CMake sets Release in directory scope and the retained install log says Release. Effective flags include C++17, `-ffast-math -fomit-frame-pointer -DNDEBUG -O3`, plus existing warnings/security flags. Merely setting a cache build type is not the primary Debug control for this project; use its BuildForDebug/BuildForDiagnostic options.

The cache's default `CMAKE_INSTALL_PREFIX` is `C:/Program Files (x86)/Yoshimi`, despite the actual x86-64 compiler. A directory name does not determine PE architecture. For staging, explicitly choose a user-writable prefix or pass `cmake --install --prefix`, as the successful packaging did.

Current source CMake already guards argp/JACK requirements on Windows and uses Windows-compatible version/install-directory quoting. Readline and curses are still required at top-level configure. `MXML` discovery accepts `mxml4` or `mxml`, but the MinGW static dependency query specifically asks for `mxml`. A future MXML-only-v4 package layout may need a source/build-system adjustment; installing unreviewed alternate headers is not a validated workaround.

**Build only `yoshimi_lv2`.** A default `cmake --build ...` still includes the standalone executable, with backend/CLI assumptions outside the current port scope. The install component `lv2` contains just the plugin, metadata and Windows bundled resources. Ordinary full installation can try to install an unbuilt standalone executable; component staging avoids that.

## 7. Complete source/build/stage procedure

### 7.1 Obtain the right source snapshot

For an upstream starting point, the local origin URL is:

```sh
cd ~
git clone https://github.com/Yoshimi/yoshimi.git yoshimi
cd yoshimi
```

**Stop here until the Windows port patch/source snapshot is supplied.** The local checkout has a pre-existing initial-build commit and further uncommitted changes. No published branch/tag is identified containing all of them. A `.diff` of tracked files alone omits these currently untracked implementation files:

```text
src/LV2_Plugin/PluginIdentity.h
src/LV2_Plugin/WindowsDiagnostics.cpp
src/LV2_Plugin/WindowsDiagnostics.h
src/Misc/WindowsPaths.cpp
src/Misc/WindowsPaths.h
```

The diagnostics header is included even when its implementation is disabled, so omitting it can break a clean build. Factory `banks`, `presets` and `doc` trees must be present for component staging. Apply the full reviewed patch or enter an already complete port snapshot. This guide does not modify/publish a patch or fetch source on the current machine.

### 7.2 Build in a fresh directory

Use the complete quick-start configure/build/install block above. The explicit `cygpath -m` compiler/install paths adapt to a nondefault MSYS2 root, unlike copying `C:/msys64/...` literals into a different installation. Choose fresh directories instead of reusing a CMake cache created under MSYS, MINGW64, Visual Studio or another compiler.

Verify after configure on that future machine:

```sh
grep -E '^(CMAKE_(C_COMPILER|CXX_COMPILER|MAKE_PROGRAM)|WINDOWS_LV2_FLTK|LV2_INSTALL_DIR|YOSHIMI_LV2_WINDOWS_)' \
  build-win-fresh/CMakeCache.txt
```

Expected paths resolve under your UCRT64 prefix; the static FLTK archive is `libfltk.a`, not `libfltk.dll.a`. The module's target name stays **yoshimi_lv2** even when its filename has an experimental identity.

### 7.3 Bundle output and installation

Fresh resource output:

```text
build-win-fresh/LV2_Plugin/yoshimi_windows_resources_20260930c.dll
stage-win-fresh/lib/lv2/yoshimi-windows-resources-20260930c.lv2/
  manifest.ttl
  yoshimi.ttl
  yoshimi_windows_resources_20260930c.dll
  resources/{banks,presets,examples,doc,COPYING}
```

The Windows component install copies resource directories; no separate Unix-style share tree or manual creation of user bank directories is needed. The plugin creates `%LOCALAPPDATA%\Yoshimi` for writable content and `%APPDATA%\Yoshimi` for settings when it starts.

Install the entire `.lv2` folder to `%APPDATA%\LV2`; on this machine the Windows user destination is `C:\Users\beng\AppData\Roaming\LV2`. Existing known-working control bundles can remain installed. Restart/rescan the host as appropriate, choose the resource-test name, show its GUI and play a factory bank patch. Do not replace an in-use DLL or alter REAPER's program directory.

### 7.4 Production identity versus historical test

A future production-candidate configure should use all three Windows test options OFF. It then stages `.../lib/lv2/yoshimi.lv2` and normal `yoshimi_lv2.dll`, with static libraries and factory resources still enabled. That switch combination is **source-supported but not established as working in REAPER with the final resources**, because the old normal identity failed earlier. The successful fresh-identity experiments did not conclusively isolate host caching versus metadata/identity differences. Record actual loaded paths and validate production identity before release.

### 7.5 Read-only output checks

For the staged module on a future build machine:

```sh
objdump -f stage-win-fresh/lib/lv2/yoshimi-windows-resources-20260930c.lv2/yoshimi_windows_resources_20260930c.dll
objdump -p stage-win-fresh/lib/lv2/yoshimi-windows-resources-20260930c.lv2/yoshimi_windows_resources_20260930c.dll \
  | grep 'DLL Name:'
```

Expect `pei-x86-64`, LV2 descriptor exports and system/UCRT imports, not MSYS, GCC, FLTK, Cairo, FFTW or other non-system DLL imports. A full local host probe is stronger than import inspection; real REAPER instantiation remains distinct from a loader test. No new probe was run during this task.

## 8. Development, debugging and verification tools

| Tool | Observed version / location | Classification |
| --- | --- | --- |
| Python | 3.14.7, `/ucrt64/bin/python.exe`; package 3.14.7-1 | Used for packaging/audits; also a **transitive installed package** through GLib. Yoshimi CMake/Ninja did not invoke Python |
| rdflib | 7.6.0 in `build-win/verification/venv/lib/python3.14/site-packages` | Verification-only Turtle parsing; metadata checked without importing/running it |
| pyparsing / pip | 3.3.3 / 26.2.1 in that venv | rdflib dependency / Python environment management; not plugin dependencies |
| objdump | GNU Binutils 2.47.20260726 | PE architecture/import/export audits; included with required binutils |
| Custom C loader/LV2 probes | Local `load-probe.c`, `resource-probe.c` and executables | Verification-only, compiled with the same native GCC; not shipped inside LV2 |
| GDB / gdb-multiarch | 18.1 executable; packages 18.1-3 | Installed optional debuggers; installing the whole toolchain group is unnecessary just for this build |
| GNU Make | Package 4.4.1-5 | Optional alternate utility; this build uses Ninja |
| Node.js | v24.21.0, package 24.21.0-1, `/ucrt64/bin/node.exe` | Optional coding-agent/tooling runtime; **not a Yoshimi compile/link/runtime dependency** |
| npm | 11.19.0 from installed npm/package.json | Optional Node package management; version read without running install/update commands |
| Codex CLI | `@openai/codex` 0.159.2; platform package 0.159.2-win32-x64 | Optional development assistant; version verified from local package metadata, not by launching the agent |

Node/npm/Codex are under the UCRT64 installation here, but location does not make them part of the synth. Inspection found no Node/npm/Codex/Python command in the two CMake files or generated build Ninja rules. Native Python arrives through package metadata and runs verification scripts separately; it is not an embedded Python synth dependency.

Codex's installed PowerShell shim launches Node plus its package JS and selected native executable. Those wrapper paths explain development-session PATH additions. Developers do **not** need Codex, a coding-agent account, Node or npm to compile or run Yoshimi. No Codex installation/configuration/authentication instructions are required for recreating the compiler environment.

The existing verification venv reports Python 3.14.7 and `include-system-site-packages=false`; rdflib is installed there rather than as a required pacman LV2 dependency. LV2's package lists developer docs, Python spec-generation libraries and `sord` validation as **optional** dependencies. They are not necessary to compile this target. A future verifier can create its own venv and install rdflib, but that optional installation was not performed here.

Existing scripts have machine/identity/control paths hardcoded. Adapt them to fresh directories and isolated APPDATA/LOCALAPPDATA before using them; older diagnostic probes may write logs and control-bundle sidecars. Running them blindly is not a read-only operation.

## 9. End-user runtime requirements

An ordinary user needs:

- A supported **64-bit Windows** system with the required system/UCRT facilities.
- A **64-bit LV2 host**; the tested host is Windows REAPER.
- The complete self-contained `.lv2` bundle, including metadata and factory `resources`.
- Writable normal Windows user profile AppData directories for configuration/editable content.

They do **not** need MSYS2, GCC, MinGW headers, CMake, Ninja, pkg-config, FLUID, Git, Python, rdflib, Node, npm, Codex, GDB, development libraries or MSYS2 on PATH. No third-party runtime DLL collection is required beside the final plugin, and no copying into REAPER's program directory is needed.

The inspected resource artifact reports **28 system/API-set import entries**, no unresolved non-system dependencies and only the plugin DLL at bundle root. System imports include Kernel32/User32/GDI32, DirectWrite, common-dialog/control, shell/COM/security/network DLLs and `api-ms-win-crt-*` contracts. Static linking does not remove those operating-system dependencies or constitute an audited minimum Windows version. UCRT is included in current Windows systems as described in the [MSYS2 environment documentation](https://www.msys2.org/docs/environments/); older Windows support has not been established.

The retained system-only-PATH/native loader tests were on the development machine, where MSYS2 remained installed. They strongly support independence from development DLL searching but do not replace an actual clean-machine deployment test, including optional library configuration/font/dynamic-loading behavior.

## 10. Troubleshooting

| Symptom/mistake | Check | Correct approach on the future build machine |
| --- | --- | --- |
| Plain MSYS shell or MSYS compiler selected | MSYSTEM, `command -v gcc`, cached compiler; MSYS target/runtime | Open UCRT64; use native `/ucrt64/bin` tools and a new build directory |
| UCRT64 mixed with MINGW64/CLANG64 libraries | Include/link prefixes, `.pc` paths, pkg-config overrides | Use matching `mingw-w64-ucrt-x86_64-*` packages; do not combine CRT-specific archives |
| Another Windows compiler/CMake wins PATH | `command -v`, `Get-Command`, CMake identification | Explicit native compiler paths and consistent native CMake/Ninja |
| Compiler selection remains wrong after switching shells | `CMakeCache.txt`, CMake compiler identification | Start a fresh build directory; shell switching does not invalidate cached tool paths |
| Static FLTK not found | `test -f /ucrt64/lib/libfltk.a`, owner/package | Install correct UCRT64 FLTK archive package; `.dll.a` is not a substitute |
| Static link misses font/text/compression symbols | Compare `pkg-config --static --libs` with Ninja link line | Correct static closure, group ordering and iconv; do not delete core functionality |
| pkg-config resolves `/usr`, `/mingw64` or unexpected prefix | `--variable=prefix`, `pcfiledir`, PKG_CONFIG_* | Fix native prefix environment; avoid stale custom `PKG_CONFIG_LIBDIR` |
| Readline/curses fails despite LV2-only goal | Top-level required CMake finds | Install native readline/ncurses; target-only building does not eliminate existing configure checks |
| MXML package/module mismatch | `pkg-config --modversion mxml`, archive | Use the observed mxml3 module or review a deliberate CMake/source adaptation for newer layouts |
| Unexpected standalone/argp/backend compile failure | Requested build target | `cmake --build ... --target yoshimi_lv2`, not default all |
| Install tries missing standalone executable | Install command | `cmake --install ... --component lv2` with writable staging prefix |
| Plugin references `.so` or an older identity | Generated staged manifest and DLL name | Copy generated metadata plus matching module, not source manifest verbatim |
| Windows load error 126 | PE import closure, PATH-isolated plain loader | Confirm actual static output; bundle-adjacent DLLs alone are not guaranteed to resolve |
| REAPER still shows an old/rejected identity | Exact bundle path, descriptor/UI URI and rescan/restart | Keep working control; verify scanned identity; do not claim the unresolved old failure is a proven cache issue |
| Empty banks | Entire resources tree and writable AppData paths | Copy full component-installed bundle, not just its DLL; use native resource-capable build |
| Build seems to target Program Files (x86) | Compiler/platform/PE format versus prefix | Folder naming is not architecture; use explicit staging prefix and inspect x86-64 PE |

Avoid solving native link issues by adding arbitrary global Windows PATH entries or copying libraries from unrelated ecosystems. No troubleshooting changes in this table were applied during documentation.

## 11. Cross-check against the earlier port report

The earlier report's listed GCC, CMake, Ninja, pkgconf, FLTK, Cairo, Fontconfig, FFTW, MXML, LV2, zlib, readline, ncurses, iconv and winpthreads **package versions match the current database**. There is no detected drift in that cited core list since 30 September. This guide adds exact executable banners, package ownership and a broader closure; it does not blindly reuse the old values.

Important refinements/discrepancies of classification rather than numerical version:

1. **Binutils** was described primarily as a verification tool. Its objdump use is verification-only, but binutils also supplies the linker/assembler required by GCC and arrives transitively with GCC.
2. **Python** is not only an optionally installed verification aid: it is a hard dependency in the installed GLib package graph. No Python invocation appears in Yoshimi's build rules, and the end-user plugin has no Python import.
3. **gettext** needs the exact native package **gettext-runtime 1.0-1**, supplying `libintl.a`. The separate MSYS `gettext/libintl 0.22.5-1` packages serve MSYS tools; they are not the native link closure.
4. **winpthreads versus libwinpthread**: the former supplies the static archives, the latter a runtime DLL for other developer tools. Both are installed, but the final plugin has no DLL import on libwinpthread.
5. **FreeType version notation**: native package 2.14.3-1, while `freetype2.pc` reports **26.6.20**, its library-interface version. These are not evidence of an accidental second FreeType installation.
6. **Minimal install list**: the earlier 14-native-package explicit command remains useful; the installed graph permits an eight-native-root lean command, without removing required interfaces. The larger command is less sensitive to future package-edge changes.
7. **Fresh build instructions**: the earlier report explicitly noted they had not been rerun on a newly provisioned PC. That limitation still applies. This guide uses `cygpath -m` for compiler/prefix arguments to make Windows path conversion explicit.

The original port report remains unchanged. No new successful fresh-machine or production-identity claim is introduced here.

## 12. Reference: observed versions, package inventory and build commands

### 12.1 Exact observed tool versions

```text
gcc/g++                 16.2.0 (Rev4, Built by MSYS2 project)
target / thread model   x86_64-w64-mingw32 / posix
CMake                   4.4.3
Ninja                   1.13.2
pkg-config/pkgconf      3.0.7
objdump/ar              GNU Binutils 2.47.20260726
FLUID                   1.4.5
Git                     2.56.0
Python                  3.14.7
GDB                     18.1
Node.js                 v24.21.0
npm package metadata    11.19.0
Codex package metadata  0.159.2 (platform package 0.159.2-win32-x64)
rdflib / pyparsing      7.6.0 / 3.3.3, local verification venv
pip                     26.2.1, same venv
```

No npm/Codex install/update command or agent session was run to obtain those metadata versions.

### 12.2 Exact relevant package versions and classification

The following tables are generated from this machine's **installed local database**, not the online repository's future latest versions. Prefixes are written in full. Category codes:

- **R**: lean command's explicit native root.
- **B**: transitive but directly used by compiler/configuration or static plugin link.
- **T**: transitive package supporting another development tool/package, not explicitly named by the plugin's link rule.
- **V/O**: verification/development or optional tools outside the lean native package graph.
- **MSYS**: host utility/base package; not native plugin code.

Several packages legitimately have more than one role. The database's historical install reason is not used to assign the category.

#### Native dependency closure of the eight recommended roots

| Installed package | Exact installed version | Role |
| --- | --- | --- |
| `mingw-w64-ucrt-x86_64-binutils` | `2.47-3` | B |
| `mingw-w64-ucrt-x86_64-brotli` | `1.2.0-1` | B |
| `mingw-w64-ucrt-x86_64-bzip2` | `1.0.8-4` | B |
| `mingw-w64-ucrt-x86_64-c-ares` | `1.34.8-1` | T |
| `mingw-w64-ucrt-x86_64-ca-certificates` | `20260816-1` | T |
| `mingw-w64-ucrt-x86_64-cairo` | `1.18.6-2` | R |
| `mingw-w64-ucrt-x86_64-cc-libs` | `16.2.0-4` | B |
| `mingw-w64-ucrt-x86_64-cmake` | `4.4.3-3` | R |
| `mingw-w64-ucrt-x86_64-cppdap` | `1.65.r11.g6464cd7-1` | T |
| `mingw-w64-ucrt-x86_64-crt` | `14.0.0.r426.g4564ee4b5-1` | B |
| `mingw-w64-ucrt-x86_64-curl` | `8.22.0-1` | T |
| `mingw-w64-ucrt-x86_64-expat` | `2.8.5-1` | B |
| `mingw-w64-ucrt-x86_64-fftw` | `3.3.11-1` | R |
| `mingw-w64-ucrt-x86_64-fltk` | `1.4.5-1` | R |
| `mingw-w64-ucrt-x86_64-fontconfig` | `2.18.3-1` | B |
| `mingw-w64-ucrt-x86_64-freetype` | `2.14.3-1` | B |
| `mingw-w64-ucrt-x86_64-gcc` | `16.2.0-4` | R |
| `mingw-w64-ucrt-x86_64-gcc-libs` | `16.2.0-4` | B |
| `mingw-w64-ucrt-x86_64-gettext-runtime` | `1.0-1` | B |
| `mingw-w64-ucrt-x86_64-glib2` | `2.90.0-1` | B |
| `mingw-w64-ucrt-x86_64-gmp` | `6.3.0-2` | T |
| `mingw-w64-ucrt-x86_64-gnutls` | `3.8.13-3` | T |
| `mingw-w64-ucrt-x86_64-graphite2` | `1.3.15-1` | B |
| `mingw-w64-ucrt-x86_64-harfbuzz` | `14.5.0-1` | B |
| `mingw-w64-ucrt-x86_64-headers` | `14.0.0.r426.g4564ee4b5-1` | B |
| `mingw-w64-ucrt-x86_64-isl` | `0.28-1` | T |
| `mingw-w64-ucrt-x86_64-jsoncpp` | `1.9.8-1` | T |
| `mingw-w64-ucrt-x86_64-libarchive` | `3.8.9-6` | T |
| `mingw-w64-ucrt-x86_64-libatomic` | `16.2.0-4` | B |
| `mingw-w64-ucrt-x86_64-libb2` | `0.98.1-3` | T |
| `mingw-w64-ucrt-x86_64-libffi` | `3.8.0-1` | T |
| `mingw-w64-ucrt-x86_64-libgcc` | `16.2.0-4` | B |
| `mingw-w64-ucrt-x86_64-libgomp` | `16.2.0-4` | T |
| `mingw-w64-ucrt-x86_64-libiconv` | `1.19-1` | B |
| `mingw-w64-ucrt-x86_64-libidn2` | `2.3.8-4` | T |
| `mingw-w64-ucrt-x86_64-libjpeg-turbo` | `3.2.0-1` | T |
| `mingw-w64-ucrt-x86_64-libpng` | `1.6.59-1` | B |
| `mingw-w64-ucrt-x86_64-libpsl` | `0.21.5-3` | T |
| `mingw-w64-ucrt-x86_64-libquadmath` | `16.2.0-4` | T |
| `mingw-w64-ucrt-x86_64-libssh2` | `1.11.1-2` | T |
| `mingw-w64-ucrt-x86_64-libstdc++` | `16.2.0-4` | B |
| `mingw-w64-ucrt-x86_64-libsystre` | `1.0.2-3` | T |
| `mingw-w64-ucrt-x86_64-libtasn1` | `4.21.0-1` | T |
| `mingw-w64-ucrt-x86_64-libtre` | `0.9.0-2` | T |
| `mingw-w64-ucrt-x86_64-libunistring` | `1.4.2-1` | T |
| `mingw-w64-ucrt-x86_64-libuv` | `1.53.0-1` | T |
| `mingw-w64-ucrt-x86_64-libwinpthread` | `14.0.0.r426.g4564ee4b5-1` | B |
| `mingw-w64-ucrt-x86_64-lv2` | `1.18.10-1` | R |
| `mingw-w64-ucrt-x86_64-lz4` | `1.10.0-1` | T |
| `mingw-w64-ucrt-x86_64-lzo2` | `2.10-3` | T |
| `mingw-w64-ucrt-x86_64-mpc` | `1.4.1-1` | T |
| `mingw-w64-ucrt-x86_64-mpdecimal` | `4.0.1-3` | T |
| `mingw-w64-ucrt-x86_64-mpfr` | `4.2.2-3` | T |
| `mingw-w64-ucrt-x86_64-mxml` | `3.3.1-4` | R |
| `mingw-w64-ucrt-x86_64-ncurses` | `6.6-4` | B |
| `mingw-w64-ucrt-x86_64-nettle` | `4.0-1` | T |
| `mingw-w64-ucrt-x86_64-nghttp2` | `1.70.0-1` | T |
| `mingw-w64-ucrt-x86_64-nghttp3` | `1.18.0-1` | T |
| `mingw-w64-ucrt-x86_64-ngtcp2` | `1.25.0-1` | T |
| `mingw-w64-ucrt-x86_64-ninja` | `1.13.2-1` | B |
| `mingw-w64-ucrt-x86_64-openssl` | `3.6.5-1` | T |
| `mingw-w64-ucrt-x86_64-p11-kit` | `0.26.5-1` | T |
| `mingw-w64-ucrt-x86_64-pcre2` | `10.49-1` | B |
| `mingw-w64-ucrt-x86_64-pixman` | `0.46.4-3` | B |
| `mingw-w64-ucrt-x86_64-pkgconf` | `1~3.0.7-1` | B |
| `mingw-w64-ucrt-x86_64-python` | `3.14.7-1` | T |
| `mingw-w64-ucrt-x86_64-python-packaging` | `26.3-1` | T |
| `mingw-w64-ucrt-x86_64-readline` | `8.3.003-1` | R |
| `mingw-w64-ucrt-x86_64-rhash` | `1.4.6-1` | T |
| `mingw-w64-ucrt-x86_64-sqlite3` | `3.53.4-1` | T |
| `mingw-w64-ucrt-x86_64-tcl` | `8.6.18-1` | T |
| `mingw-w64-ucrt-x86_64-termcap` | `1.3.1-7` | T |
| `mingw-w64-ucrt-x86_64-tk` | `8.6.18-1` | T |
| `mingw-w64-ucrt-x86_64-tzdata` | `2026d-1` | T |
| `mingw-w64-ucrt-x86_64-windows-default-manifest` | `20260815-1` | T |
| `mingw-w64-ucrt-x86_64-wineditline` | `2.208-1` | T |
| `mingw-w64-ucrt-x86_64-winpthreads` | `14.0.0.r426.g4564ee4b5-1` | B |
| `mingw-w64-ucrt-x86_64-xz` | `5.8.4-1` | T |
| `mingw-w64-ucrt-x86_64-zlib` | `1.3.2-2` | B |
| `mingw-w64-ucrt-x86_64-zstd` | `1.5.7-2` | T |

#### Additional installed development tools outside that closure

| Installed package | Exact installed version | Role |
| --- | --- | --- |
| `mingw-w64-ucrt-x86_64-gdb` | `18.1-3` | V/O |
| `mingw-w64-ucrt-x86_64-gdb-multiarch` | `18.1-3` | V/O |
| `mingw-w64-ucrt-x86_64-make` | `4.4.1-5` | V/O |
| `mingw-w64-ucrt-x86_64-nodejs` | `24.21.0-1` | V/O |

#### MSYS host/base tools used for setup and inspection

| Installed package | Exact installed version | Role |
| --- | --- | --- |
| `base` | `2022.06-1` | MSYS |
| `bash` | `5.3.020-1` | MSYS |
| `filesystem` | `2026.03.06-1` | MSYS |
| `git` | `2.56.0-1` | MSYS |
| `msys2-launcher` | `1.5-3` | MSYS |
| `msys2-runtime` | `3.6.10-6` | MSYS |
| `pacman` | `6.1.0-25` | MSYS |
| `which` | `2.25-1` | MSYS |

The native closure contains 80 installed packages. This is a dependency-name/provider graph, not a claim that every archive from every package is linked into Yoshimi. Native `libgomp` provides FFTW's virtual `omp` dependency. No dependency names in this local graph were unresolved. Packages outside the closure are not required merely because they are installed.


### 12.3 Recommended minimal package-install command

Repeated here for a maintainer's environment checklist; run only on the machine being set up:

```sh
pacman -S --needed git \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-fltk mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-fftw mingw-w64-ucrt-x86_64-mxml \
  mingw-w64-ucrt-x86_64-lv2 mingw-w64-ucrt-x86_64-readline
```

Its dependency resolution was inspected locally, not tested as an installation transaction. Prefer Section 4.3's explicit interfaces if a future pacman graph no longer supplies Ninja/pkgconf/fontconfig/zlib/iconv/ncurses transitively.

### 12.4 Previously successful build commands and current evidence

The existing target/install artifacts and cache support these commands in the original checkout. They are a record, **not commands executed by this inspection**:

```powershell
# PowerShell on the original development machine, with existing configured build-win.
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-win --target yoshimi_lv2 -j 4
cmake --install build-win --component lv2 --prefix C:/msys64/home/beng/yoshimi/build-win/test-install-resources-20260930c
```

The complete fresh configure/build/stage command block is in Section 0; it pins the same target/options but uses fresh paths and explicit compilers. It is **inspection-derived**, not a new validated clean-machine build.

Retained actual output:

```text
C:\msys64\home\beng\yoshimi\build-win\test-install-resources-20260930c\lib\lv2\yoshimi-windows-resources-20260930c.lv2
```

The factory-resource verifier recorded 24 banks/917 instruments, native loading/MIDI/audio/cleanup and user-file preservation. The user confirmed this resource identity in REAPER. Those tests validate the retained build, not a future dependency snapshot after package updates.

### 12.5 Final dependency classification checklist

| Category | Examples | Developer action | End-user requirement |
| --- | --- | --- | --- |
| Direct source/build interfaces | GCC, CMake, Ninja, pkgconf, FLTK/FLUID, Cairo, Fontconfig, FFTW, MXML, zlib, LV2, readline/curses | Supply through lean roots or explicit command; verify native prefix/static archives | None separately |
| Transitive compiled/static support | FreeType, HarfBuzz, pixman, GLib, intl/iconv, pcre2, compression/XML, winpthreads, GCC runtimes | Let pacman resolve matching UCRT64 versions; preserve closure record | Embedded in plugin as applicable |
| Transitive development-tool package support | Python, Tcl/Tk, OpenSSL/curl/archive support and compiler arithmetic libraries | Normally arrives automatically; do not confuse with final plugin link graph | None separately |
| Verification-only workflows | Python audit scripts, rdflib, loader/LV2 probes, objdump analysis | Optional for compilation; strongly useful for validating distribution | None |
| Optional development aids | Node/npm/Codex, GDB, GNU Make, LV2 docs/sord/spec tools | Install only for the desired workflow | None |
| Host/base development utilities | MSYS2 Bash/pacman/cygpath, Git | Needed for this setup workflow; Git optional with a source archive | No MSYS runtime requirement |
| Runtime prerequisites | Windows system/UCRT facilities, x64 LV2 host, complete bundle, writable AppData | Test on supported Windows versions | Required |

## 13. Known unknowns and clean-machine reproducibility caveats

1. **Source distribution is the primary blocker.** All port changes are not available in a named published commit/tag: tracked modifications and five new implementation files must be transferred together. Neither a plain upstream clone nor only `git diff` reproduces the code. This task did not stage/commit/publish them.
2. **Rolling package versions are not locked.** The tables describe installed versions today, not availability of those exact files in future repositories. A clean setup with `pacman -Syu/-S` can obtain a different dependency graph, MXML layout or static archive set. Preserve coherent package artifacts/manifests for a release; do not partially downgrade a few compiler/runtime packages. The local GCC 16.2.0-4 archive and signature were observed in `/var/cache/pacman/pkg`, but completeness of a restorable full snapshot was not verified.
3. **Lean graph depends on current packaging.** In particular ncurses currently arrives through Cairo → GLib → Python, and Ninja/pkgconf through CMake. The explicit command makes direct interfaces clearer. A graph walk validated installed names/providers, not a new remote solver/transaction or all future constraints.
4. **No clean-PC reconstruction performed.** The current development installation contains many tools beyond the recommended roots. PATH-isolated DLL tests do not prove absence of every optional runtime file/config/font dependency on a machine without MSYS2 installed. Fresh build and clean-host installation still need to be run.
5. **Historical versus production identity.** Resource test `20260930c` works in REAPER; normal identity with the final implementation remains a separate validation requirement. The reason the old normal static identity failed has not been conclusively explained as caching.
6. **Hardcoded local paths remain in verification artifacts.** `build-win` probes/verifiers/reference logs and control-bundle assumptions are not a portable CI suite. Do not use the older packaging script to overwrite the known-working clean control; prefer CMake's LV2 component install.
7. **Static and platform assumptions remain.** This recipe covers MinGW UCRT64 x86-64, not MSVC, ARM64, 32-bit Windows or the standalone application. Static linking still depends on the package-provided archives and coherent system import libraries. End-user minimum Windows/CPU policy has not been certified; the MSYS2 installer minimum is a different question.
8. **Language/path/data breadth is not complete.** The current resource helpers use narrow/ANSI paths; Unicode/long paths, interrupted/concurrent seeding, user-data migration and versioned factory upgrades need additional engineering/testing. Existing bank copies are not a transactional factory update system.
9. **Host/regression testing is still limited.** Production state save/reload, concurrent instances, complete Multi routing, broad sample-rate/buffer matrices, GUI close/reopen, long-running real-time stability and Linux behavior comparison are not established by the short probes or this environment inspection.
10. **This document creates instructions, not an environment change.** Only this Markdown report was intentionally created. No Yoshimi source/configuration or previous report was edited; no packages/software were installed/updated/removed; no build/plugin/REAPER execution, staging, commit or push was performed. Source/configuration fingerprints were compared before and after documentation.
