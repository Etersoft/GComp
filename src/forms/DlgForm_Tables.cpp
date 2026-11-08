
#include "DialogForm.h"


CHotKey::VK_Pair CHotKey::virtual_keys[] = {
	{0x01, "LButton"},
	{0x02, "RButton"},
	{0x08, "BackSpase"},
	{0x09, "Tab"},
	//{0x10, "Shift"},
	//{0x11, "Ctrl"},
	//{0x12, "Alt"},
	{0x0d, "Enter"},
	{0x13, "Pause"},
	{0x14, "CapsLock"},
	{0x1b, "Esc"},
	{0x20, "Space"},
	{0x21, "PgUp"},
	{0x22, "PgDn"},
	{0x23, "End"},
	{0x24, "Home"},
	{0x25, "Left"},
	{0x26, "Up"},
	{0x27, "Right"},
	{0x28, "Down"},
	{0x2c, "PrintScreen"},
	{0x2d, "Ins"},
	{0x2e, "Del"},
	{0x5d, "Apps"},
	{0x60, "Gray0"},
	{0x61, "Gray1"},
	{0x62, "Gray2"},
	{0x63, "Gray3"},
	{0x64, "Gray4"},
	{0x65, "Gray5"},
	{0x66, "Gray6"},
	{0x67, "Gray7"},
	{0x68, "Gray8"},
	{0x69, "Gray9"},
	{0x6a, "Gray*"},
	{0x6b, "Gray+"},
	{0x6c, "Separator"},
	{0x6d, "Gray-"},
	{0x6e, "GrayDecimal"},
	{0x6f, "Gray/"},
	{0x70, "F1"},
	{0x71, "F2"},
	{0x72, "F3"},
	{0x73, "F4"},
	{0x74, "F5"},
	{0x75, "F6"},
	{0x76, "F7"},
	{0x77, "F8"},
	{0x78, "F9"},
	{0x79, "F10"},
	{0x7a, "F11"},
	{0x7b, "F12"},
	{0x90, "NumLock"},
	{0x91, "ScrollLock"},
	{0xa0, "LShift"},
	{0xa1, "RShift"},
	{0xa2, "LControl"},
	{0xa3, "RControl"},
	{0x90, "NumLock"},
	{0, ""}
};

//===============================================================================
//======  CFormFont  ============================================================
//===============================================================================
CFormFont* CFormFont::DefVal = new CFormFont;
CObjectProperty CFormFont::Props[] = {
	{"Шрифт",   13, CObjectProperty::t_string, VAR_OFFSET(DefVal, FontName)},
	{"Размер",  0,  CObjectProperty::t_prop, VAR_OFFSET(DefVal, FontSize)},
	{"ШиринаСимвола", 1,  CObjectProperty::t_int,  VAR_OFFSET(DefVal, Param2)},
	{"УголСтроки", 2,  CObjectProperty::t_int,  VAR_OFFSET(DefVal, Param3)},
	{"УголСимвола", 3,  CObjectProperty::t_int,  VAR_OFFSET(DefVal, Param4)},
	{"Жирный",  4,  CObjectProperty::t_prop, VAR_OFFSET(DefVal, FontBold)},
	{"Наклон",  5,  CObjectProperty::t_prop, VAR_OFFSET(DefVal, FontItalic)},
	{"Подчёркивание", 6, CObjectProperty::t_prop, VAR_OFFSET(DefVal, FontUnderline)},
	{"Зачёркивание", 7,  CObjectProperty::t_int, VAR_OFFSET(DefVal, Param8)},
	{"НаборСимволов", 8,  CObjectProperty::t_int, VAR_OFFSET(DefVal, Param9)},
	{"ТочностьВывода", 9,  CObjectProperty::t_int, VAR_OFFSET(DefVal, Param10)},
	{"ТочностьОтсечения", 10, CObjectProperty::t_int, VAR_OFFSET(DefVal, Param11)},
	{"Качество", 11, CObjectProperty::t_int, VAR_OFFSET(DefVal, Param12)},
	{"ШагИСемейство", 12, CObjectProperty::t_int, VAR_OFFSET(DefVal, Param13)},
	{NULL, 0, CObjectProperty::t_int, 0}
};
CPropertySet CFormFont::PropSet(Props, DefVal);


//===============================================================================
//======  CFormControlBase  =====================================================
//===============================================================================
/* Window styles of form elements, the WinAPI names; a group of bits that
   holds one of several values (the button type, the alignment) has the
   mask of the group. Bits without a name are written FlagN. */
