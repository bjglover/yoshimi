# Yoshimi native Windows LV2 port: implementation and verification report

Date: 30 September 2026. Source version: **2.3.6.5**. Scope: native x86-64 Windows LV2 with the existing FLTK GUI; the standalone application is not the porting target.

This report describes the actual local working tree and retained build/test artifacts. The checkout HEAD is `daaacf33c694c6018d970e427dc51069e7150ba9` (`Initial Windows LV2 build fixes`). That pre-existing commit already contains the initial CMake argp/JACK guards and quoting fixes. Subsequent portability, deployment, diagnostics and resource changes are uncommitted. Writing this report did not change source, build a target, run plugin tests, launch/modify REAPER, stage files, commit, or push.

Evidence labels used below:

- **Automated**: retained probe/script results and their inspected implementations.
- **Manual REAPER**: the user's reported host tests; the diagnostic host log additionally corroborates some startup behavior.
- **Inspection**: source, diffs, configuration, resource inventory and read-only PE inspection.
- **Incomplete**: plausible explanation or untested behavior, explicitly identified.

Relative file links refer to this checkout. Local test artifacts under `build-win` are evidence, not files to submit as the production source patch.

# 1. Executive summary

Yoshimi has been built as a **native 64-bit Windows LV2 DLL** using MSYS2's **UCRT64** MinGW-w64 toolchain, CMake and Ninja. This is a Windows PE module using the Universal C Runtime, not an MSYS-emulated/Linux executable. The instrument's existing DSP, MIDI handling, LV2 interfaces and FLTK GUI remain enabled.

The current deployable experiment is **Yoshimi Windows Resources Test**, identity `20260930c`. It includes factory content and resolves writable paths through Windows AppData conventions. The user has confirmed that it loads in 64-bit REAPER, displays the GUI, receives REAPER MIDI, synthesizes audio, displays populated factory banks, and plays factory patches including arpeggios. Earlier diagnostic and clean fresh-identity builds were also confirmed working in REAPER.

Automated verification exercised native DLL loading, descriptor discovery, both plugin variants, instantiation, MIDI-triggered finite/nonzero audio, cleanup/unload, metadata parsing, system-only imports, native user-data folders, factory discovery, relocation, and preservation of user files. It is deliberately narrower than a production DAW certification suite.

Unverified areas include Linux audio/behavior comparison after these changes; state/project round trips; concurrent instances; complete Multi output routing; real-time stress; controller/automation coverage; GUI reopening; user bank editing through the GUI; a clean Windows machine without MSYS2 installed; other Windows hosts; and long-running stability. The original plugin identity has not been conclusively rehabilitated, and the cause of its rejection remains unresolved.

Current bundle:

```text
C:\msys64\home\beng\yoshimi\build-win\test-install-resources-20260930c\lib\lv2\yoshimi-windows-resources-20260930c.lv2
```

The implementation is a successful development port with a tested deployment approach, not yet a normal production distribution named simply Yoshimi.

# 2. Original source/build assumptions

The following issues are identifiable in the current diffs or the pre-existing initial-build commit. POSIX code was retained where possible.

| Source/function | Original assumption and Windows failure | Implemented solution | Linux implications |
| --- | --- | --- | --- |
| `src/CMakeLists.txt`, argp detection | Missing libc argp always requires libargp; native UCRT lacks it, although LV2 does not compile `CmdOptions.cpp` | Require fallback argp only outside `WIN32` | Original Linux requirement remains; this does not port standalone argument parsing |
| `src/CMakeLists.txt`, JACK discovery | JACK is a mandatory configuration dependency even for Windows LV2 | Skip required JACK package discovery on Windows | Linux JACK discovery unchanged; Windows LV2 uses the host, not JACK |
| `src/CMakeLists.txt`, version/install defines | Shell-style single quoting surrounds compiler definitions; Windows Ninja command handling differs from POSIX shells | Use escaped double quotes for the C++ string literals | Values remain strings on Linux; shell-independent quoting is intended |
| `FileMgrFuncs.h::copyFile`, option 2 | `utimensat` available to retain modification time | `_utime` with source access/modification times, returning failure code 3 on error | Existing POSIX branch retained verbatim; Windows has second-level `_utime` precision |
| `FileMgrFuncs.h::countDir` | `dirent::d_type` and `DT_DIR` exist | `stat` each candidate and use `S_ISDIR` | Linux retains its `d_type` branch; no blanket directory API replacement |
| `FileMgrFuncs.h::createDir` | POSIX two-argument `mkdir`, permissions and slash-based recursion | Windows C++17 `std::filesystem::create_directories` with `error_code` | Linux permissions/recursion retained; false still means success, including an existing directory |
| `FileMgrFuncs.h::userHome` | `getenv("HOME")` is non-null | Windows checks HOME, USERPROFILE, TEMP and TMP, validates existing directories, normalizes slashes, then falls back to `./` | Linux code untouched; final config/data paths no longer depend on HOME on Windows |
| `FormatFuncs.h::asString` | Existing integer overload set covers native unsigned sizes | Add `unsigned long long` overload using `ostringstream` | Shared additive change; accounts for Windows LLP64, where pointers/size_t are 64-bit but `long` is 32-bit |
| `UI/BankUI.fl` callbacks/menu data | Pointer-sized IDs can be cast through `long` | Cast through `std::intptr_t`, then explicitly to the bounded integer ID | Shared pointer-width correction; Linux IDs behave as before |
| `UI/MiscGui.cpp::custom_graphics`, `WidgetPDial.cpp::draw` | Legacy Cairo surface is Xlib/X server based | `_WIN32` selects `cairo-win32.h` and `cairo_win32_surface_create(fl_gc)` | Xlib branch retained; existing FLTK Cairo integration branch retained |
| LV2 entry points | Ordinary C linkage/visibility sufficiently exports symbols on all platforms | Mark `lv2_descriptor` and `lv2ui_descriptor` with `LV2_SYMBOL_EXPORT` | LV2-defined export mechanism works with Linux visibility and Windows export decoration |
| LV2 metadata | Binary is always `yoshimi_lv2.so`; custom target invokes shell `test -f` | Generate build-directory metadata, substitute `.dll` on Windows, use CMake dependency tracking | Linux generated contents keep `.so`; no Windows rewrite of source Turtle |
| DLL deployment | Dependencies placed beside a loaded plugin automatically resolve | Statically link non-system libraries and MinGW runtime into LV2 | Static selection is `MINGW`-specific; Linux link list remains in the other branch |
| `InstanceManager::Instance::shutDown` / InterChange destructor | A retained primary instance can join its resolver thread during module static destruction | Join explicitly during Windows LV2 shutdown, before unload/loader-lock static destruction | Early join is `_WIN32 && YOSHIMI_LV2_PLUGIN`; destructor still calls the shared helper |
| `localDir`, `configDir`, factory lookup | Linux user/share/config paths and checkout-relative CWD discovery | Windows AppData plus DLL-relative `resources` | POSIX paths remain under `#else`; no forced Windows scheme on Linux |
| `Config::findHtmlManual`, Open Manual | External `find` and `xdg-open` commands available | Direct bundled manual marker lookup and `ShellExecuteA` | Linux shell-based behavior retained |

The directory/timestamp fixes do not remove the broader use of MinGW's POSIX-compatible headers, `stat`, `opendir`, pthreads or semaphores. Winpthreads supplies the threading compatibility used by this toolchain. No comprehensive Windows signal abstraction was introduced. Standalone signal/session handling remains outside the validated scope; existing JACK-session code is conditional and JACK session support was absent in this build.

The resolver shutdown adjustment addresses a Windows unload hazard: joining/waiting for other threads from DLL static destruction can stall under the loader lock. `stopSortResultsThread()` posts its semaphore, joins, then clears the handle. It assumes the caller has already made `runSynth` false; shutdown does that first. It is not a general thread cancellation API. Automated cleanup/unload succeeded; every other background thread and exceptional shutdown path has not been exhaustively audited.

# 3. Build system changes

## 3.1 Toolchain and configuration

The actual cache selects:

