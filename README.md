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

The tree format is kept in `gcomp.ini` (`format: 6`). Decompiling always
writes the current format; compiling stops on an older tree with a hint:
such a tree is converted through the MD, compiled with the GComp that
decompiled it and decompiled with this one.

### Files

- descriptions of metadata, forms, rights, menus, commands and panels are
  YAML (`.yaml`, see below);
- modules (`.1s`) and long descriptions (`.txt`) are text as 1C keeps
  them; work books (`.mxl`) and pictures are copied as they are;
- `ПорядокОбъектов.ord` keeps the order of objects; a work book with the
  usual single page (Page.1 "Moxel WorkPlace" in `Таблица.mxl`) has none;
- `Container.Profile` is written only when it differs from the one that
  compiling creates; an empty file means that the work book has none.

Text files are UTF-8 (`encoding: UTF-8`; `--cp1251` keeps CP1251 for all
but YAML) with unix line ends (`eol: unix`; `--eol windows` writes CRLF
as 1C keeps them). Compilation converts back: a character that CP1251
does not have or a file that is not valid UTF-8 stops it with the file and
line, a BOM is skipped, the byte 0x98 maps to U+0098. With unix line ends
a file is converted only when all its lines end with CRLF in the MD; a
file with CR is taken as is, and the rare texts whose lines end with a
bare LF are listed in `gcomp.lf`. As with the other settings, `gcomp.ini`
takes precedence over the command line. Names starting with a dot
(`.git`, `.gitignore`) are left alone.

### Descriptions in YAML

GComp keeps parsing its descriptions of `Name: value` lines and `{ }`
blocks; `.yaml` files are converted on reading and writing:

```yaml
Валюты:
  Комментарий: Валюта
  Длина кода: 3
  Атрибуты:
    Курс:
      Тип: Число
      Описание: |-
        Текущий курс
        валюты
  Формы списка:
    ФормаСписка: {}
```

- A block is a mapping, or a sequence when its names repeat or it lists
  values (references, buttons); form elements, menus and buttons are
  always sequences.
- A block whose header has a value: in a mapping the value goes under `_`
  (`Шрифт: {_: MS Sans Serif, Жирный: 0}`), in a sequence the item starts
  with it and goes on with the block's lines (`- BUTTON: Закрыть` and
  `Заголовок: ...` below); `_:` holds lines that cannot be keys of the
  item.
- A value is its text, whatever the YAML style: `Заголовок: Печать`,
  `Заголовок: "Печать"` and `Заголовок: 'Печать'` are the same. Whether
  1C keeps the text in quotes is known from the type of the property, so
  the quoting is not written. Values that YAML would read as other things
  (numbers like `0050`, `yes`, `-`, a leading `#` or `&`) are written in
  quotes, texts of several lines as block (`|`) or double quoted scalars.
  `!u "..."` is the rare value that the description text holds as it is
  (in quotes that are not GComp's quoting, or with spaces at the ends of a
  property kept without quotes).
- A file written by another YAML program is read the same way, whatever
  styles it chose; `!!null` is an empty value, other `!!` tags are
  ignored. Anchors, aliases and complex keys are not supported.
- Errors report the line of the `.yaml` file.

### Forms

Most lines of a decompiled form repeat what 1C writes into every form;
they are left out:

- `ДопФлаги` (window styles) usual for the class;
- `ИдентификаторМетаданных: -1` (no attribute; 0 is written as `0`);
- `ПорядокОбхода` equal to the previous element's + 1, 4152 for the first;
- `Слой` of the previous element, the first layer for the first one;
- the font: 1C's dialog font (MS Sans Serif 8.5, RUSSIAN_CHARSET) when
  the element has `ШрифтПоУмолчанию`, otherwise the empty font with
  weight 0 that 1C stores; the font name is not repeated in the block;
- of the form parameters, the dialog font, `Param24: 6`, `Param25: 25`
  and `Флаги2` equal to `Флаги`.

`X`, `Y`, `Ширина`, `Высота` are one line, `Положение: X, Y, Ширина,
Высота`.

### Object descriptions

An object's description (UserHelp) of up to 2000 bytes, whose lines end
with CRLF, is `Описание:` in the object's block of its `.yaml`; longer
ones stay in `.txt` files.

### On the corpus

On the 430 configurations above the trees hold 70 thousand files (98
thousand before format 4) and forms take 2.6 million lines instead of 6.0
(3.3 million in braces, YAML is 23% shorter). Every
stream of an MD compiled from a format 6 tree is the same as from GComp
before these formats; the storages of `TypedText` may come in another
order inside the compound file. Trees rewritten by PyYAML, in its own
styles or with every scalar double quoted, compile to the same streams.

See `PORTING.md` for dependencies and the plan for a native Linux build.
The Delphi GUIs (`vgcomp/`, `gcomp_vis/`) are not built.