static const CWindowStyle::TName WS_Names[] = {
	{0x00010000, 0x00010000, "WS_TABSTOP"},
	{0x00020000, 0x00020000, "WS_GROUP"},
	{0x00040000, 0x00040000, "WS_THICKFRAME"},
	{0x00080000, 0x00080000, "WS_SYSMENU"},
	{0x00100000, 0x00100000, "WS_HSCROLL"},
	{0x00200000, 0x00200000, "WS_VSCROLL"},
	{0x00400000, 0x00400000, "WS_DLGFRAME"},
	{0x00800000, 0x00800000, "WS_BORDER"},
	{0x01000000, 0x01000000, "WS_MAXIMIZE"},
	{0x02000000, 0x02000000, "WS_CLIPCHILDREN"},
	{0x04000000, 0x04000000, "WS_CLIPSIBLINGS"},
	{0x08000000, 0x08000000, "WS_DISABLED"},
	{0x10000000, 0x10000000, "WS_VISIBLE"},
	{0x20000000, 0x20000000, "WS_MINIMIZE"},
	{0x40000000, 0x40000000, "WS_CHILD"},
	{0x80000000, 0x80000000, "WS_POPUP"},
	{0, 0, NULL}
};

static const CWindowStyle::TName BS_Names[] = {
	{0x000F, 0x0001, "BS_DEFPUSHBUTTON"},
	{0x000F, 0x0002, "BS_CHECKBOX"},
	{0x000F, 0x0003, "BS_AUTOCHECKBOX"},
	{0x000F, 0x0004, "BS_RADIOBUTTON"},
	{0x000F, 0x0005, "BS_3STATE"},
	{0x000F, 0x0006, "BS_AUTO3STATE"},
	{0x000F, 0x0007, "BS_GROUPBOX"},
	{0x000F, 0x0008, "BS_USERBUTTON"},
	{0x000F, 0x0009, "BS_AUTORADIOBUTTON"},
	{0x000F, 0x000A, "BS_PUSHBOX"},
	{0x000F, 0x000B, "BS_OWNERDRAW"},
	{0x000F, 0x000C, "BS_SPLITBUTTON"},
	{0x000F, 0x000D, "BS_DEFSPLITBUTTON"},
	{0x000F, 0x000E, "BS_COMMANDLINK"},
	{0x000F, 0x000F, "BS_DEFCOMMANDLINK"},
	{0x0020, 0x0020, "BS_LEFTTEXT"},
	{0x0040, 0x0040, "BS_ICON"},
	{0x0080, 0x0080, "BS_BITMAP"},
	{0x0300, 0x0100, "BS_LEFT"},
	{0x0300, 0x0200, "BS_RIGHT"},
	{0x0300, 0x0300, "BS_CENTER"},
	{0x0C00, 0x0400, "BS_TOP"},
	{0x0C00, 0x0800, "BS_BOTTOM"},
	{0x0C00, 0x0C00, "BS_VCENTER"},
	{0x1000, 0x1000, "BS_PUSHLIKE"},
	{0x2000, 0x2000, "BS_MULTILINE"},
	{0x4000, 0x4000, "BS_NOTIFY"},
	{0x8000, 0x8000, "BS_FLAT"},
	{0, 0, NULL}
};

static const CWindowStyle::TName SS_Names[] = {
	{0x001F, 0x0001, "SS_CENTER"},
	{0x001F, 0x0002, "SS_RIGHT"},
	{0x001F, 0x0003, "SS_ICON"},
	{0x001F, 0x0004, "SS_BLACKRECT"},
	{0x001F, 0x0005, "SS_GRAYRECT"},
	{0x001F, 0x0006, "SS_WHITERECT"},
	{0x001F, 0x0007, "SS_BLACKFRAME"},
	{0x001F, 0x0008, "SS_GRAYFRAME"},
	{0x001F, 0x0009, "SS_WHITEFRAME"},
	{0x001F, 0x000A, "SS_USERITEM"},
	{0x001F, 0x000B, "SS_SIMPLE"},
	{0x001F, 0x000C, "SS_LEFTNOWORDWRAP"},
	{0x001F, 0x000D, "SS_OWNERDRAW"},
	{0x001F, 0x000E, "SS_BITMAP"},
	{0x001F, 0x000F, "SS_ENHMETAFILE"},
	{0x001F, 0x0010, "SS_ETCHEDHORZ"},
	{0x001F, 0x0011, "SS_ETCHEDVERT"},
	{0x001F, 0x0012, "SS_ETCHEDFRAME"},
	{0x0040, 0x0040, "SS_REALSIZECONTROL"},
	{0x0080, 0x0080, "SS_NOPREFIX"},
	{0x0100, 0x0100, "SS_NOTIFY"},
	{0x0200, 0x0200, "SS_CENTERIMAGE"},
	{0x0400, 0x0400, "SS_RIGHTJUST"},
	{0x0800, 0x0800, "SS_REALSIZEIMAGE"},
	{0x1000, 0x1000, "SS_SUNKEN"},
	{0x2000, 0x2000, "SS_EDITCONTROL"},
	{0xC000, 0x4000, "SS_ENDELLIPSIS"},
	{0xC000, 0x8000, "SS_PATHELLIPSIS"},
	{0xC000, 0xC000, "SS_WORDELLIPSIS"},
	{0, 0, NULL}
};