```text
CMAKE_C_COMPILER       C:/msys64/ucrt64/bin/cc.exe
CMAKE_CXX_COMPILER     C:/msys64/ucrt64/bin/c++.exe
CMAKE_GENERATOR        Ninja
BuildWithFLTK          ON
LV2Plugin              ON
BuildForThisCPU        OFF
BuildForDebug          OFF
BuildForDiagnostic     OFF
YOSHIMI_LV2_WINDOWS_DIAGNOSTICS   OFF
YOSHIMI_LV2_WINDOWS_CLEAN_TEST    OFF
YOSHIMI_LV2_WINDOWS_RESOURCE_TEST ON
LV2_INSTALL_DIR        lib/lv2
WINDOWS_LV2_FLTK       C:/msys64/ucrt64/lib/libfltk.a
```

CMake sets the effective build type to Release in directory scope despite an empty `CMAKE_BUILD_TYPE` cache string. Flags include `-ffast-math -fomit-frame-pointer -DNDEBUG -O3`, C++17/GNU++17 and existing warning/security flags. Disabling CPU-native tuning makes the experiment less machine-specific, but does not establish a minimum supported CPU or cross-machine certification.

Tools inspected for this report: GCC **16.2.0**, CMake **4.4.3**, Ninja **1.13.2**. UCRT64 is essential: headers, import/static libraries, compiler and pkg-config should come from the same prefix. `/usr/bin` MSYS utilities can assist development, but their runtime is not a plugin dependency.

`WIN32` is the CMake platform condition; `_WIN32` is the C++ platform condition. The module also receives `YOSHIMI_LV2_PLUGIN=1`, `GUI_FLTK`, and `CAIRO_WIN32_STATIC_BUILD`. One opt-in test identity define is added for the selected experiment. Linux ALSA defines remain limited to Linux.

## 3.2 Target construction and metadata

[LV2 CMake](src/LV2_Plugin/CMakeLists.txt) constructs `yoshimi_lv2` as a `MODULE` from DSP, effects, parameters, synth, shared miscellaneous/interface code, the LV2 adapter, MusicClient/MusicIO, and generated FLTK UI sources. Standalone `main.cpp`, command-line parser and JACK/ALSA backend implementations are not this target's entry path. FLUID generates `.cpp`/`.h` from `.fl` files; edits to generated build-directory UI files are not source changes.

`PREFIX ""` prevents an unwanted `lib` prefix. Production fallback output is `yoshimi_lv2.dll`; test output names are selected with `OUTPUT_NAME`. `manifest.ttl` and `yoshimi.ttl` are copied at configure time into `build-win/LV2_Plugin`. On Windows the binary filename is changed to `.dll`, and `opts:supportedOptions` is corrected to `opts:supportedOption`. The source files still contain the original Linux metadata, including that spelling; this is a Windows-generated correction, not yet a cross-platform metadata cleanup.

Test options are mutually exclusive on Windows and default OFF. `PluginIdentity.h` separately selects the matching descriptor URI. Keeping these two identity definitions synchronized is currently a maintenance responsibility.

## 3.3 Static linkage

The `MINGW` branch obtains `pkg-config --static` dependency metadata for `cairo fontconfig fftw3f mxml zlib`. It explicitly locates `libfltk.a`, fails configuration if unavailable, adds static library search directories, and uses:

```text
-static -static-libgcc -static-libstdc++
libfltk.a
-Wl,--start-group <pkg-config static dependency libraries> iconv -Wl,--end-group
<static additional linker flags> comctl32 ws2_32 shell32
```

The group resolves cyclic static-library references; iconv is needed by static gettext/GLib dependencies. System import libraries still link Windows DLLs. `CAIRO_WIN32_STATIC_BUILD` prevents inappropriate Cairo Win32 DLL-import decoration for the static implementation. FLTK supplies its generated/native GUI dependencies; pkg-config supplies the Cairo/font stack's transitive libraries.

The link graph includes Cairo, pixman, fontconfig, FreeType, HarfBuzz, GLib, gettext/iconv, compression/XML/regex libraries, FFTW, MXML, zlib and toolchain runtime support. Source configuration still discovers curses/readline because the top-level project retains standalone checks; those packages are required to configure this checkout even though the Windows LV2 link list does not deliberately add standalone readline/curses libraries.

The static branch is **MinGW-specific**, not an implemented MSVC port. It also hardcodes pkg-config module `mxml`, whereas the top-level search supports `mxml4` or `mxml`; reproduction currently relies on the installed MXML 3 package.

## 3.4 Installation

LV2 target/metadata/data installs now have component `lv2`, permitting packaging without building/installing the standalone target. Windows adds banks, presets, examples, all documentation and COPYING under the bundle's `resources`. Linux still uses separate shared-data installs. Test bundle directory names derive from their DLL identity; all test switches OFF retains `yoshimi.lv2`.

The top-level source list includes `WindowsPaths.cpp` and links `shell32` for Windows so shared helpers are resolvable if standalone is built. That bookkeeping does not constitute a tested standalone port. Ordinary full-project build/install is not the validated workflow: build `--target yoshimi_lv2` and install `--component lv2`.

# 4. Runtime dependency problem

An initial native DLL linked and metadata discovery succeeded, yet REAPER rejected instantiation. A retained dynamic bundle contains the plugin plus **21 non-system runtime DLLs**. Its direct non-system imports are:

```text
libcairo-2.dll       libfftw3f-3.dll      libgcc_s_seh-1.dll
libwinpthread-1.dll libstdc++-6.dll      zlib1.dll
libfltk-1.4.dll
```

Cairo/FLTK/font dependencies introduce further transitive DLLs. The dependency closure was packaged, but merely putting those DLLs beside the plugin did not make default Windows loading reliable.

`build-win/verification/plain-loader.txt` and `verify-plain-loader.py` demonstrate the narrower, supported conclusion: with PATH reduced to Windows System32 and CWD outside the bundle, **plain `LoadLibraryW` failed with error 126 for the dynamic bundle**, and the seven tested DLL names were unresolved by normal search. With bundle-directory-aware loading, earlier isolated tests succeeded. Error 126 indicates a module could not be found; it does not, by itself, identify one missing transitive dependency. Here there are several unsatisfied normal-search dependencies, rather than proof of a unique failing DLL.

Loading a primary DLL by absolute path does not imply that ordinary dependency searching will search that DLL's directory. A host can explicitly request bundle-directory searching, but an LV2 distribution should not depend on REAPER doing so. A plugin's own DllMain or instantiate function cannot repair imports that prevented the loader from reaching it. A loader shim/delay-loading design would be a different architecture; it was not implemented. Copying dependencies into REAPER's directory or changing global PATH was rejected as a distribution strategy.

Static linking removed that prerequisite. The retained normal-static bundle passes plain loading even while the old runtime DLL names remain unresolved on PATH. The current resource DLL has **28 system/API-set import entries**, no non-system imported DLLs, and only one DLL in the bundle. Imports include the CRT API sets and:

```text
ADVAPI32.dll COMCTL32.dll comdlg32.dll DWrite.dll GDI32.dll KERNEL32.dll
MSIMG32.dll ole32.dll RPCRT4.dll SHELL32.dll USER32.dll USP10.dll
WINSPOOL.DRV WS2_32.dll
```

The remaining entries are `api-ms-win-crt-*` contracts. They are Windows/UCRT requirements, not files copied from MSYS2. This establishes independence from ordinary non-system import resolution; it does not prove that every supported Windows version supplies the required system facilities or that no library can ever dynamically load an optional module.

Automated tests used system-only PATH and foreign working directories. The resource probe used System32 as CWD. This is a valuable loader test on the development computer, not a clean-OS test: MSYS2 remains installed on the machine. REAPER's exact failing loader call/policy was not captured. The dynamic-load failure is proven locally; attributing every historical REAPER rejection to it would overstate the evidence, because the old static identity also failed in REAPER.

# 5. REAPER / LV2 identity investigation

## 5.1 Experiments and identities

Let `U = http://yoshimi.sourceforge.net/` for the table below. Multi appends `_multi`; each UI appends `#ExternalUI` to the listed base URI.

