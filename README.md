# GComp

GComp decompiles a 1C:Enterprise 7.7 configuration (`1Cv7.MD`) or external
report (`.ert`) into a tree of text files and compiles it back, so that
configurations can be kept in a version control system.

This repository is based on GComp 2.2.15 by the Alterplast team
(https://1c.alterplast.ru/gcomp/), GNU GPL (see `Documentation/txt`).

## License

GComp is free software under the GNU General Public License. The original
`Documentation/txt/license.txt` does not state a version, and the English
text it refers to (`gnugpl.eng.txt`) was not in the 2.2.15 source archive;
the bundled Russian translation is of GPL version 2. Under section 9 of
GPL v2 a program that specifies no version may be used under any version,
so GComp with the changes in this repository is distributed under
GPL-2.0-or-later (`COPYING`, text from gnu.org). New files carry
`SPDX-License-Identifier: GPL-2.0-or-later`.

Bundled components: `mfc-compat/` — LGPL-2.1-or-later; zlib (downloaded
at build time) — zlib license. Both are compatible with the GPL.

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

## Round trip

The goal is that decompiling and compiling back loses nothing; it is not a
byte copy of the original MD. Checked on 431 1C 7.7 configurations and
external reports (and a production configuration): 430 decompile and
compile, and comparing the result stream by stream (compressed streams
after inflating) shows no lost data. What still differs:

- storages and pages that no Container.Contents refers to (left-overs of
  deleted forms and rights) are not decompiled;
- the order of elements in Container.Contents;
- garbage that 1C leaves after the end of some streams;
- deflate output (zlib 1.3.1 instead of 1C's 1.0.4).

GComp 2.2.15 "normalized" much on the way; this version keeps:

- the MMS version and the `CRC` record that 1C 7.7 requires for versions
  above 10010, both account masks (from 10010), IDs of "based on" lists;
- fonts and types of form elements, the structure of dialog streams,
  references and types stored in forms, Container.Profile, the whole
  work book of common tables, rights values, spaces at string ends.

Where the description format had no place for a detail, it gets a
property that older descriptions simply lack: `ЧислоПолей`,
`Cnt_Ver: Нет`, `СкобкиКомпактно`, `ПробелыВКонце`, `ОснФлаги: Пусто`;
values with spaces at the ends are written in quotes; an empty
`Container.Profile` file means "no profile". Trees decompiled by older
GComp compile as before (version 10009, no CRC).

Compilation writes `<name>.gcomp-tmp` and replaces the target only on
success. Unknown properties, invalid values and broken description files
stop it with the file and line instead of being dropped silently.

A compiled MD with a changed configuration opens in 1C:Enterprise and
the Configurator under Wine; the full syntax check passes.

## Source tree format

The tree format is kept in `gcomp.ini` (`format: 4`). Decompiling always
writes the current format; compiling takes formats 3 and 4 and stops on
an older tree (format 2: GComp 2.2.15 and earlier versions of this one,
whose `gcomp.ini` has no `format`). Such a tree is converted through the
MD: compile it with the GComp that decompiled it, then decompile the MD
with this one.

Most lines of a decompiled form repeat what 1C writes into every form.
Since format 3 they leave out what follows from an element's class and
position:

- `ДопФлаги` (window styles) usual for the class;
- `ИдентификаторМетаданных: -1` (no attribute; 0 is written as `0`);
- `ПорядокОбхода` equal to the previous element's + 1, 4152 for the first;
- `Слой` of the previous element, the first layer for the first one;
- the font: 1C's dialog font (MS Sans Serif 8.5, RUSSIAN_CHARSET) when
  the element has `ШрифтПоУмолчанию`, otherwise the empty font with
  weight 0 that 1C stores; the font name is not repeated in the block;
- of the form parameters, the dialog font, `Param24: 6`, `Param25: 25`
  and `Флаги2` equal to `Флаги`.

`X`, `Y`, `Ширина`, `Высота` become one line, `Положение: X, Y, Ширина,
Высота`. On the 430 configurations above forms shrink from 6.0 to 3.3
million lines (the whole tree by 16%).

Format 4 drops small files:

- a work book with the usual single moxel page (Page.1 "Moxel WorkPlace")
  has no `ПорядокОбъектов.ord`, compiling takes `Таблица.mxl` as that
  page;
- `Container.Profile` equal to the one compiling creates is not written
  (before, it was written for every work book because of a comparison
  error); an empty file still means that the work book has none;
- an object description without line breaks, up to 1000 bytes, is the
  line `Описание: "..."` (quotes doubled) in the object's block of its
  `.mdp` instead of a `.txt` file; longer ones stay in files.

On the 430 configurations that is 77 thousand files instead of 98. Every
stream of an MD compiled from format 4 is the same as from format 3; the
storages of `TypedText` may come in another order inside the compound
file.

## Source tree in UTF-8

1C keeps text in CP1251, and so did GComp. With `--utf8` the text files of
the tree (modules `.1s`, descriptions `.mdp`, `.frm`, `.txt`, `.ord`,
`.acc`, `Container.Profile`) are written in UTF-8, which git, web
interfaces and editors show without settings; pictures and work books stay
as they are. The choice is kept in `gcomp.ini` (`encoding: UTF-8`), so
compilation and later decompilations into the same tree use it; as with
the other settings, `gcomp.ini` takes precedence over the command line.

Compilation converts back to CP1251. A character that CP1251 does not
have, or a file that is not valid UTF-8, stops it with the file and line;
a BOM is skipped. The byte 0x98, which has no CP1251 character, maps to
U+0098, so any module survives. On the 430 configurations above the MD
compiled from a UTF-8 tree is byte-identical to the one compiled from the
CP1251 tree.

See `PORTING.md` for dependencies and the plan for a native Linux build.
The Delphi GUIs (`vgcomp/`, `gcomp_vis/`) are not built.