static const CWindowStyle::TName ES_Names[] = {
	{0x0003, 0x0001, "ES_CENTER"},
	{0x0003, 0x0002, "ES_RIGHT"},
	{0x0004, 0x0004, "ES_MULTILINE"},
	{0x0008, 0x0008, "ES_UPPERCASE"},
	{0x0010, 0x0010, "ES_LOWERCASE"},
	{0x0020, 0x0020, "ES_PASSWORD"},
	{0x0040, 0x0040, "ES_AUTOVSCROLL"},
	{0x0080, 0x0080, "ES_AUTOHSCROLL"},
	{0x0100, 0x0100, "ES_NOHIDESEL"},
	{0x0400, 0x0400, "ES_OEMCONVERT"},
	{0x0800, 0x0800, "ES_READONLY"},
	{0x1000, 0x1000, "ES_WANTRETURN"},
	{0x2000, 0x2000, "ES_NUMBER"},
	{0, 0, NULL}
};

static const CWindowStyle::TName CBS_Names[] = {
	{0x0003, 0x0001, "CBS_SIMPLE"},
	{0x0003, 0x0002, "CBS_DROPDOWN"},
	{0x0003, 0x0003, "CBS_DROPDOWNLIST"},
	{0x0010, 0x0010, "CBS_OWNERDRAWFIXED"},
	{0x0020, 0x0020, "CBS_OWNERDRAWVARIABLE"},
	{0x0040, 0x0040, "CBS_AUTOHSCROLL"},
	{0x0080, 0x0080, "CBS_OEMCONVERT"},
	{0x0100, 0x0100, "CBS_SORT"},
	{0x0200, 0x0200, "CBS_HASSTRINGS"},
	{0x0400, 0x0400, "CBS_NOINTEGRALHEIGHT"},
	{0x0800, 0x0800, "CBS_DISABLENOSCROLL"},
	{0x2000, 0x2000, "CBS_UPPERCASE"},
	{0x4000, 0x4000, "CBS_LOWERCASE"},
	{0, 0, NULL}
};

static const CWindowStyle::TName LBS_Names[] = {
	{0x0001, 0x0001, "LBS_NOTIFY"},
	{0x0002, 0x0002, "LBS_SORT"},
	{0x0004, 0x0004, "LBS_NOREDRAW"},
	{0x0008, 0x0008, "LBS_MULTIPLESEL"},
	{0x0010, 0x0010, "LBS_OWNERDRAWFIXED"},
	{0x0020, 0x0020, "LBS_OWNERDRAWVARIABLE"},
	{0x0040, 0x0040, "LBS_HASSTRINGS"},
	{0x0080, 0x0080, "LBS_USETABSTOPS"},
	{0x0100, 0x0100, "LBS_NOINTEGRALHEIGHT"},
	{0x0200, 0x0200, "LBS_MULTICOLUMN"},
	{0x0400, 0x0400, "LBS_WANTKEYBOARDINPUT"},
	{0x0800, 0x0800, "LBS_EXTENDEDSEL"},
	{0x1000, 0x1000, "LBS_DISABLENOSCROLL"},
	{0x2000, 0x2000, "LBS_NODATA"},
	{0x4000, 0x4000, "LBS_NOSEL"},
	{0x8000, 0x8000, "LBS_COMBOBOX"},
	{0, 0, NULL}
};

