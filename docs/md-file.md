# Файл конфигурации 1Cv7.MD и внешний отчёт .ert

© 2026 Виталий Липатов. Распространяется на условиях GPL, см.
[licenses/license.txt](licenses/license.txt).

Документ собран при переносе GComp 3 по исходникам GComp
(`src/metadata/NameTable.cpp`, `src/storage`), разбору 431 конфигурации и
внешнего отчёта и по коду 1С 7.7 (`BkEnd.dll`), из которого восстановлен
расчёт CRC. Форматы отдельных потоков описаны в
[metadata.md](metadata.md), [dialog-forms.md](dialog-forms.md),
[interfaces.md](interfaces.md), [moxcel.md](moxcel.md) и
[rights.md](rights.md).

## Контейнер

`1Cv7.MD` и `.ert` — составной файл OLE (Compound File Binary, MS-CFB):
дерево хранилищ (каталогов) и потоков. Каждое хранилище содержит поток
`Container.Contents` со списком своих элементов:

```
{"Container.Contents",{"<Тип>","<Имя потока или хранилища>","<Название>",""}, ...}
```

Четвёртое поле во всех просмотренных файлах пустое. 1С перечисляет элементы в
своём порядке, а не в порядке каталога CFB (тот сортирует имена по длине), и
заканчивает поток переводом строки. GComp 3 записывает этот порядок так же.
Хранилища и потоки, на которые не ссылается ни один `Container.Contents`, —
остатки удалённых объектов; 1С их не читает, и GComp их не выгружает.

Модули (`MD Programm text`) и картинки сжаты raw deflate (zlib, windowBits −15).
1С пишет их zlib 1.0.4 (`zlibeng.dll`, уровень 4, memLevel 9). Другие
версии zlib сжимают иначе, но распакованный текст тот же, и 1С такие потоки
читает. За концом сжатых данных у некоторых потоков лежит мусор буфера 1С.

