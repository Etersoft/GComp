# GComp

GComp decompiles a 1C:Enterprise 7.7 configuration (`1Cv7.MD`) or an
external report (`.ert`) into a tree of text files and compiles the tree
back. A configuration can then be kept in Git, changes reviewed as diffs,
merged and built from the repository.

This is GComp 3, based on GComp 2.2.15 by the Alterplast team
(https://1c.alterplast.ru/gcomp/). It builds without Visual C++ 6 and
Microsoft MFC. Decompiling and compiling back loses nothing, and the
source tree is YAML and UTF-8 text.

## What is new in 3.0

- **Lossless round trip.** A compiled MD has the same streams as the
  original: the MMS version and the CRC that 1C 7.7 requires are kept,
  and so are forms, fonts, rights, work books and spaces at the ends of
  strings. GComp 2.2.15 dropped or normalised all of these.
- **YAML descriptions.** Metadata, forms, rights, menus and panels are
  written as `.yaml` files. A value is its text in any YAML style, so
  other YAML tools can read and rewrite the files.
- **UTF-8 and line ends.** Modules and texts are written in UTF-8 with
  unix line ends by default. `--cp1251` and `--eol windows` keep them as
  1C stores them.
- **Short forms.** Lines that 1C writes into every form are left out,
  and a form takes less than half the lines it took before. Window styles and font
  fields have their WinAPI names.
- **Safe compilation.** An unknown property, an invalid value or a broken
  file stops the compilation with the file and line. The target is
  replaced only when the compilation succeeds. `.git` and other files
  starting with a dot are left alone.
- **Free build.** GComp builds with winegcc or MinGW-w64 and does not need
  1C's `V77.zlibEngine`.

GComp 3 cannot compile trees decompiled by GComp 2.2.15. To convert such a
tree, compile it into an MD with the old GComp and decompile the MD with
this one. See the release notes for the details.

## Usage

`gcomp.exe` needs `GComp.dll` next to it and runs on Windows or under
Wine:

    gcomp -d -F 1Cv7.MD -D src     # decompile the configuration into src
    gcomp -c -F 1Cv7.MD -D src     # compile src into the configuration
    gcomp -d -F report.ert         # an external report, into SRC/report
    gcomp -h                       # all options

Close the configuration in the Configurator before compiling into it.
GComp writes `1Cv7.MD.gcomp-tmp` and replaces `1Cv7.MD` only when the
compilation succeeds. Decompiling into an existing tree updates it and
removes the files of objects that are no longer in the MD. Files and
directories whose names start with a dot, such as `.git`, are kept.

Options of the source tree (they are kept in `gcomp.ini`, which takes
precedence over the command line):

| Option | Meaning |
| --- | --- |
| `--cp1251` | text files other than `.yaml` in CP1251, as 1C keeps them (default UTF-8) |
| `--eol unix\|windows` | line ends of text files (default unix) |
| `--more-subfolders` | constants, enumerations, registers etc. one object per directory |
| `--sort-rights`, `--sort-form-controls` | sorted rights objects and form elements |
| `--filter <path>` | decompile or compile only some objects, e.g. `Справочники/Банки` |
| `--no-defaults` | write every property, even those equal to the defaults |
| `--mxl-text` | write a Markdown view `.mxl.md` next to every table, see below |

## The source tree

```
src/
  gcomp.ini                    format, encoding, line ends, options
  ОписаниеКонфигурации.yaml
  ГлобальныйМодуль.1s
  Константы/Константы.yaml
  Справочники/
    Банки/
      Структура.yaml           the catalog: attributes, forms, options
      Форма.yaml               the element form
      МодульФормы.1s
      Основная.fls/            a list form: Форма.yaml, МодульФормы.1s
  Документы/
    АвансовыйОтчет/
      Структура.yaml
      Форма.yaml
      МодульПроведения.1s
      Таблица.mxl              the print form (work book)
      Описание.txt             the description of the object
  Права/Администратор.yaml
  Интерфейсы/Бухгалтер/{Меню,Панели,Команды}.yaml
```

- `.yaml` holds the descriptions of metadata, forms, rights, menus,
  commands and panels.
- `.1s` files are modules and `.txt` files are object descriptions.
  Work books (`.mxl`) and pictures are copied as they are.
- `ПорядокОбъектов.ord` keeps the order of objects. A work book with the
  usual single page has none.
- `Container.Profile` is written only when it differs from the one that
  compiling creates. An empty file means that the work book has none.

### Descriptions in YAML

```yaml
Банки:
  Длина кода: 9
  Серии кодов: Во всём справочнике
  Основное представление: В виде кода
  Форма для выбора: Справочник.Банки.Form.ДляВыбора
  Атрибуты:
    КоррСчет:
      Синоним: Корреспондентский счет
      Тип: Строка
      Длина: 20
```

- **Values.** A value is its text, whatever the YAML style:
  `Заголовок: Печать`, `Заголовок: "Печать"` and `Заголовок: 'Печать'`
  are the same. Whether 1C keeps the text in quotes is known from the
  type of the property.
  - Values that YAML would read as other things are written in quotes:
    numbers like `0050`, `yes`, `-`, or a leading `#` or `&`.
  - Texts of several lines are block (`|`) or double quoted scalars.
  - `!u "..."` is the rare value kept exactly as GComp's internal text
    holds it.
- **Blocks.** A block is a mapping, or a sequence when its names repeat or
  it lists values. Form elements, menus and buttons are always sequences.
- **Blocks with a value.** In a mapping, the value of the block's header
  goes under `_` (`Шрифт: {_: MS Sans Serif, Жирный: 0}`). A sequence
  item starts with it (`- BUTTON: OK`) and goes on with the block's
  lines.
- **Other YAML programs.** Files written by them are read the same way.
  `!!null` is an empty value and other `!!` tags are ignored. Anchors,
  aliases and complex keys are not supported.
- **Errors.** Error messages give the line of the `.yaml` file.
- **Object descriptions.** An object's description is a `.txt` file. A
  description given as `Описание:` in the object's block is read too.

### Forms

```yaml
Диалог:
  Параметры формы:
    Параметры:
      Ширина: 480
      Высота: 74
      Заголовок: Налог на рекламу
      Слои:
        Основной: Видимый, Активный
  Элементы:
    - STATIC: Текст_точность
      Заголовок: Точн.
      ДопФлаги: SS_RIGHT, SS_CENTERIMAGE, WS_VISIBLE, WS_CHILD
      Положение: 156, 58, 18, 10
      ОснФлаги: ШрифтПоУмолчанию, ЦветТекстаАвто
    - BUTTON: OK
      Заголовок: Закрыть
      Положение: 6, 17, 54, 13
      Формула: '#Закрыть'
```

**Left out.** Most lines of a form repeat what 1C writes into every form,
so they are not written:

- the usual window styles of the class;
- `ИдентификаторМетаданных: -1`;
- a tab order of the previous element's + 1, starting from 4152;
- the layer of the previous element;
- the usual font: 1C's dialog font, or the empty font when the element
  does not use the default one;
- the usual parameters of the form.

**Written in short.** `Положение` is `X, Y, Ширина, Высота` on one line.

**Window styles.** `ДопФлаги` are the window styles of the element with
their WinAPI names:

- `WS_` styles for every element;
- `BS_` for `BUTTON`, `CHECKBOX`, `RADIO`, `1CGROUPBOX` and `PICTURE`;
- `SS_` for `STATIC`;
- `ES_` for `1CEDIT` and `BMASKED`;
- `CBS_`, `LBS_` and `TVS_` for combo boxes, list boxes and trees.

A bit without a name is `FlagN`.

**Fonts.** The font block names the fields of LOGFONT:

- `ШиринаСимвола`, `УголСтроки`, `УголСимвола`;
- `Жирный`, `Наклон`, `Подчёркивание`, `Зачёркивание`;
- `НаборСимволов` (204 is RUSSIAN_CHARSET);
- `ТочностьВывода`, `ТочностьОтсечения`, `Качество`, `ШагИСемейство`.

### Text files

Modules and texts are UTF-8 with unix line ends by default. A BOM is
skipped and the byte 0x98 maps to U+0098. When compiling, the files are
converted back:

- **Characters.** A character that CP1251 does not have, or a file that
  is not valid UTF-8, stops the compilation with the file and line.
- **Unix line ends.** A file is converted only when all its lines end
  with CRLF in the MD. A file with CR is taken as it is.
- **Bare LF.** The rare texts whose lines end with a bare LF are listed
  in `gcomp.lf`.

`.yaml` files are always UTF-8.

### Tables

Tables (`.mxl`) stay binary: they are the `Page.N` streams as they are, and
compiling takes them. With `--mxl-text` decompiling also writes a Markdown
view `<table>.mxl.md` (always UTF-8) next to each table, for reviewing
changes in git and on GitHub or GitLab; it is never read back, so editing
it changes nothing. A table without content gets no view, and one that
cannot be read gets a warning instead.

The view starts with the size of the table, its fonts and its header and
footer. The texts of the cells follow as Markdown tables, one for each row
section of the top level (and one for each run of rows outside sections),
with only the rows and columns that have text; line breaks are `<br>`, and
the first cell of a join tells where the join ends (`до R5C10`). Then come
lists: the formatting of columns, rows and cells by the names of the
properties of a table area in the language of 1C (`РамкаСлева тонкая
сплошная`, `ЦветФона светло-серый`, `ВысотаСтроки 15`), the details and the value of dialog table cells, joins,
objects, sections, page breaks and named areas. Rows and columns are
numbered from 1 as in 1C, e.g. `R5C2`.

```
## Шапка (R1–R5)

|  | C2 | C67 |
|---|---|---|
| R5 | СокрЛП(Фирма.ЮрЛицо.ПолнНаименование) <sub>до R5C60</sub> | 0317022 |

## Оформление

### Строки и ячейки

- R5: РазмерШрифта 9, Полужирный нет, ВысотаСтроки 10
  - R5C2: Тип Выражение
  - R5C67: Рамка тонкая сплошная, ГоризонтальноеПоложение по центру, ВертикальноеПоложение по центру
```

The layout of the tables is described in `docs/moxcel.md`.

### Format versions

`gcomp.ini` keeps the format of the tree (`format: 7`). Decompiling
always writes the current format. Compiling stops on a tree of an older
format and tells how to convert it through the MD.

Format 7 (after GComp 3.0.0) names properties that format 6 wrote as
numbers or with typos: the choice form of a form element (`ФормаВыбора`),
the journals of the configuration (`Журнал подчинённых документов`,
`Полный журнал документов`), the name of an interface command
(`Название`), the list forms and the operation document of the accounting
parameters, and corrects `Автонумерация` and four accounting property
names. References to objects are written by name.

## What a round trip keeps

The aim is that decompiling and compiling back loses nothing; a
compiled MD is not a byte copy of the original. GComp 3 keeps:

- the MMS version and the `CRC` record that 1C 7.7 requires for versions
  above 10010;
- both account masks, and the IDs of "based on" lists;
- fonts and types of form elements, the structure of dialog streams, and
  the references and types stored in forms;
- `Container.Profile`, the whole work book of common tables, rights
  values, and spaces at the ends of strings.

What still differs from the original:

- storages and pages that no `Container.Contents` refers to (left-overs of
  deleted forms and rights) are not decompiled;
- the order of elements in `Container.Contents` and of the storages of
  `TypedText` inside the compound file;
- garbage that 1C leaves after the end of some streams;
- deflate output: zlib 1.3.1 is used instead of 1C's 1.0.4.

**Testing.** It was checked on 431 configurations and external reports
and on a production configuration:

- all of them but one encrypted MD decompile and compile;
- every stream of the compiled MD is the same as the original's, with
  compressed streams compared after inflating;
- trees rewritten by PyYAML compile to the same streams, both in
  PyYAML's own styles and with every scalar double quoted.

A compiled MD with a changed configuration opens in 1C:Enterprise and the
Configurator under Wine, and the full syntax check passes.

## Building

### Source layout

| Directory | Contents |
| --- | --- |
| `src/common/` | strings, messages, the lexer, YAML, the Wine `atexit` fix |
| `src/storage/` | compound files, streams, containers, order of objects, pictures |
| `src/metadata/` | metadata objects (MMS), their types and name tables, description files |
| `src/forms/` | dialog forms and their property tables |
| `src/ui/` | user interfaces: menus, commands, panels, rights |
| `src/DLL/` | `GComp.dll`: exported functions, version resource |
| `src/cli/` | `gcomp.exe`: command line and help |
| `gui/` | Delphi GUIs `vgcomp` and `gcomp_vis` (not built) |
| `mfc-compat/` | free replacement for the non-GUI core of MFC 4.2 |
| `scripts/` | Perl, Python and VBS tools from GComp 2.2.15 |
| `docs/` | descriptions of the 1C 7.7 stream formats (Russian), licenses |

The source files include each other by name, so every directory of `src/`
is on the include path. The lists of files to compile are the Visual C++ 6
projects `src/DLL/GCompDLL.dsp` and `src/cli/gcomp.dsp`; the build scripts
read them.

### MinGW-w64

Needs the i686-w64-mingw32 cross compiler (Fedora: `mingw32-gcc-c++`).
The modules are linked with `-static` and need only system DLLs. zlib
1.3.1 is downloaded and checked by SHA256.

    ./build-mingw.sh

The result is `build-mingw/GComp.dll` and `build-mingw/gcomp.exe`.

### winegcc

Needs the Wine development tools: winegcc/wineg++, wrc and i386-windows
import libraries.

    ./build-winegcc.sh

The result is `build-pe/GComp.dll` and `build-pe/gcomp.exe`. The winegcc
build has two quirks under Wine:

- Wine's `__iscsym` is ASCII-only, so Cyrillic file names come out as `_`
  (https://bugs.etersoft.ru/show_bug.cgi?id=19905). The MinGW build is
  not affected.
- `src/common/wine-pe-atexit.c` works around another Wine bug: the static
  `atexit()` linked into PE modules uses an uninitialised table
  (https://bugs.etersoft.ru/show_bug.cgi?id=19907).

The MFC classes come from the bundled `mfc-compat/`, a free replacement
for the non-GUI core of MFC 4.2 (LGPL-2.1, see its README). The sources
still compile with MSVC (`src/gcomp.dsw`). `PORTING.md` lists the
dependencies and the plan for a native Linux build. The Delphi GUIs
(`gui/`) are not built.

## License

GComp is free software under the GNU General Public License. The original
`docs/licenses/license.txt` does not state a version; the texts it
refers to, `gnugpl.eng.txt` and its Russian translation `gnugpl.rus.txt`,
are GPL version 2. Under section 9 of GPL v2 a program that specifies no
version may be used under any version, so GComp with the changes in this
repository is distributed under GPL-2.0-or-later (`COPYING`, text from
gnu.org). New files carry `SPDX-License-Identifier: GPL-2.0-or-later`.

The descriptions of the moxcel and rights formats (`docs/moxcel.md`,
`docs/rights.md`), first written in HTML for the 1L project, are under the
GNU Free Documentation License 1.2 or later (`docs/licenses/gnu_fdl.html`);
see `docs/README.md`.

Bundled components: `mfc-compat/` — LGPL-2.1-or-later; zlib (downloaded
at build time) — zlib license. Both are compatible with the GPL.

## History

- `Import GComp 2.2.15 sources` — upstream `gcomp_src_2.2.15.zip`
  (SHA256 `7d3f727f69089472640bbcf656decf3fe18500841d05a9176cd39919ebe6bbf8`),
  byte-identical except removed compiled Delphi resources, a 1C spreadsheet
  sample and CVS metadata. Sources keep upstream CP1251/CP866 and CRLF.
- Later commits make it buildable without Visual C++ 6 and Microsoft MFC
  while keeping it compilable by MSVC, make the round trip lossless and
  introduce the YAML source tree. The upstream history is in `ChangeLog`.