static const CWindowStyle::TName TVS_Names[] = {
	{0x0001, 0x0001, "TVS_HASBUTTONS"},
	{0x0002, 0x0002, "TVS_HASLINES"},
	{0x0004, 0x0004, "TVS_LINESATROOT"},
	{0x0008, 0x0008, "TVS_EDITLABELS"},
	{0x0010, 0x0010, "TVS_DISABLEDRAGDROP"},
	{0x0020, 0x0020, "TVS_SHOWSELALWAYS"},
	{0x0040, 0x0040, "TVS_RTLREADING"},
	{0x0080, 0x0080, "TVS_NOTOOLTIPS"},
	{0x0100, 0x0100, "TVS_CHECKBOXES"},
	{0x0200, 0x0200, "TVS_TRACKSELECT"},
	{0x0400, 0x0400, "TVS_SINGLEEXPAND"},
	{0x0800, 0x0800, "TVS_INFOTIP"},
	{0x1000, 0x1000, "TVS_FULLROWSELECT"},
	{0x2000, 0x2000, "TVS_NOSCROLL"},
	{0x4000, 0x4000, "TVS_NONEVENHEIGHT"},
	{0x8000, 0x8000, "TVS_NOHSCROLL"},
	{0, 0, NULL}
};

//the Windows class of the elements: their styles match it
const CWindowStyle::TClass CWindowStyle::Classes[] = {
	{"BUTTON",        BS_Names},
	{"CHECKBOX",      BS_Names},
	{"RADIO",         BS_Names},
	{"1CGROUPBOX",    BS_Names},
	{"PICTURE",       BS_Names},
	{"STATIC",        SS_Names},
	{"1CEDIT",        ES_Names},
	{"BMASKED",       ES_Names},
	{"COMBOBOX",      CBS_Names},
	{"LISTBOX",       LBS_Names},
	{"SysTreeView32", TVS_Names},
	{NULL, NULL}
};
const CWindowStyle::TName* CWindowStyle::WindowNames = WS_Names;

char* CFormControlBase::BasicFlagList[] = {
	"Flag0",						//0
	"Flag1",						//1
	"ПропускатьПриВводе",			//2
	"Flag3",						//3
	"ИспользоватьОписание",			//4
	"Педалька",						//5 // нужно назвать нормально. кнопка выбора. Но не сейчас.
	"ШрифтПоУмолчанию",				//6
	"ЦветТекстаАвто",				//7
	"РастянутьКартинку",			//8
	"НетАвтоВыбора",				//9
	"КартинкаПоЦентру",				//10
	"КартинкаПропорционально",		//11
	"Многострочное",				//12
	"Невидимо",						//13
	"Недоступно",					//14
	"Flag15",						//15
	"ПрозрачныйФон",				//16
	"ОбычнаяРамка",					//17
	"ВыпуклаяРамка",				//18
	"ВогнутаяРамка",				//19
	"АвтоВыбор",					//20
	"Flag21",						//21
	"СНовойСтроки",					//22
	"ВТойЖеКолонке",				//23
	"НеСохранять",					//24
	"СПометками",					//25
	"ОтрицательноеКрасным",			//26
	"ПоказыватьПиктограммы",		//27
	"ЗапретитьРедактирование",		//28
	"Flag29",						//29
	"Flag30",						//30
	"Flag31"						//31
};

char* CFormControlBase::ValueFlagList[] = {
	"Неотрицательный",
	"РазделятьТриады",
	"Bit2", //на всякий случай добавим битов
	"Bit3",
	"Bit4",
	"Bit5",
	"Bit6",
	"Bit7"
};