| Experiment | Bundle / DLL | Plugin base URI | Display name | Outcome |
| --- | --- | --- | --- | --- |
| Normal dynamic | `yoshimi.lv2` / `yoshimi_lv2.dll` | `U + lv2_plugin` | Yoshimi | Discovery, REAPER rejection; normal-search failure 126 reproduced locally |
| Normal static | `yoshimi.lv2` / `yoshimi_lv2.dll` | `U + lv2_plugin` | Yoshimi | Plain loader/probe succeeded; user still reported REAPER rejection and no TEMP log |
| Diagnostic `a` | `yoshimi-windows-diagnostic-20260930a.lv2` / `yoshimi_windows_diagnostic_20260930a.dll` | `U + lv2_plugin_windows_diagnostic_20260930a` | Yoshimi Windows Diagnostic | User confirmed REAPER GUI/MIDI/audio and diagnostic logs |
| Clean `b` | `yoshimi-windows-clean-20260930b.lv2` / `yoshimi_windows_clean_20260930b.dll` | `U + lv2_plugin_windows_clean_20260930b` | Yoshimi Windows Clean Test | User confirmed REAPER GUI/MIDI/audio, without diagnostic implementation |
| Resources `c` | `yoshimi-windows-resources-20260930c.lv2` / `yoshimi_windows_resources_20260930c.dll` | `U + lv2_plugin_windows_resources_20260930c` | Yoshimi Windows Resources Test | User confirmed REAPER GUI/MIDI/audio and playable populated factory banks |

The normal-static and diagnostic DLLs have identical system import lists and named export sets. Metadata normalization in `compare-bundles.py` establishes that the substantive Turtle differences are identity/name/binary strings and `supportedOption` spelling. Diagnostic `a` also adds the earlier attachment signal described below. Clean `b` normalizes to diagnostic metadata apart from identities, with diagnostics excluded. These are not one-variable experiments isolating folder versus DLL versus plugin URI.

## 5.2 Earliest diagnostics

[WindowsDiagnostics.cpp](src/LV2_Plugin/WindowsDiagnostics.cpp) is included only with `YOSHIMI_LV2_WINDOWS_DIAGNOSTICS`. A MinGW TLS callback placed in `.CRT$XLB` writes an attachment message before CRT entry/static constructors. DllMain then writes a second process-attach message. Both use Win32 file operations, with no thread creation or DLL loading; they also call OutputDebugStringA.

Signals appear at:

```text
%TEMP%\yoshimi-windows-diagnostic-20260930a.log
<full-plugin-DLL-path>.attach.log
```

The later formatted logger records PID/TID, descriptors, environment/module paths, host features, options, startup stages, UI operations, and cleanup. The sidecar path provides evidence of which binary actually attached. Diagnostic code must not become production loader-time behavior: even small loader callbacks deserve caution, and writing beside the installed DLL is unsuitable for read-only deployments.

The successful diagnostic host log identifies `C:\Program Files\REAPER (x64)\reaper.exe`, the expected installed diagnostic module, and successful plugin/UI initialization. It shows REAPER supplying URID map/unmap, options, boundedBlockLength, external UI host, worker scheduling and state-path features, among others. The UI instance-access pointer is populated for UI initialization.

## 5.3 Supported conclusions and uncertainty

Fresh-identity diagnostic, clean and resource experiments work. Clean success proves that diagnostic logging/TLS/DllMain instrumentation is not required for functional GUI/audio startup. Source/metadata equivalence supports keeping the functional LV2 architecture.

The observations do **not** prove that REAPER caching caused the original static identity to fail. Bundle directory, DLL filename, plugin URI, UI URI and metadata spelling changed together. A prior read-only examination recorded the diagnostic URI in `reaper-recentfx.ini`, but found no separately identified LV2 cache file in the standard Roaming REAPER directory. That file is host history evidence, not proof of a failure cache.

Absent earlier logs is consistent with rejection before our instrumented function, another installed binary/path, or failed logging, among other possibilities. No trace proves which happened for the old identity. Static linking fixes a demonstrated loader limitation; fresh identity and corrected metadata establish a successful combination. The minimum identity/metadata difference responsible for acceptance remains unknown.

# 6. LV2 implementation

The adapter remains in [YoshimiLV2Plugin.cpp](src/LV2_Plugin/YoshimiLV2Plugin.cpp). The entry points use `LV2_SYMBOL_EXPORT`; actual PE inspection and GetProcAddress-based probes find both `lv2_descriptor` and `lv2ui_descriptor`.

- Descriptor index 0: normal stereo instrument, **5 ports**: atom input, freewheel control, stereo outputs, atom notify output.
- Descriptor index 1: Multi instrument, **37 ports**: atom input, freewheel control, 34 audio outputs, atom notify output.
- Descriptor index 2: null. UI index 0 exposes the shared external UI; index 1 returns null.

Multi has the stereo master plus 16 stereo part output pairs. Automated probing connects all outputs but sums their energy; it does not establish every part's routing or independence. Manual REAPER Multi operation has not been confirmed.

Manifest plugin types remain `lv2:Plugin` and `lv2:InstrumentPlugin`. The UI remains the external-UI Widget type, with `instance-access` required and idle/show interfaces declared. No new Windows-native embedded LV2 UI type was introduced: FLTK creates Windows GUI windows using the existing external UI integration.

The plugin metadata requires `urid:map`, `buf-size:boundedBlockLength` and `opts:options`, with `maxBlockLength` required as an option. Min/nominal block sizes are supported options; `hardRTCapable` is optional metadata, not evidence of an audited real-time guarantee. State and programs interfaces are advertised. A worker namespace in Turtle and a host-provided worker schedule feature do not imply a Yoshimi LV2 worker interface: the adapter's extension data exposes state/programs, not a worker implementation.

Instantiation creates an InstanceManager-managed SynthEngine and MusicClient. `runtime().loadConfig()` occurs **before** the LV2 MusicIO object is constructed, so factory discovery cannot depend only on assigning `_bundlePath` inside that constructor. Windows module-relative lookup is consequently usable throughout early config setup.

The constructor consumes the host's URID map and options, maps MIDI/state/atom/time URIs, selects buffer capacity from min/max options and nominal size when present, then configures LV2 mode. `openAudio()` checks sample rate, capacity and essential URIDs before preparing buffers. The adapter preserves existing processing/time/MIDI logic. Windows instantiate wraps startup in C++ exception handlers and returns null on exception; this is not SEH/access-violation handling and does not prove complete unwind cleanup after partial initialization.

On successful startup, `isReady` is published with release semantics. UI initialization requires instance-access and waits with acquire semantics before attaching to the synth. Existing FLTK event/show/hide handling is preserved. Cleanup terminates the managed instance; Windows LV2 explicitly joins the resolver thread before possible FreeLibrary/static destruction. The primary managed synth is retained by the existing registry while non-primary defunct entries can be removed; probes exercise sequential instances, not simultaneous-instance safety.

State is still stored as the existing portable/string session blob. Importantly, **the state key remains** `http://yoshimi.sourceforge.net/lv2_plugin#state`, even for fresh test plugin URIs. Descriptor identities changed; state format/key did not. Actual DAW save/reload/state restoration remains untested.

Current bundle layout:

```text
yoshimi-windows-resources-20260930c.lv2/
  manifest.ttl
  yoshimi.ttl
  yoshimi_windows_resources_20260930c.dll
  resources/
    COPYING
    banks/<24 bank directories>/...
    presets/<19 component presets>
    examples/<patchsets, themes, scales, MIDI, text>
    doc/<full documentation tree>
```

# 7. GUI port

Two legacy drawing paths explicitly assumed an X server. `MiscGui.cpp::custom_graphics` and `WidgetPDial.cpp::draw` now include the platform's Cairo surface header and create a Win32 surface using FLTK's current graphics context `fl_gc` when `_WIN32` is defined. Their Xlib surface code is retained in the non-Windows branch. Cairo context transforms/drawing remain the existing implementation; the port does not replace dial/graph rendering with placeholder controls.

`ConfBuild.h` retains FLTK-version/Cairo capability selection. When FLTK offers `Fl::cairo_make_current`, that existing path is used; otherwise Windows now has a native legacy-surface alternative. The current capability warning still says Yoshimi is “forced to X11” when Cairo-integrated FLTK is unavailable. That label is inaccurate on the Windows branch: Win32 surfaces, not X11, are actually selected. No X server is a distribution requirement.

Bank UI menu/browser user-data casts formerly went through `long`; Windows x64 uses a 32-bit long and 64-bit pointer. Source `.fl` edits use intptr_t so generated callbacks preserve IDs correctly. These changes apply to shared UI source and are suitable for review as portable type fixes.

