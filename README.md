# GComp

GComp decompiles a 1C:Enterprise 7.7 configuration (`1Cv7.MD`) or external
report (`.ert`) into a tree of text files and compiles it back, so that
configurations can be kept in a version control system.

This repository is based on GComp 2.2.15 by the Alterplast team
(https://1c.alterplast.ru/gcomp/), GNU GPL (see `Documentation/txt`).

## History

- `Import GComp 2.2.15 sources` — upstream `gcomp_src_2.2.15.zip`
  (SHA256 `7d3f727f69089472640bbcf656decf3fe18500841d05a9176cd39919ebe6bbf8`),
  byte-identical except removed compiled Delphi resources, a 1C spreadsheet
  sample and CVS metadata. Sources keep upstream CP1251/CP866 and CRLF.
- Later commits make it buildable without Visual C++ 6 and Microsoft MFC
  while keeping it compilable by MSVC.

## Building a Windows PE with winegcc

Requires Wine development tools (winegcc/wineg++, wrc, i386-windows import
libraries). MFC classes come from the bundled `mfc-compat/` (free
replacement for the non-GUI core of MFC 4.2, LGPL-2.1, see its README).
zlib 1.3.1 is downloaded and checked by SHA256.

    ./build-winegcc.sh

Result: `build-pe/GComp.dll`, `build-pe/gcomp.exe`; run on Windows or with
Wine:

    wine build-pe/gcomp.exe -d -F 1Cv7.MD -D src    # decompile
    wine build-pe/gcomp.exe -c -F 1Cv7.MD -D src    # compile

`wine-pe-atexit.c` works around a Wine bug: the static `atexit()` that
winegcc links into PE modules uses an uninitialised onexit table, and a C++
DLL with global destructors crashes on Windows with STATUS_HEAP_CORRUPTION
(https://bugs.etersoft.ru/show_bug.cgi?id=19907).

## Building a Windows PE with MinGW-w64

Requires the i686-w64-mingw32 cross compiler (Fedora: `mingw32-gcc-c++`).
The modules are linked with `-static` and need only system DLLs.

    ./build-mingw.sh

Result: `build-mingw/GComp.dll`, `build-mingw/gcomp.exe`.

Both builds were checked on Windows 10 against the original GComp 2.2.15
binary: the decompiled tree is byte-identical and the compiled MD has the
same streams. Under Wine the winegcc build names Cyrillic files with `_`
because Wine's `__iscsym` is ASCII-only
(https://bugs.etersoft.ru/show_bug.cgi?id=19905); the MinGW headers
implement `__iscsym` as a macro and are not affected.

See `PORTING.md` for dependencies and the plan for a native Linux build.
The Delphi GUIs (`vgcomp/`, `gcomp_vis/`) are not built.