//===============================================================================
//======  CFormControl  =========================================================
//===============================================================================
CFormControl* CFormControl::DefVal = new CFormControl;
CObjectProperty CFormControl::Props[] = {
	{"Класс",          1, CObjectProperty::t_string, VAR_OFFSET(DefVal, Class)},
	{"Идентификатор", 12, CObjectProperty::t_string, VAR_OFFSET(DefVal, Identifier)},
	{"Заголовок",      0, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, Caption)},
	{"ИдентификаторМетаданных", 13, CObjectProperty::t_prop, VAR_OFFSET(DefVal, ObjectID)},
	{"Тип",           14, CObjectProperty::t_subobject, VAR_OFFSET(DefVal, Type)},
	{"ФлагиЗначения", 18, CObjectProperty::t_prop, VAR_OFFSET(DefVal, ValueFlags)},
	{"ДопФлаги",       2, CObjectProperty::t_prop, VAR_OFFSET(DefVal, Flags)},
	{"X",              3, CObjectProperty::t_int, VAR_OFFSET(DefVal, PosX)},
	{"Y",              4, CObjectProperty::t_int, VAR_OFFSET(DefVal, PosY)},
	{"Ширина",         5, CObjectProperty::t_int, VAR_OFFSET(DefVal, Width)},
	{"Высота",         6, CObjectProperty::t_int, VAR_OFFSET(DefVal, Height)},
	{"Атрибут",        7, CObjectProperty::t_bool, VAR_OFFSET(DefVal, IsObjectAttribute)},
	{"Param9",         8, CObjectProperty::t_int, VAR_OFFSET(DefVal, Param9)},
	{"ПорядокОбхода",  9,  CObjectProperty::t_int, VAR_OFFSET(DefVal, TabOrder)},
	{"Param11",       10, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param11)},
	{"Формула",       11, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, Formula)},
	{"ОснФлаги",      19, CObjectProperty::t_prop, VAR_OFFSET(DefVal, BasicFlags)},
	{"Маска",         20, CObjectProperty::t_string, VAR_OFFSET(DefVal, Mask)},
	{"Hint",          21, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, LongHint)},
	{"ShortHint",     22, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, ShortHint)},
	{"ФормаВыбора",   23, CObjectProperty::t_prop, VAR_OFFSET(DefVal, ChoiceForm)},
	{"Шрифт",         24, CObjectProperty::t_subobject, VAR_OFFSET(DefVal, Font)},
	{"ЦветТекста",    38, CObjectProperty::t_int, VAR_OFFSET(DefVal, FontColor)},
	{"Param40",       39, CObjectProperty::t_int, VAR_OFFSET(DefVal, Param40)},
	{"Пиктограмма",   40, CObjectProperty::t_uint, VAR_OFFSET(DefVal, PictureID)},
	{"Слой",          41, CObjectProperty::t_string, VAR_OFFSET(DefVal, Layer)},
	{"HotKey",        42, CObjectProperty::t_object, VAR_OFFSET(DefVal, HotKey)},
	{NULL, 0, CObjectProperty::t_int, 0}
};
CPropertySet CFormControl::PropSet(Props, DefVal);

//===============================================================================
//======  CFormColumn  ==========================================================
//===============================================================================
CFormColumn* CFormColumn::DefVal = new CFormColumn;
CObjectProperty CFormColumn::Props[] = {
	{"Класс",           3, CObjectProperty::t_string, VAR_OFFSET(DefVal, Class)},
	{"Идентификатор",   7, CObjectProperty::t_string, VAR_OFFSET(DefVal, Identifier)},
	{"Заголовок",       1, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, Caption)},
	{"ИдентификаторМетаданных", 8,  CObjectProperty::t_prop, VAR_OFFSET(DefVal, ObjectID)},
	{"Тип",             9, CObjectProperty::t_subobject, VAR_OFFSET(DefVal, Type)},
	{"ФлагиЗначения",  13, CObjectProperty::t_prop, VAR_OFFSET(DefVal, ValueFlags)},
	{"ТипКолонки",      0, CObjectProperty::t_int, VAR_OFFSET(DefVal, ColumnType)},
	{"Ширина",          2, CObjectProperty::t_int, VAR_OFFSET(DefVal, Width)},
	{"ПорядокОбхода",   4, CObjectProperty::t_int, VAR_OFFSET(DefVal, TabOrder)},
	{"Param6",          5, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param6)},
	{"Формула",         6, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, Formula)},
	{"Param15",        14, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param15)},
	{"Param16",        15, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param16)},
	{"Выравнивание",   16, CObjectProperty::t_int, VAR_OFFSET(DefVal, TextAlignment)},
	{"ОснФлаги",       17, CObjectProperty::t_prop, VAR_OFFSET(DefVal, BasicFlags)},
	{"Маска",          18, CObjectProperty::t_string, VAR_OFFSET(DefVal, Mask)},
	{"Hint",           19, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, LongHint)},
	{"ShortHint",      20, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, ShortHint)},
	{"Param22",        21, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param22)},
	{NULL, 0, CObjectProperty::t_int, 0}
};
CPropertySet CFormColumn::PropSet(Props, DefVal);