Manual REAPER testing establishes that the GUI opens, accepts the interaction needed to play factory patches and displays banks. It does not establish high-DPI correctness, all dialogs, file chooser Unicode behavior, resize edge cases, GUI close/reopen, focus/keyboard capture, themes, or manual-opening behavior. The probe enumerates the UI descriptor but does not instantiate an FLTK GUI.

# 8. Windows filesystem/resource architecture

## 8.1 Location mapping

Linux shared locations use GNUInstallDirs; examples below assume a conventional prefix and existing `/usr`/`/usr/local` discovery. Native Windows user paths no longer retain `~/.local/share` or `~/.config` conventions.

| Data | Existing Linux installation/runtime location | Windows location |
| --- | --- | --- |
| Factory bank originals | `<prefix>/share/yoshimi/banks`; Linux also considers Zyn banks and checkout banks | `<bundle>/resources/banks` |
| Component preset originals | `<prefix>/share/yoshimi/presets` | `<bundle>/resources/presets` |
| Examples/patchsets/vectors/scales/MIDI | `<prefix>/share/yoshimi/examples` | `<bundle>/resources/examples` |
| Factory theme examples | `share/yoshimi/examples/themes` | `<bundle>/resources/examples/themes` |
| Documentation/manual | `<prefix>/share/doc/yoshimi` | `<bundle>/resources/doc` |
| Editable bank copies/default bank root | `~/.local/share/yoshimi/found/yoshimi/banks`; configured additional roots | `%LOCALAPPDATA%/Yoshimi/found/yoshimi/banks`; user-configured additional roots |
| Editable component presets | `~/.local/share/yoshimi/presets`; configured extra paths | `%LOCALAPPDATA%/Yoshimi/presets`; configured extra paths |
| Preset directory catalog | `~/.local/share/yoshimi/presetDirs` | `%LOCALAPPDATA%/Yoshimi/presetDirs` |
| Writable themes/theme selection data | `~/.local/share/yoshimi/themes` | `%LOCALAPPDATA%/Yoshimi/themes` |
| Master config | `~/.config/yoshimi/yoshimi.config` | `%APPDATA%/Yoshimi/yoshimi.config` |
| LV2 instance defaults/default session | `~/.config/yoshimi/yoshimi-LV2.instance`, `yoshimi-LV2.state` | Same filenames under `%APPDATA%/Yoshimi` |
| Bank catalog | `~/.config/yoshimi/yoshimi.banks` | `%APPDATA%/Yoshimi/yoshimi.banks` |
| Favourites | `~/.config/yoshimi/yoshimi-favourites` | `%APPDATA%/Yoshimi/yoshimi-favourites` |
| GUI geometry | `~/.config/yoshimi/windows` | `%APPDATA%/Yoshimi/windows` |
| Recent-file history | `~/.local/share/yoshimi/recent`; old `.history` config migration | `%LOCALAPPDATA%/Yoshimi/recent`, through existing history code |
| Preset clipboard | `~/.local/share/yoshimi/clipboard` | `%LOCALAPPDATA%/Yoshimi/clipboard` |
| Plugin binary and Turtle | `<prefix>/<libdir>/lv2/yoshimi.lv2` | `%APPDATA%/LV2/<bundle>.lv2` in the tested user deployment |
| License | Source COPYING and documentation/license history | `<bundle>/resources/COPYING` and documentation |

Default filenames/locations are mapped, not a claim that every file above was created or exercised in the automated tests. User-chosen save/export files can remain outside these defaults. Configuration files are generated from defaults; this tree has no required installed configuration template pack.

## 8.2 Shared versus writable data

The bundle contains logically immutable factory originals and can be copied as a single install unit. Existing Bank APIs support save/delete/rename/edit operations, so registering the bundle factory path as an ordinary bank root would expose those originals to writes. Instead, Windows seeds writable bank copies into Local AppData and registers only that writable default root. Presets/themes similarly have separate originals and editable copies.

Local AppData holds machine-local content, caches/history and potentially substantial bank data. Roaming AppData holds comparatively small settings/catalog/geometry files, consistent with normal Windows profile layout. Absolute catalog paths remain an existing limitation for roaming across computers. No write-protect ACL is applied to bundle files: immutability is enforced by path selection and behavior, not an OS permission policy.

## 8.3 Path implementation and trace

[WindowsPaths.cpp](src/Misc/WindowsPaths.cpp) provides:

- `windowsUserDirectory(configuration)`: APPDATA or LOCALAPPDATA, falling back to `SHGetFolderPathA` with `CSIDL_APPDATA`/`CSIDL_LOCAL_APPDATA` and create semantics; appends `/Yoshimi` and normalizes slashes.
- `windowsFactoryDirectory()`: `GetModuleHandleExA` with FROM_ADDRESS and UNCHANGED_REFCOUNT on its own function address, then `GetModuleFileNameA`, takes the module's parent and appends `/resources`.
- `windowsOpenDocument()`: `ShellExecuteA("open", filename)`.

`FileMgrFuncs.h::localDir/configDir` creates those user directories with the established false-means-success helper. `Config::buildConfigLocation/initFromPersistentConfig/defaultPresets` establishes settings/preset paths. `Bank::establishBanks/transferDefaultDirs/addDefaultRootDirs` seeds and registers banks. `SynthEngine::installBanks/saveBanks` uses the user bank catalog. `UnifiedPresets`, MasterUI and MiscGui continue to use the centralized path helpers for clipboard, favourites and geometry.

On Linux, Bank's existing shared-bank list and transfer/update logic remain. Windows substitutes its resource source and avoids exposing it as an editable root; the Linux-specific Companion update step is excluded on Windows to avoid overwriting user banks.

`findExampleFile()` checks user data first, then bundled examples. Manual discovery directly resolves `resources/doc/yoshimi_user_guide/files/yoshimi_user_guide_version`; opening uses its adjacent guide index. This removes an MSYS `find`/`xdg-open` dependency. The manual path may be cached in configuration by existing code, so relocation/upgrade needs broader manual-path tests than the new-profile test performed here.

The helper currently uses **ANSI/narrow** Windows APIs and `std::string` file helpers. A 32768-byte module-path buffer avoids the immediate MAX_PATH buffer limit, but does not solve Unicode, extended-path prefixes or every CRT/filesystem limitation. Environment variables are accepted without a new absolute-path validation layer. Folder-API fallback with APPDATA/LOCALAPPDATA absent has not been separately verified for this final resource implementation.

# 9. Factory content

Read-only inventory of source data and the installed resource tree gives:

| Content | Verified count | Notes |
| --- | ---: | --- |
| Factory bank directories | 24 | Arpeggios through the two Will Godfrey collections |
| `.bankdir` markers | 24 | Dotfile markers; simple suffix-based counts can miss them |
| `.xiz` bank files | 902 | Bank instrument files only |
| `.xiy` bank files | 33 | Bank instrument files only |
| Total bank instrument files | 935 | Format variants can share a slot |
| Exposed factory instruments/programs | 917 | Actual enumeration and startup count, not inferred from file count |
| Component `.xpz` presets | 19 | 18,248 bytes in source |
| Theme `.clr` examples | 9 | Copied to user themes without replacing edits |
| Example files, including themes | 64 | 294,936 bytes |
| Documentation files, including its copy of examples | 264 | 3,667,045 bytes |
| Current complete bundle files | 1,310 | 29,600,576 bytes; one DLL plus metadata/resources |

Examples include 42 `.xmz` patchsets, one `.xiz` instrument, one `.xvy` vector, one each `.xsz`/`.scl`/`.kbm`, two MIDI files, six text files and nine themes. Documentation includes 34 HTML files, a CSS file, 74 PNG and two JPG images, 89 text files, an ODT guide, a PDF, and other reference/version/history files. Documentation counts already include examples; do not add them as unique content counts. The bundle intentionally mirrors the existing full-doc install's duplicate examples tree.

The current resource DLL is **20,694,008 bytes**, SHA-256:

```text
92cc9f9e45aec766953e396732298c501998989b58b36fd0743409b4bb7e7b9e
```

Initialization behavior:

1. Create Windows user config/data directories.
2. Ensure user presets/themes directories; copy factory files with `copyDir` option **0**, meaning copy only if absent. Option 1 means overwrite and was caught/corrected during testing.
3. Ensure writable bank root; list factory banks and copy each valid bank only when its destination directory is absent.
4. Load an existing bank catalog or establish the default writable root; scan and enumerate instruments.
5. Save catalogs through existing lifecycle code.

The bank copier does use option 1 inside a **new** destination bank directory. Existing bank directories are skipped completely, preserving edits and deletions there. Empty parent-root recovery works. A partially copied existing bank directory is not repaired; missing files in it are not replenished. Bank/preset copy failures are not comprehensively checked or surfaced, and simultaneous first-run copying is not synchronized. These are maintenance limitations, not claims of transactional content installation.

All factory/source resource hashes matched in the resource verifier, and startup did not mutate bundled originals. User-file sentinel edits and an additional user bank survived restart. This is byte-preservation verification, not a full GUI editor correctness test or a versioned factory-content upgrade scheme.

# 10. Source changes

## 10.1 Tracked files modified relative to current HEAD

| File | Important changes/purpose | Scope and Linux implication |
| --- | --- | --- |
| [src/CMakeLists.txt](src/CMakeLists.txt) | Add WindowsPaths to shared source list; Windows shell32 linkage | Shared source registration, Win32 link condition; early argp/JACK/quote changes are already in HEAD |
| [src/Interface/InterChange.cpp](src/Interface/InterChange.cpp) | Factor resolver join into stop helper; clear handle; Windows manual opening | Shared lifecycle factoring; Windows-specific document launch |
| [src/Interface/InterChange.h](src/Interface/InterChange.h) | Declare `stopSortResultsThread()` | Shared API addition; no Linux early-shutdown call introduced |
| [src/LV2_Plugin/CMakeLists.txt](src/LV2_Plugin/CMakeLists.txt) | Generated metadata, Windows test options/identities, export-friendly target naming, static MinGW link, helper source, LV2 install component, Windows data packaging | Cross-platform metadata generation/dependency tracking; platform-specific contents/link/data; Linux library branch retained |
| [src/LV2_Plugin/YoshimiLV2Plugin.cpp](src/LV2_Plugin/YoshimiLV2Plugin.cpp) | Identity header, diagnostic calls/features, Windows instantiate exception handling, LV2_SYMBOL_EXPORT | Test macros default no-op; production URI fallback; Windows catch; portable exports |
| [src/Misc/Bank.cpp](src/Misc/Bank.cpp) | Windows factory seeding, writable-root-only selection, Windows factory path list, omit Linux Companion update | Windows-only branches; existing Linux discovery/transfer/update remains |
| [src/Misc/Config.cpp](src/Misc/Config.cpp) | Diagnostics forwarding, Windows preset/theme seeding and defaults, DLL-relative manual | Windows behavior; logger calls compile out when diagnostics off; Linux defaults/manual retained |
| [src/Misc/FileMgrFuncs.h](src/Misc/FileMgrFuncs.h) | `_utime`, stat directories, filesystem mkdir, safe native home, AppData and example paths | Primarily `_WIN32` branches; new helper include harmless outside Windows |
| [src/Misc/FormatFuncs.h](src/Misc/FormatFuncs.h) | Unsigned-long-long formatter | Shared additive overload |
| [src/Misc/InstanceManager.cpp](src/Misc/InstanceManager.cpp) | Startup tracing; Windows LV2 shutdown joins resolver | Trace no-op unless opted in; early join Windows LV2 only |
| [src/UI/BankUI.fl](src/UI/BankUI.fl) | intptr_t casts and cstdint | Shared pointer-width fix; generated UI is not checked-in source here |
| [src/UI/MiscGui.cpp](src/UI/MiscGui.cpp) | Win32 Cairo header/surface in legacy drawing | Windows alternate; Xlib code retained |
| [src/UI/WidgetPDial.cpp](src/UI/WidgetPDial.cpp) | Win32 Cairo header/surface in dial drawing | Windows alternate; existing Linux drawing retained |

## 10.2 New source files, currently untracked

| File | Lines | Purpose |
| --- | ---: | --- |
| [src/LV2_Plugin/PluginIdentity.h](src/LV2_Plugin/PluginIdentity.h) | 15 | Opt-in diagnostic/clean/resource descriptor URIs, original URI fallback |
| [src/LV2_Plugin/WindowsDiagnostics.cpp](src/LV2_Plugin/WindowsDiagnostics.cpp) | 134 | Temporary Windows logger, TLS callback and DllMain instrumentation |
| [src/LV2_Plugin/WindowsDiagnostics.h](src/LV2_Plugin/WindowsDiagnostics.h) | 12 | Trace declarations or no-op macros |
| [src/Misc/WindowsPaths.cpp](src/Misc/WindowsPaths.cpp) | 55 | Windows AppData/module/document helpers; implementation inside `_WIN32` |
| [src/Misc/WindowsPaths.h](src/Misc/WindowsPaths.h) | 13 | Windows-only helper declarations |

These **229 new-source lines** are not included in ordinary `git diff --stat`. They must be carried with a reproducing patch even though this task did not stage them. `src/CMakeLists.txt.backup` is a retained backup, not part of the port source inventory. `src/version.txt` is generated by configure, not a new source implementation. `build-win` contains generated objects/binaries/bundles and local verification utilities; none is counted as a tracked source change. This report is intentionally untracked documentation.

# 11. Reproduction instructions

## 11.1 Source prerequisite

A clean upstream checkout alone does **not** contain these uncommitted fixes. Apply/transfer the reviewed Windows-port source modifications, including the five new source files and the initial CMake changes already present in this local HEAD. Building only upstream HEAD or only a diff omitting untracked files will not reproduce the result.

Use a fresh build directory for toolchain/identity experiments. The following recipe is derived from the inspected current CMake and installed packages; it has **not** been rerun from a newly provisioned clean checkout/machine during this documentation task. Reproduction therefore remains PARTIAL pending that exercise.

## 11.2 MSYS2 packages

Open the **MSYS2 UCRT64 shell**. Verify `MSYSTEM=UCRT64` and compiler/pkg-config paths under `/ucrt64/bin`, not MINGW64/CLANG64 or MSYS `/usr/bin` compilers. Update MSYS2 following its normal update/restart procedure, then install the relevant package family:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-fftw \
  mingw-w64-ucrt-x86_64-mxml \
  mingw-w64-ucrt-x86_64-fontconfig \
  mingw-w64-ucrt-x86_64-cairo \
  mingw-w64-ucrt-x86_64-fltk \
  mingw-w64-ucrt-x86_64-lv2 \
  mingw-w64-ucrt-x86_64-zlib \
  mingw-w64-ucrt-x86_64-readline \
  mingw-w64-ucrt-x86_64-ncurses \
  mingw-w64-ucrt-x86_64-libiconv
```

Pacman supplies transitive dependencies. Static archives must exist, especially `libfltk.a`, and the pkg-config static closure must resolve; do not silently substitute DLL import libraries and assume bundle-local dependency loading. No JACK or argp package is needed to configure/build this LV2 experiment. Git is useful for obtaining/reviewing source; Python/rdflib and binutils are verification tools rather than end-user requirements.

Observed package versions on this machine:

| Package suffix, all `mingw-w64-ucrt-x86_64-` | Version |
| --- | --- |
| gcc | 16.2.0-4 |
| cmake / ninja / pkgconf | 4.4.3-3 / 1.13.2-1 / 1~3.0.7-1 |
| fftw / mxml | 3.3.11-1 / 3.3.1-4 |
| fontconfig / cairo / fltk / lv2 | 2.18.3-1 / 1.18.6-2 / 1.4.5-1 / 1.18.10-1 |
| zlib / readline / ncurses / libiconv | 1.3.2-2 / 8.3.003-1 / 6.6-4 / 1.19-1 |
| winpthreads | 14.0.0.r426.g4564ee4b5-1 |

The installed stack also includes pixman 0.46.4-3, FreeType 2.14.3-1, HarfBuzz 14.5.0-1, GLib 2.90.0-1, libpng 1.6.59-1, brotli 1.2.0-1, bzip2 1.0.8-4, pcre2 10.49-1 and graphite2 1.3.15-1. This is an observed development environment, not an immutable dependency lock or guarantee for future package updates.

## 11.3 Configure, build and package a fresh resource identity

From the patched checkout root in UCRT64:

```sh
cmake -S src -B build-win-repro -G Ninja \
  -DCMAKE_C_COMPILER=/ucrt64/bin/gcc.exe \
  -DCMAKE_CXX_COMPILER=/ucrt64/bin/g++.exe \
  -DCMAKE_INSTALL_PREFIX="$(cygpath -m "$PWD/stage-win-repro")" \
  -DLV2_INSTALL_DIR=lib/lv2 \
  -DLV2Plugin=ON -DBuildWithFLTK=ON \
  -DBuildForThisCPU=OFF -DBuildForDebug=OFF -DBuildForDiagnostic=OFF \
  -DYOSHIMI_LV2_WINDOWS_DIAGNOSTICS=OFF \
  -DYOSHIMI_LV2_WINDOWS_CLEAN_TEST=OFF \
  -DYOSHIMI_LV2_WINDOWS_RESOURCE_TEST=ON