Потоки метаданных и диалогов начинаются с длины текста: 1 байт, `FF` и
2 байта или `FF FF FF` и 4 байта, little-endian. Подробно — в
[dialog-forms.md](dialog-forms.md#заголовок-потока).

## Хранилища 1Cv7.MD

Порядок корневого `Container.Contents`: Metadata, Subconto, SubList,
SubFolder, Document, Journal, Report, TypedText, UserDef, Picture,
CalcJournal, CalcVar, AccountChart, AccountChartList, OperationList,
Operation, GlobalData, ProvList.

| Хранилище | Тип в Container.Contents | Содержимое | Каталог GComp |
| --- | --- | --- | --- |
| `Metadata` | MetaDataContainer | `Main MetaData Stream` — описание всех объектов, `GUIDData`, `TagStream` | `Структура.yaml` и описания объектов, `GUIDData`, `TagStream` |
| `Subconto/Subconto_Number<ID>` | SubcontoContainer | формы элемента справочника | `Справочники/<Имя>` |
| `SubList/SubList_Number<ID>` | SublistContainer | формы списка справочника | `Справочники/<Имя>/<Форма>.fls` |
| `SubFolder/SubFolder_Number<ID>` | SubcontoGroupFolder | формы группы справочника | `Справочники/<Имя>/ФормаГруппы` |
| `Document/Document_Number<ID>` | DocumentContainer | формы документов | `Документы/<Имя>` |
| `Journal/Journal_Number<ID>` | JournalContainer | формы журналов документов | `ЖурналыДокументов/<Имя>` |
| `Report/Report_Number<ID>` | ReportContainer | отчёты | `Отчеты/<Имя>` |
| `CalcVar/CalcVar_Number<ID>` | CalcVarContainer | обработки | `Обработки/<Имя>` |
| `CalcJournal/CalcJournal_Number<ID>` | CalcJournalContainer | формы журналов расчётов | `ЖурналыРасчетов/<Имя>` |
| `TypedText` | TypedTextContainer | `ModuleText_Number1` — глобальный модуль, `Transact_Number<ID>` — модули проведения, `CalcAlg_Number<ID>` — модули видов расчёта, `UserHelp_Number<ID>` — описания объектов | `ГлобальныйМодуль.1s`, `МодульПроведения.1s`, `МодульРасчета.1s`, `Описание.txt` |
| `UserDef` | UserDefContainer | `Page.1` (WorkPlaceType) — интерфейсы, `Page.2` (RigthType, так у 1С) — права | `Интерфейсы`, `Права` |
| `Picture` | PictureContainer | `Gallery` и картинки `__Picture.<ID в hex>` | `PictureGallery` |
| `AccountChart/AccountChart_Number<ID>` | AccountChartContainer | формы счёта | `ПараметрыБухгалтерии/Счета/ФормаСчета` |
| `AccountChartList` | | формы списка плана счетов | `ПараметрыБухгалтерии/Счета` |
| `Operation`, `OperationList` | | формы и формы списка (журнала) операций | `ПараметрыБухгалтерии/Операция` |
| `ProvList` | | формы списка проводок | `ПараметрыБухгалтерии/Проводка` |
| `GlobalData/GlobalData_Number1` | | общие таблицы | `ОбщиеТаблицы` |

Хранилище объекта с формой содержит `WorkBook`:

| Поток | Тип | Содержимое |
| --- | --- | --- |
| `Dialog Stream` | DialogEditor, «Dialog Form» | [диалоговая форма](dialog-forms.md) |
| `MD Programm text` | TextDocument, «Module text» | модуль формы, сжат |
| `Page.<N>` | Moxcel.Worksheet, название таблицы | [таблица moxcel](moxcel.md) |
| `Container.Profile` | | настройки: `MoxelName`, `MoxelPos`, `UUID`, `Entry`, `MoxelNextMode` |

`Container.Profile` выглядит так:

```
{
{"MoxelName","",""},
{"MoxelPos","0",""},
{"UUID","D41D8CD98F00B204E9800998ECF8427E",""},
{"Entry","1",""},
{"MoxelNextMode","1",""}}
```

GComp 3 не выгружает его, когда он совпадает с тем, что создаётся при
сборке.

### Интерфейсы и права

`UserDef/Page.1` содержит хранилище `Page.<N>` на каждый интерфейс; тип
записи — `UsersInterfaceType` (основной интерфейс) или
`SubUsersInterfaceType` (подчинённый), название — имя интерфейса. В
хранилище интерфейса лежат потоки `Commands` (команды), `Page.1` (меню,
MenuEditorType) и `Page.2` (панели, ToolbarEditorType); у подчинённого
интерфейса — только `Commands`. Форматы — в [interfaces.md](interfaces.md).

`UserDef/Page.2` содержит потоки `Page.<N>` с наборами прав, формат — в
[rights.md](rights.md).

## Внешний отчёт .ert

Внешний отчёт — тот же составной файл без хранилищ, все потоки в корне:

| Поток | Тип в Container.Contents |
| --- | --- |
| `Main MetaData Stream` | MetaDataHolderContainer |
| `Dialog Stream` | DialogEditor, «Dialog Form» |
| `MD Programm text` | TextDocument, «Module text» |
| `Inplace description` | MetaDataDescription, «Описание» |
| `Page.<N>` | Moxcel.Worksheet, название таблицы |
| `Container.Profile` | |

Поток метаданных внешнего отчёта содержит те же записи, что и у
конфигурации, но почти пустые; ID в форме отчёта относятся к той
конфигурации, в которой отчёт создан. Ключ `--truncate-mms` заменяет его
при сборке пустым.

## Main Metadata Stream

Текст списков в фигурных скобках, CP1251, с `\r\n` перед вложенными
списками. Корневой список — `{"MainDataContDef", ...}`, затем `TaskItem`,
списки объектов (см. [metadata.md](metadata.md)) и в конце запись `CRC`.

### MainDataContDef

```
{"MainDataContDef","<NextID>","<Версия>","<UnknownID>"}
```

| № | Поле | Описание |
| ---: | --- | --- |
| 1 | NextID | Следующий свободный ID объекта |
| 2 | Версия | Версия формата метаданных. В корпусе — 10011 у всех 30 конфигураций и 399 внешних отчётов, 10010 у одного отчёта |
| 3 | UnknownID | Во всех 430 читаемых файлах корпуса — 7120 |

В версии 10010 у плана счетов две маски кода счёта; GComp 2.2.15 при сборке
понижал версию до 10009 и терял первую маску.

### TaskItem — описание конфигурации

Свойства задачи, в дереве GComp 3 — `ОписаниеКонфигурации.yaml`:

| № | Свойство GComp 3 | Описание |
| ---: | --- | --- |
| 1 | ID | ID |
| 2 | Идентификатор | Имя конфигурации |
| 3 | Комментарий | Комментарий, часто номер релиза |
| 4 | Синоним | Синоним |
| 5 | Пароль | Пароль конфигурации |
| 6 | Основной язык | Основной язык конфигурации |
| 7 | Журнал подчинённых документов | ID журнала, отмеченного «Использовать как журнал подчиненных документов» |
| 8 | Непосредственное удаление | Удаление объектов без пометки |
| 9 | Способ округления | Округление при вычислениях |
| 10 | Полный журнал документов | ID журнала, отмеченного «Использовать как полный журнал документов» |

Параметры 7 и 10 — флажки в диалоге свойств журнала документов; в потоке они
хранятся в описании задачи. Значение бывает пустым или `0`, если журнал не
выбран.

### CRC

Для версии выше 10010 последний элемент корня — `{"CRC","N"}`. Без него или
с неверным значением 1С не открывает конфигурацию («Общая файловая ошибка при
доступе к ...\1Cv7.MD\Metadata\Main MetaData Stream»).

`N` считается по тексту потока, пересохранённому без переводов строк (вне
кавычек), из которого удалён элемент `CRC`. Для каждого байта `c` (signed
char) с номером `i`:

```
b3 += c;  b1 ^= c % 3;  b2 ^= c % 7;  b0 ^= (signed char)(i + c) % 11;
N = b0 | b1 << 8 | b2 << 16 | b3 << 24     (печатается как беззнаковое)
```

Остатки — как у деления в C (знак делимого). Алгоритм восстановлен по
`CMetaDataCont::LoadFromList` и `SaveToList` из `BkEnd.dll`. GComp 3
вычисляет CRC при сборке: потоки метаданных корпуса, собранные им, совпадают
с исходными, а MD с изменённой конфигурацией открывается в 1С.