//===============================================================================
//======  CFormBrowser  =========================================================
//===============================================================================
CFormBrowser* CFormBrowser::DefVal = new CFormBrowser;
CObjectProperty CFormBrowser::Props[] = {
	//{"sID",         0, CObjectProperty::t_string,  VAR_OFFSET(DefVal, sID)},
	{"Param1",      0, CObjectProperty::t_int,  VAR_OFFSET(DefVal, Param1)},
	{"Param2",      1, CObjectProperty::t_int,  VAR_OFFSET(DefVal, Param2)},
	{"Multicolumn", 2, CObjectProperty::t_object,  VAR_OFFSET(DefVal, Multicolumn)},
	{"Fixed",       3, CObjectProperty::t_object,  VAR_OFFSET(DefVal, Fixed)},
	{NULL, 0, CObjectProperty::t_int, 0}
};
CPropertySet CFormBrowser::PropSet(Props, DefVal);



char* CDialogFrameContent::FlagList[] = {
	"FormFlag0",				//0
	"FormFlag1",				//1
	"FormFlag2",				//2
	"FormFlag3",				//3
	"FormFlag4",				//4
	"FormFlag5",				//5
	"FormFlag6",				//6
	"ЦветАвто",					//7
	"РастянутьКартинку",		//8
	"FormFlag9",				//9
	"КартинкаПоЦентру",			//10
	"КартинкаПропорционально",	//11
	"FormFlag12",				//12
	"FormFlag13",				//13
	"FormFlag14",				//14
	"FormFlag15",				//15
	"FormFlag16",				//16
	"FormFlag17",				//17
	"FormFlag18",				//18
	"FormFlag19",				//19
	"НеСохранятьНастройки",		//20
	"FormFlag21",				//21
	"FormFlag22",				//22
	"FormFlag23",				//23
	"FormFlag24",				//24
	"FormFlag25",				//25
	"FormFlag26",				//26
	"FormFlag27",				//27
	"FormFlag28",				//28
	"FormFlag29",				//29
	"FormFlag30",				//30
	"FormFlag31"				//31
};

//===============================================================================
//======  CFormLayer  ===========================================================
//===============================================================================
CFormLayer* CFormLayer::DefVal = new CFormLayer;
CObjectProperty CFormLayer::Props[] = {
	{"sID",       0, CObjectProperty::t_string,  VAR_OFFSET(DefVal, sID)},
	{"Видимость", 1, CObjectProperty::t_prop,  VAR_OFFSET(DefVal, IsVisible)},
	{NULL, 0, CObjectProperty::t_int, 0}
};
CPropertySet CFormLayer::PropSet(Props, DefVal);

//===============================================================================
//======  CDialogFrameContent  ==================================================
//===============================================================================
CDialogFrameContent* CDialogFrameContent::DefVal = new CDialogFrameContent;
CObjectProperty CDialogFrameContent::Props[] = {
	{"Шрифт",               0, CObjectProperty::t_subobject,  VAR_OFFSET(DefVal, Font)},
	{"Ширина",              14, CObjectProperty::t_int,    VAR_OFFSET(DefVal, Width)},
	{"Высота",              15, CObjectProperty::t_int,    VAR_OFFSET(DefVal, Height)},
	{"Заголовок",           16, CObjectProperty::t_quotstring, VAR_OFFSET(DefVal, Caption)},
	{"Param18",             17, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param18)},
	{"Param19",             18, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param19)},
	{"Флаги",               19, CObjectProperty::t_prop,   VAR_OFFSET(DefVal, Flags1)},
	{"Param21",             20, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param21)},
	{"ШрифтПоУмолчанию",    21, CObjectProperty::t_bool,   VAR_OFFSET(DefVal, FontDefault)},
	{"АвтоПорядокОбхода",   22, CObjectProperty::t_bool,   VAR_OFFSET(DefVal, AutoTabOrder)},
	{"Param24",             23, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param24)},
	{"Param25",             24, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param25)},
	{"ЦветФона",            25, CObjectProperty::t_int,    VAR_OFFSET(DefVal, BackgroundColor)},
	{"ИдентификаторКартинки", 26, CObjectProperty::t_uint, VAR_OFFSET(DefVal, PictureID)},
	{"Флаги2",              27, CObjectProperty::t_prop,   VAR_OFFSET(DefVal, Flags2)},
	{"Слои",                28, CObjectProperty::t_object, VAR_OFFSET(DefVal, Layers)},
	{"ПанельИнструментов",  29, CObjectProperty::t_bool,   VAR_OFFSET(DefVal, ShowPanel)},
	{"ИзменятьРазмер",      30, CObjectProperty::t_bool,   VAR_OFFSET(DefVal, AllowResize)},
	{NULL, 0, CObjectProperty::t_int, 0}
};
CPropertySet CDialogFrameContent::PropSet(Props, DefVal);