cmake --build build-win-repro --target yoshimi_lv2 -j 4
cmake --install build-win-repro --component lv2 \
  --prefix "$(cygpath -m "$PWD/stage-win-repro")"
```

Output module: `build-win-repro/LV2_Plugin/yoshimi_windows_resources_20260930c.dll`. Complete installed bundle: `stage-win-repro/lib/lv2/yoshimi-windows-resources-20260930c.lv2`. Use the component installation, not the older hardcoded `build-win/package-lv2.py`, which targets the clean `b` bundle and must not overwrite a control.

The actual retained build used the existing `build-win` directory, then:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build build-win --target yoshimi_lv2 -j 4
cmake --install build-win --component lv2 --prefix C:/msys64/home/beng/yoshimi/build-win/test-install-resources-20260930c
```

For a production-candidate identity, all three Windows test switches OFF would select `Yoshimi`, original descriptor/UI URIs, `yoshimi_lv2.dll` and `yoshimi.lv2`, while retaining static linking and Windows resources. That **combination with the latest resources has not been validated in REAPER**; do not present it as an already certified release.

## 11.4 Installation and host test

Manually copy the complete `.lv2` folder into `%APPDATA%\LV2`, retaining its name and resources tree. For the tested account:

```text
C:\Users\beng\AppData\Roaming\LV2\yoshimi-windows-resources-20260930c.lv2
```

Do not copy dependency DLLs into REAPER or change global PATH. Keep diagnostic/clean controls during comparison. Fully restart REAPER after replacing a loaded DLL; request a plugin rescan if needed using REAPER's FX/plugin preferences (exact UI varies by host version). Search for `Yoshimi Windows Resources Test`, insert it as a virtual instrument on a new track, show the GUI, send MIDI and select/play a factory bank patch. If discovery/load behavior differs, first establish which bundle/identity was scanned; do not edit cache files blindly. These are instructions for the developer/user, not actions performed while writing this report.

## 11.5 Recreating probes

Local utilities reside under `build-win/verification`; their paths/identities are hardcoded and must be adapted to a fresh build and **fresh isolated profile**. `verify-resources.py` deliberately refuses an already existing relocation directory. Older diagnostic verifiers can append logs to their control bundle sidecar: do not rerun them against a preserved control without accounting for this.

The C probes can be rebuilt in UCRT64, for example:

```sh
gcc -O2 -static build-win/verification/load-probe.c \
  -o build-win/verification/instantiate-probe.exe
gcc -O2 -static -I src/LV2_Plugin build-win/verification/resource-probe.c \
  -o build-win/verification/resource-probe.exe
```

Metadata verifiers require Python and rdflib, which were installed in a local verification venv. They are not CTest-integrated or a reusable upstream test suite yet. Set APPDATA/LOCALAPPDATA, USERPROFILE, TEMP/TMP to workspace test locations before loading a plugin; otherwise its normal startup can create real user files.

# 12. Verification performed

No runtime test was rerun for this report. The PE inspection and file inventories were rechecked read-only. Retained successful artifacts and the user's new real-world confirmation support this matrix.

| Test | Status | Evidence and precise coverage |
| --- | --- | --- |
| Native x86-64 PE | PASS | objdump format/PE32+ and earlier `pei-x86-64` audits; current Windows DLL |
| Imports/dependencies | PASS | `resource-dependencies.json`; read-only import inspection; system/API-set entries only |
| LV2 plugin/UI exports | PASS | Named table inspection plus GetProcAddress in probes; 199 total named exports, not only two |
| Metadata parsing/references | PASS | rdflib verifiers; both plugin URIs/names and DLL reference; earlier clean/control port-count/index/symbol checks |
| Plain LoadLibraryW | PASS | Static/clean/resources; system-only PATH, foreign CWD |
| Dynamic bundle plain loader | PASS (negative test) | Expected failure 126 with unresolved seven direct library names |
| Plugin descriptor enumeration | PASS | Indices 0/1 valid; 2 null |
| UI descriptor enumeration | PASS | Index 0 valid with callbacks; 1 null; automated GUI instantiation not included |
| Instantiation | PASS | Both normal and Multi descriptors sequentially |
| MIDI to finite/nonzero audio | PASS | Note-on, 64 processing blocks, energy/finite checks; freewheel flag set |
| Factory enumeration | PASS | 917 programs and 24 banks per fresh profile; extra user bank gives 918/25 |
| Factory program selection/audio, automated | PARTIAL | Program interface selects Arpeggio1 then checks audio; no independent assertion of internal active patch/state |
| Factory selection/audio in REAPER | PASS | User played factory patches including arpeggios |
| Cleanup/unload | PASS | Deactivate, cleanup and FreeLibrary complete in sequential probes |
| Path containing spaces | PASS | Resource bundle relocated under `relocated resources final`; earlier native home also contained spaces |
| New/empty user data | PASS | Isolated native first-run directories and pre-existing empty bank/preset parent directories |
| Preservation of user edits | PASS | Byte-hash preservation of sentinel-edited instrument, preset, theme and added user bank across restart |
| Immutable factory/control preservation | PASS | Resource verifier hashes sources/bundle and both saved controls before/after |
| APPDATA/LOCALAPPDATA absent fallback | NOT TESTED | Implemented shell-folder fallback inspected; older TEMP/HOME test covers different path design |
| REAPER discovery/instantiation | PASS | User confirmation for diagnostic, clean and resources; diagnostic host log corroborates startup |
| REAPER GUI | PASS | User confirmation; not exhaustive GUI regression |
| REAPER MIDI/audio | PASS | Virtual MIDI keyboard, audible synthesis |
| REAPER factory banks | PASS | User confirms populated browser and playable patches |
| Sample-rate/block-size combinations | PARTIAL | Earlier clean/diagnostic probes: 48k/512, 44.1k/128, 48k/256; not full resource/REAPER matrix |
| Concurrent instances | NOT TESTED | Sequential normal/Multi tests do not imply concurrency coverage |
| Multi routing in REAPER | NOT TESTED | Probe connects/sums all outputs; no per-part routing assertions |
| State/project save and reload | NOT TESTED | State implementation present but no retained round-trip test |
| Manual opening/themes/GUI edit dialogs | NOT TESTED | Resources copied and manual marker found; interaction not exercised |
| Windows machine without MSYS2 installed | NOT TESTED | Only PATH isolation on development machine |
| Linux rebuild/audio regression | NOT TESTED | Source preservation inspected; Linux CI not run |

Important limits discovered while reviewing tests:

- Probes use `freewheel=1`, sleep briefly between callbacks and run a short MIDI note sequence. They verify synthesis/lifecycle, not sustained real-time scheduling, underrun behavior or host automation.
- Current resource logs report 48 kHz and an internal period size of 256; the probe source defaults its host-call block to 512 unless its environment overrides it. Internal engine period and host callback length are not necessarily identical. The retained resource script does not explicitly enumerate rate/block combinations, so the earlier clean matrix must not be attributed wholesale to the final resource binary.
- `resource-tests.txt` contains `Main Unrecognised Main` lines as well as successful program enumeration/audio. The test does not explain them or independently prove the selected program changed internal sound state. Manual REAPER factory-patch playback now supplies the stronger real-host evidence.
- `compare-bundles.py` originally applied its export regex to all objdump output and includes spurious repeated `Export` tokens from other tables. This report rechecked only the named-export table: normal-static, diagnostic, clean and resource DLLs genuinely have the same **199** export names, including the two LV2 entry points. The old serialized export lists should not be treated as a precise export inventory.

