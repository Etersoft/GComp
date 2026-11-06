# Перенос GComp: зависимости и способы сборки

Опись по исходникам 2.2.15 (консольный `gcomp`, ~15,9 тыс. строк C++),
2026-11-07. Проект собирался Visual C++ 6 (`gcomp.dsp`, `DLL/GCompDLL.dsp`).

## Состав

- Корень: ядро разборки/сборки MD и ERT, консольный `gcomp`.
- `DLL/`: та же логика как `GComp.dll` с экспортами `GComp_Decompile`,
  `GComp_Compile`, настройками; ресурс версии `GComp.rc`.
- `vgcomp/`, `gcomp_vis/`: GUI-оболочки на Delphi над DLL; для Linux-сборки
  не нужны. Их скомпилированные `.res` в репозиторий не взяты.
- `scripts/`: Perl/Python/VBS-утилиты вокруг разобранных файлов.

## MFC: используется только не-GUI часть

Окон, диалогов, `CWinApp`, `afxwin.h` нет. `CDialogForm`, `CDialogFrame`
и подобные — собственные классы GComp для форм 1С, а не MFC.
Подключаются `<afx.h>`, `<afxtempl.h>`, `<afxcoll.h>`.

| Класс / макрос | Использование |
| --- | --- |
| `CString` | базовый класс `GCString`; Find, Format/FormatV, Mid/Left/Right, Trim*, Replace, SpanIncluding/Excluding, ReverseFind, MakeUpper, Compare, GetBufferSetLength, **AnsiToOem** |
| `CStringArray`, `CArray<>`, `CPtrArray` | Add, GetAt/SetAt, GetSize/SetSize, RemoveAt |
| `CPtrList`, `CList<>`, `POSITION` | AddTail, GetHeadPosition, GetNext, GetCount |
| `CMapStringToPtr`, `CMapWordToPtr`, `CMapPtrToPtr` | Lookup, SetAt |
| `CTime`, `CTimeSpan` | время выполнения, GetCurrentTime |
| `CObject`, `TCHAR`, `LPCTSTR` | минимально |

Свободной полной реализации MFC нет (в Wine запрос открыт годами).
Для этого подмножества достаточно собственного заголовка совместимости
(строка, массивы, списки, словари, время) — порядка сотен строк, либо
готового CString-совместимого класса (например, CStdString).

## Win32 / OLE / CRT

- **OLE structured storage** — сам формат `1Cv7.MD`/`.ert`: `IStorage` (35),
  `IStream` (43), `IEnumSTATSTG`, `StgOpenStorage`, `StgCreateDocfile`,
  `CreateStreamOnHGlobal`, `CoInitialize`. Главная часть переноса.
- **Сравнение строк `_stricoll` (80 вызовов)**: порядок зависит от русской
  локали Windows. Для совпадения вывода нужна та же сортировка CP1251, а не
  произвольная `strcoll` Linux.
- Файлы и пути: `_findfirst/_findnext/_findclose`, `_mkdir`, `_rmdir`, `_stat`,
  `_chmod`, `_splitpath`, `_tempnam`, `GetFileAttributes`/`SetFileAttributes`.
- Прочее: `stricmp`/`_strnicmp`, `__iscsym`, `FormatMessage`/`GetLastError`,
  `AnsiToOem` (CP1251 → CP866 для консоли).
- Тексты исходников и документации — CP1251 и CP866 с CRLF.

## Варианты сборки

1. **MinGW-w64 (Windows exe) или winegcc (Winelib) + заголовок совместимости
   вместо MFC.** OLE storage и CRT берутся из Windows/Wine. Быстрый путь к
   рабочему бинарнику из исходников; Linux — через Wine.
2. **Нативный Linux.** Тот же заголовок совместимости плюс собственный слой
   хранилища: реализация Compound File Binary (MS-CFB) или библиотека
   (libgsf: LGPL, чтение/запись OLE2) за интерфейсом, повторяющим нужную часть
   IStorage/IStream; POSIX вместо `_find*`; таблица сравнения CP1251 вместо
   `_stricoll`.

Порядок: сначала вариант 1 как эталон поведения, затем вариант 2 с
проверкой побайтового совпадения результатов разборки и открытием собранного
MD в оригинальной 1С 7.7.