Evidence files: `plain-loader.txt`, `verification.txt`, `instantiation.txt`, `clean-tests.txt`, `resource-tests.txt`, `resource-dependencies.json`, `identity-attach-only.log`, `bundle-comparison.json/.txt`; implementations: `load-probe.c`, `resource-probe.c`, `verify-plain-loader.py`, `verify-clean.py`, `verify-resources.py`. The successful diagnostic REAPER log is at `%TEMP%/yoshimi-windows-diagnostic-20260930a.log`. User confirmations for clean/resources are manual reports, not captured automated REAPER sessions.

# 13. Outstanding testing

The following are proposed release gates; they are not additional changes made or tests run by this report.

1. **Production identity**: install the latest implementation under normal Yoshimi identity on a clean host/profile and the development machine; verify actual loaded path and metadata. Isolate URI/path/metadata differences if rejection persists.
2. **State/project persistence**: save/reopen projects and presets, restore multiple parts/effects/tunings, remove/reinsert the plugin, restart the host and compare sound/state. Check project portability and moved bundle/user-data paths.
3. **Lifecycle/concurrency**: multiple simultaneous instances, concurrent GUI operations, insert/remove cycles, UI close/reopen, unload/reload and shutdown with background PAD work. Verify retained-primary management and join ordering under stress.
4. **Audio/MIDI coverage**: rate/block matrix on the final bundle, varying callback lengths within capacity, freewheel/offline versus real-time, transport/tempo, sustain/pitch bend/controllers, program/bank changes, host automation and MIDI learn. Compare Linux Yoshimi output with documented floating-point tolerances rather than assuming bit identity across compilers.
5. **Multi**: host routing of each stereo part plus master, channel assignment, state persistence and simultaneous part synthesis; summed energy is insufficient.
6. **Writable data**: real GUI save/rename/delete/create bank and preset operations, favourites/history/clipboard/geometry persistence, theme changes, documentation opening, permission failures, interrupted first-run copies and concurrent first-run initialization.
7. **Deployment**: fresh Windows x64 without MSYS2, no development PATH, read-only bundle location, non-ASCII/space/long user and install paths, roaming profiles and supported minimum Windows version. Confirm built-in font/UI behavior and no optional module/config dependency on developer-installed tools.
8. **Regression/stability**: Linux build/install/LV2 smoke tests, representative factory patches using ADD/SUB/PAD engines, long-running host playback, memory/resource growth and stress under host rendering.

Testing another Windows LV2 host is a useful compatibility extension after those gates; it is not evidence required to confirm the already observed REAPER milestone. Additional visual/DPI/accessibility and latency/performance measurements are recommended before broader distribution.

# 14. Upstreaming/maintenance considerations

Platform-dependent filesystem/Cairo behavior is mostly isolated under `_WIN32`/`WIN32`; Windows paths are in a small dedicated helper implementation. Linux bank-transfer/default preset/manual branches remain. Shared changes are principally pointer-sized casts, an additional formatter overload, resolver join factoring, LV2 export decoration and CMake-generated metadata/component handling. These still need Linux CI, because source similarity is not runtime regression evidence.

Before upstream submission:

- Separate the genuine portability/runtime/resource patch from the experimental identities and tracing. Date-stamped names/options and `PluginIdentity.h` test branches should not be permanent production branding. Remove TLS/DllMain sidecar logging and the opt-in logger or move them to developer-only tooling outside the submitted production path.
- Review Windows exception handling for cleanup after partially created instances; current catches prevent ordinary C++ exceptions escaping instantiate but do not audit unwind correctness or Windows structured exceptions. UI instantiate has no added analogous catch boundary.
- Restrict exports to intended LV2 API where compatible with static libraries; current hidden visibility does not prevent already export-decorated Cairo/pixman symbols from producing 199 named exports.
- Prefer target-scoped CMake definitions/link properties and a single source of identity/metadata substitutions. Review top-level `.version` generation, inherited standalone dependencies, MXML module selection and Windows-versus-MinGW condition consistency. Existing minimum CMake is 3.12, so any modernization should respect or explicitly update it.
- Assess Unicode/long-path support coherently through Windows and CRT file helpers; converting only the module lookup to wide strings will not fix narrow downstream file access.
- Improve factory seeding error handling, partial-install recovery/concurrent copying and versioned updates without overwriting user edits. Establish migration policy for earlier `.local/.config` experiments if needed. Define sharing between normal/Multi and multiple installed test identities, all of which currently use the same Windows user directories.
- Correct platform/audio status messages and investigate the `Main Unrecognised Main` probe output. Do not confuse misleading existing labels with functional X11/JACK/ALSA requirements.
- Review the Windows-only `supportedOption` correction as a separate metadata fix appropriate for Linux too; duplicated option declarations and hardRTCapable advertising also deserve review rather than silently changing unrelated Linux metadata here.
- Review third-party static licensing, notices and corresponding source/build reproducibility alongside Yoshimi's GPL distribution obligations. The experiment copies COPYING and documentation but is not a completed release compliance package.

Suggested CI, without repository changes in this task:

- Linux: configure/build LV2 with FLTK, metadata checks, install layout, lifecycle/audio smoke test and representative regression sounds.
- Windows UCRT64: pinned/recorded packages, target-only build, component install, PE import allowlist, descriptor/metadata alignment, plain loader and headless LV2 MIDI/audio/cleanup tests in a fresh environment.
- Resource tests: new user profile, read-only factory originals, missing/partial copies, preservation of edits, spaces/Unicode, and state round trips.
- Separate periodic/manual REAPER checks for GUI/state/Multi; a headless harness must not be represented as a DAW compatibility test.

Generated build directories, binaries, test profiles, logs, PDFs, source backup and generated version file should not become part of the portability source patch. Verification scripts can be turned into clean repository tests after removing hardcoded local paths, mutation of controls and stale identities.

# 15. Known limitations and unresolved questions

- **Original identity failure is unresolved.** Neither a REAPER cache explanation nor a metadata-only explanation was isolated. Fresh identities and corrected metadata work; normal-static loader success did not imply REAPER instantiation success.
- **Production identity not validated with final resources.** All flags OFF provides it in code, but historical failure makes a new actual host test necessary.
- **Linux regression evidence incomplete.** POSIX branches were intentionally retained; no Linux rebuild/output comparison is in these artifacts.
- **Narrow paths remain.** ANSI Windows calls and narrow CRT/stat/stream interfaces limit non-ASCII/extended-path confidence. Unicode/long/UNC support is not certified merely because filesystem mkdir handles drive and UNC roots.
- **Seeding is conservative, not transactional.** An existing partial bank is skipped, copy results are not comprehensively handled, factory-bank updates are not version-managed, and concurrent initialization can race. A missing resource source can leave a created empty root rather than clear first-run failure.
- **Shared user state across identities.** All test and production variants use `%APPDATA%/Yoshimi` and `%LOCALAPPDATA%/Yoshimi`; distinct LV2 URIs isolate plugin discovery, not settings/banks. Host project states are distinct while the state-key URI is unchanged.
- **State/UI/lifecycle breadth incomplete.** Ordinary startup works, but DAW state restoration, multiple active engines and all GUI lifecycle/dialog paths have not been exercised.
- **System baseline not declared.** System-only imports include UCRT and DirectWrite. No oldest Windows release, installer runtime prerequisites or CPU baseline was tested.
- **Not an MSVC/ARM64/32-bit/standalone port.** The static path and practical validation target UCRT64 MinGW x86-64 LV2 only.
- **Warnings/diagnostic messages persist.** Retained build output includes existing SVFilter initialization warnings, the `fl_disable_wayland` extern warning and FLTK deprecated API warnings. They were not fixed as part of this focused port. Existing host/log labels can be misleading.
- **Static footprint/export surface.** The DLL is approximately 20.7 MB unstripped and exports static-library API names. A smaller release package and controlled export surface need separate work; stripping this file was not performed.

# 16. Proposed path to production

The minimum remaining engineering path, proposed only:

1. Freeze the working resource bundle/control hashes and source patch; make the source changes reproducible including all untracked helper files.
2. Produce an otherwise equivalent all-test-switches-OFF candidate: normal `Yoshimi`, original plugin/UI URI, `yoshimi_lv2.dll`, `yoshimi.lv2`, static dependencies and resources. Test the exact installed candidate in REAPER, first on a clean profile/machine and then the existing environment. Record path/metadata evidence if the old rejection returns; isolate changes individually rather than labeling it a known cache bug.
3. Remove experimental branding and loader-time logging from the production patch; retain a separate diagnostic tool/build process. Rebuild/retest after that cleanup.
4. Complete state, simultaneous-instance, GUI lifecycle, real-time/Multi, Windows deployment and Linux regression gates. Fix only confirmed production blockers, including resource/error-path issues as required by the selected installation policy.
5. Define supported Windows baseline and content update policy; prepare license/source notices, reproducible dependency record, controlled exports and release packaging. Optional symbol stripping can produce a separate distribution artifact with retained matching debugging symbols.
6. Submit a minimal reviewed portability/resource patch and documented test evidence to maintainers through a separately authorized process. No submission, commit or push is part of this task.

The current architecture does not require a redesigned synth engine or embedded Windows UI to reach production. The central remaining uncertainty is acceptance of the normal identity, followed by the release-level coverage listed above.

# 17. Appendix: chronological porting/debugging history

1. **Initial configuration fixes**: argp fallback and mandatory JACK were bypassed for Windows; string-definition quoting was made Windows compatible. These are already in local HEAD, preceding the current uncommitted patch.
2. **Filesystem compile failures**: missing utimensat/d_type/DT_DIR and incompatible mkdir were handled with conditional Windows implementations, preserving POSIX branches.
3. **Further native compile/UI issues**: unsigned integer formatting, pointer-through-long casts and Xlib Cairo assumptions were corrected. The LV2 target linked and explicitly exported its entry points.
4. **First bundle/discovery**: Windows-generated metadata referenced `.dll`; a dependency closure was packaged beside the module. REAPER discovered Yoshimi but reported inability to load it.
5. **Harness/runtime investigation**: native home/environment setup, descriptor/feature/instantiation tracing and lifecycle checks were added. Isolated host-like tests passed. Cleanup was changed to join the resolver before loader-time static destruction.
6. **Normal search experiment**: the dynamic bundle failed plain LoadLibraryW with 126 when MSYS2 was absent from PATH; bundle-aware loading passed. This distinguished dependency searching from LV2 instantiate behavior.
7. **Static linkage**: third-party/toolchain runtime dependencies were embedded. Normal static bundle passed plain loading, instantiation and MIDI/audio, but the user still saw the normal identity rejected by REAPER with no TEMP log.
8. **Earlier signal/fresh identity**: TLS-before-CRT attachment and DllMain/sidecar signals were added; bundle/DLL/plugin/UI names were made unmistakable; supportedOption spelling corrected. User reported successful diagnostic REAPER GUI/MIDI/audio and both logs.
9. **Clean control experiment**: another fresh identity excluded diagnostics while keeping functional implementation/static linkage/corrected metadata. Automated rate/block checks and manual REAPER GUI/audio succeeded. Diagnostic side effects were thus unnecessary; minimum identity versus spelling difference remained unresolved.
10. **Resources**: traced Linux install/discovery, packaged banks/presets/examples/docs, added AppData user paths and module-relative lookup. The user-edit preservation test caught the preset/theme overwrite-mode mistake; it was corrected and rebuilt before the final bundle. Fresh/empty/relocated tests passed with 24/917 enumeration.
11. **Real-host content confirmation**: user now confirms the Resources Test loads in 64-bit REAPER, shows banks and plays factory patches including arpeggios. This closes the basic native LV2 plus factory-content milestone.
12. **This report**: read-only inspection of sources/configuration/artifacts and PE/resource inventory; documentation written only. No new runtime behavior or source fix was introduced.

Lessons: compilation is not deployment; bundle-adjacent DLLs are not proof of default dependency resolution; plain loader success is not DAW acceptance; no log is ambiguous without an early/path-specific signal; multi-variable identity experiments support a working combination, not a uniquely identified cause; factory bank editing requires writable copies; preservation assertions are more reliable than assuming a copy-option meaning.

# 18. Appendix: exact diff summary

Baseline: current `HEAD = daaacf33c694c6018d970e427dc51069e7150ba9`. Relative to its parent, that existing initial-build commit changes `src/CMakeLists.txt` by **8 additions / 4 deletions**. Those lines are not in the current unstaged diff. No new commit was made while porting/reporting in the tasks described here.

Current unstaged tracked source summary is **13 files, 373 additions / 35 deletions**. New source files add **229 lines** separately, for an approximate uncommitted source total of **602 added / 35 removed** when those untracked implementations are included. Backup/generated/report/test artifacts are excluded from that total. This is a textual accounting, not a diff against pristine upstream.

The exact short status/stat/numstat captured after writing this report follow. `build-win/` is shown because git reports it as untracked, not because its generated contents are source changes. The report itself is also intentionally untracked. The staged diff is empty.

## git status --short

```text
 M src/CMakeLists.txt
 M src/Interface/InterChange.cpp
 M src/Interface/InterChange.h
 M src/LV2_Plugin/CMakeLists.txt
 M src/LV2_Plugin/YoshimiLV2Plugin.cpp
 M src/Misc/Bank.cpp
 M src/Misc/Config.cpp
 M src/Misc/FileMgrFuncs.h
 M src/Misc/FormatFuncs.h
 M src/Misc/InstanceManager.cpp
 M src/UI/BankUI.fl
 M src/UI/MiscGui.cpp
 M src/UI/WidgetPDial.cpp
?? YOSHIMI_WINDOWS_LV2_PORT_REPORT.md
?? build-win/
?? src/CMakeLists.txt.backup
?? src/LV2_Plugin/PluginIdentity.h
?? src/LV2_Plugin/WindowsDiagnostics.cpp
?? src/LV2_Plugin/WindowsDiagnostics.h
?? src/Misc/WindowsPaths.cpp
?? src/Misc/WindowsPaths.h
?? src/version.txt
```

## git diff --stat

```text
 src/CMakeLists.txt                  |   5 +-
 src/Interface/InterChange.cpp       |  12 +++-
 src/Interface/InterChange.h         |   1 +
 src/LV2_Plugin/CMakeLists.txt       | 134 +++++++++++++++++++++++++++++++-----
 src/LV2_Plugin/YoshimiLV2Plugin.cpp |  89 +++++++++++++++++++++---
 src/Misc/Bank.cpp                   |  32 +++++++++
 src/Misc/Config.cpp                 |  23 ++++++-
 src/Misc/FileMgrFuncs.h             |  63 +++++++++++++++++
 src/Misc/FormatFuncs.h              |   7 ++
 src/Misc/InstanceManager.cpp        |  13 ++++
 src/UI/BankUI.fl                    |   9 ++-
 src/UI/MiscGui.cpp                  |  10 ++-
 src/UI/WidgetPDial.cpp              |  10 ++-
 13 files changed, 373 insertions(+), 35 deletions(-)
```

## git diff --numstat

```text
4	1	src/CMakeLists.txt
11	1	src/Interface/InterChange.cpp
1	0	src/Interface/InterChange.h
115	19	src/LV2_Plugin/CMakeLists.txt
81	8	src/LV2_Plugin/YoshimiLV2Plugin.cpp
32	0	src/Misc/Bank.cpp
22	1	src/Misc/Config.cpp
63	0	src/Misc/FileMgrFuncs.h
7	0	src/Misc/FormatFuncs.h
13	0	src/Misc/InstanceManager.cpp
6	3	src/UI/BankUI.fl
9	1	src/UI/MiscGui.cpp
9	1	src/UI/WidgetPDial.cpp
```

## New implementation files (untracked)

```text
src/LV2_Plugin/PluginIdentity.h              15 lines
src/LV2_Plugin/WindowsDiagnostics.cpp      134 lines
src/LV2_Plugin/WindowsDiagnostics.h         12 lines
src/Misc/WindowsPaths.cpp                  55 lines
src/Misc/WindowsPaths.h                    13 lines
Total                                    229 lines
```

`git diff --cached --stat`: empty. No files were staged.

