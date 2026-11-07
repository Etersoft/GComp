
#include "Declarations.h"
#include "DLL/GComp.h"
#include "CommonFunctions.h"
#include "DialogForm.h"
#include "MMS.h"
#include "NameTable.h"
#include "Storage.h"
#include "Strings.h"


/* нелепая таблица размеров шрифта в 1С
0    =   0
1    =  -1
1.5  =  -2
2.5  =  -3
3    =  -4
4    =  -5
4.5  =  -6
5.5  =  -7
6    =  -8
7    =  -9
7.5  = -10
8.5  = -11
9    = -12
10   = -13
10.5 = -14
11.5 = -15
12   = -16

  Таблица подобрана опытным путем, мы надеемся, что выявленная закономерность
  будет соблюдаться и при больших размерах шрифтов, но гарантии не даем.   */
void CFontSize::SetValue(GCString& NewVal)
{
	if( NewVal.Find('-') >= 0 )
	{
		int size = atoi(NewVal);
		int halves = -size / 2;
		int ones = halves + (-size % 2);
		
		Value = ones + (0.5 * halves);
	}
	else
	{
		Value = atof(NewVal);
	}
}

GCString CFontSize::BeautyString()
{
	return GCString(Value);
}

GCString CFontSize::MMSString()
{
	return GCString(-(int)((Value * 4.0) / 3.0)); // Потеря данных мне и нужна
}

/////////////////////////////////////////////////////////////////
//////  CFormFont  //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
CFormFont::CFormFont()
{
	sID = "Font";

	FontSize = 8.5;
	Param2 = 0;
	Param3 = 0;
	Param4 = 0;
	FontBold = false;
	FontItalic = false;
	FontUnderline = false;
	Param8 = 0;
	Param9 = 0;
	Param10 = 0;
	Param11 = 0;
	Param12 = 0;
	Param13 = 0;
	FontName = "MS Sans Serif";

	FontNeeded = true;
}

CFormFont& CFormFont::operator =(CFormFont& src)
{
	FontSize = src.FontSize;
	Param2 = src.Param2;
	Param3 = src.Param3;
	Param4 = src.Param4;
	FontBold = src.FontBold;
	FontItalic = src.FontItalic;
	FontUnderline = src.FontUnderline;
	Param8 = src.Param8;
	Param9 = src.Param9;
	Param10 = src.Param10;
	Param11 = src.Param11;
	Param12 = src.Param12;
	Param13 = src.Param13;
	FontName = src.FontName;
	return *this;
}

/* A font may be left out of the description only if compiling without it
   gives the same bytes: an absent font becomes CFormFont() (MS Sans Serif
   8.5, charset 0, weight 400), whatever the default-font flag says. */
bool CFormFont::IsDefault()
{
	CFormFont def;
	if( !FontBold.IsNeeded() ) def.FontBold.UnNeeded();
	CMMSString a, b;
	ToString(a);
	def.ToString(b);
	return a == b;
}

bool CFormFont::SameAs(CFormFont& Font)
{
	CMMSString a, b;
	ToString(a);
	Font.ToString(b);
	return a == b;
}

void CFormFont::SetElementDefault(bool DefaultFont)
{
	CFormFont def;
	if( DefaultFont )
	{
		def.Param9 = 204; //RUSSIAN_CHARSET
		def.Param10 = 1;  //OUT_STRING_PRECIS
		def.Param11 = 2;  //CLIP_STROKE_PRECIS
		def.Param12 = 1;  //DRAFT_QUALITY
		def.Param13 = 34; //VARIABLE_PITCH | FF_SWISS
	}
	else
	{
		GCString Weight("0");
		def.FontBold.SetValue(Weight);
	}
	*this = def;
}

void CFormFont::BeautyString(CBeautyString& String, int level)
{
	if( !FontNeeded )
		return;

	String.add_parameter(level, BeautyName(), FontName, "", true);
	String.add_brace(level, '{');
	
	level++;

	//the name is already in the header
	for( int i = 1; i < PropSet.nProps; i++ )
	{
		PropSet.BeautyString(String, level, this, i);
	}

	String.add_brace(level-1, '}');
}

void CFormFont::ToString(CMMSString& String)
{
	CStoredObject::ToString(String);
}

/////////////////////////////////////////////////////////////////
//////  CFormLayers  ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
bool CFormLayers::SetValue(int MMS_Index, GCString& Value)
{
	if( MMS_Index == 0 )
	{
		ActiveLayer = atoi(Value);
		return true;
	}
	else
	{
		return CMMSAttributes::SetValue(MMS_Index, Value);
	}
}

void CFormLayers::BeautyString(CBeautyString& String, int level)
{
	String.add_offset(level);
	String += "Слои: \r\n";
	String.add_brace(level, '{');

	for( int i = 0; i < GetNChildren(); i++ )
	{
		CFormLayer& Layer = *(CFormLayer*)GetChild(i);
		char* descr = "";
		if( Layer.IsVisible && i == ActiveLayer )
			descr = "Видимый, Активный";
		else if( Layer.IsVisible )
			descr = "Видимый";
		String.add_parameter(level+1, Layer.sID, descr, "", true);
	}

	String.add_brace(level, '}');
}

void CFormLayers::ToString(CMMSString& String)
{
	String += "\r\n{";
	String.add_quoted(ActiveLayer, false);
	for( int i = 0; i < GetNChildren(); i++ )
	{
		String += ",";
		GetChild(i)->ToString(String);
	}
	String += "}";
}



/////////////////////////////////////////////////////////////////
//////  CFormControlBase  ///////////////////////////////////////
/////////////////////////////////////////////////////////////////
void CFormControlBase::BeautyString(CBeautyString& String, int level)
{
	String.add_parameter(level, Class, Identifier, NULL, true);
	String.add_brace(level, '{');


	CPropertySet* pPropSet = GetPropSet();
	if( pPropSet != NULL )
	{
		for( int i = 2; i < pPropSet->nProps; i++ ) //пропускаем Class и Identifier
			pPropSet->BeautyString(String, level+1, this, i);
	}
	BeautyFieldCount(String, level+1);
	
	String.add_brace(level, '}');
}


/* GComp 2.2.15 replaced the stored attribute reference and type of a form
   element with those of the owner's attribute of the same name. 1C keeps
   its own copy in the form (it may be stale), and a decompiled form must
   compile back to the same bytes, so the stored values are kept. */
void CFormControlBase::CorrectObjectID(CMMSObject* ParentObject)
{
	return;
	if( ParentObject == NULL ) return;
	if( ObjectID.iTypeObjectID <= 0 ) return;

	//Попробуем найти поле по имени, в объекте-владельце формы
	CMMSObject* field = ParentObject->FindChild(Identifier, true);
	if( field != NULL )
	{
		if( ObjectID.iTypeObjectID != field->ID )
		{
			ObjectID.iTypeObjectID = field->ID;
			ObjectID.sTypeObjectID = ObjectID.iTypeObjectID;
		}
		Type = ((CMMSObjectAttribute*)field)->Type; //тип тоже возьмём из параметров поля
	}
}

/////////////////////////////////////////////////////////////////
//////  CFormControls  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////
bool CHotKey::SetValue(int MMS_Index, GCString& Value)
{
	char* ptr = (char*)(LPCSTR)(Value) + 3;

	Modifier = strtol(ptr, &ptr, 10);
	KeyCode  = strtol(ptr+5, &ptr, 10);
	return true;
}

bool CHotKey::SetValue(GCString& Name, GCString& Value)
{
	if( Value.Left(3) == "{\"\"" && Value.Right(3) == "\"\"}" )
		return SetValue(-1, Value);

	GCString mod, key;

	mod = Value.SpanExcluding("+-");
	if( mod.GetLength() == Value.GetLength() )
	{
		key = mod;
		mod = "";
	}
	else
		key = Value.Mid(mod.GetLength() + 1);

	Modifier = atoi(mod);
	KeyCode = atoi(key);

	mod.MakeUpper();
	if( Modifier == 0 && !mod.IsEmpty() )
	{
		if( strstr(mod, "SHIFT") != NULL ) Modifier |= 0x07;
		if( strstr(mod, "CTRL")  != NULL ) Modifier |= 0x0b;
		if( strstr(mod, "ALT")   != NULL ) Modifier |= 0x13;
	}
	
	if( KeyCode == 0 && !key.IsEmpty() )
	{
		if( key.GetLength() == 1 )
		{
			key.MakeUpper();
			KeyCode = (unsigned char)(key[0]);
		}
		else
		{
			int i;
			for( i = 0; virtual_keys[i].code != 0; i++ )
			{
				if( stricmp(virtual_keys[i].name, key) != 0 ) continue;
				KeyCode = virtual_keys[i].code;
				i = -1;
				break;
			}
			if( i > 0 )
			{
				Msg(0, "ERR: Unknown key '%s'", (LPCSTR)key);
				throw NULL;
				//return false;
			}
		}
	}

	return true;
}

void CHotKey::BeautyString(CBeautyString& String, int level)
{
	if( Modifier == 0 && KeyCode == 0 )
		return;

	GCString str;
	/*
	  111 - Shift
	 1011 - Ctrl
	10011 - Alt
	*/
	if( Modifier == 0x03 ) str += "3";
	if( Modifier & 0x04 ) str += "Shift";
	if( Modifier & 0x08 ) str += "Ctrl";
	if( Modifier & 0x10 ) str += "Alt";
	if( !str.IsEmpty() ) str += "+";

	if( ('0' <= KeyCode && KeyCode <= '9') || ('A' <= KeyCode && KeyCode <= 'Z') )
		str += (char)KeyCode;
	else
	{
		int i;
		for( i = 0; virtual_keys[i].code != 0; i++ )
		{
			if( KeyCode != virtual_keys[i].code ) continue;
			str += virtual_keys[i].name;
			i  = -1;
			break;
		}

		if( i > 0 )
		{
			GCString strCode;
			strCode.Format("%i", KeyCode);
			str += strCode;
		}
	}

	String.add_parameter(level, "HotKey", str, "");
}

void CHotKey::ToString(CMMSString& String)
{
	GCString str;
	str.Format("{\"\"%i\"\",\"\"%i\"\"}", Modifier, KeyCode);
	String.add_quoted(str, false);
}

//==========================================================================
//========  CFormControl  ==================================================
//==========================================================================
void CFormControl::Setup()
{
	sID = "Control";

	Flags = 0;
	PosX = PosY = 0;
	Width = Height = 0;
	IsObjectAttribute = false; //control привязан к атрибуту объекта - владельца формы. 1 - привязан, 0 - нет
	Param9 = 0;
	TabOrder = 0;
	ValueFlags = 0;  //1 - неотрицательный, 2 - разделять триады. Комбинируются по OR.
	BasicFlags = 0;
	Param24 = 0;
	FontColor = -1; //-1 - default. В hex-формате: 0xBBGGRR
	Param40 = -1;
	PictureID = 0;

	Flags.SetTable(FlagList);
	BasicFlags.SetTable(BasicFlagList);
	ValueFlags.SetTable(ValueFlagList);

	FormatDefaults = false;
	Prev = NULL;
	FontSet = false;

	ToDestroyChildren = false;
}

/* Window styles that 1C gives to elements of each class (WS_CHILD |
   WS_VISIBLE and the class's own), the most frequent ones in 430
   configurations */
static const struct {
	const char* Class;
	unsigned int Flags;
} ElementFlags[] = {
	{"STATIC",        0x50000200},
	{"BUTTON",        0x5000000B},
	{"BMASKED",       0x50800000},
	{"1CEDIT",        0x50800000},
	{"1CGROUPBOX",    0x50000007},
	{"CHECKBOX",      0x50000003},
	{"RADIO",         0x50000009},
	{"COMBOBOX",      0x50A00043},
	{"LISTBOX",       0x50A00101},
	{"TABLE",         0x50A00000},
	{"SysTreeView32", 0x50A00007},
	{"PICTURE",       0x5000800B},
	{NULL, 0}
};

void CFormControl::ApplyDefaults()
{
	for( int i = 0; ElementFlags[i].Class != NULL; i++ )
		if( strcmp(Class, ElementFlags[i].Class) == 0 )
			Flags = ElementFlags[i].Flags;

	GCString NoObject("-1");
	ObjectID.SetValue(NoObject);

	if( Prev != NULL )
	{
		TabOrder = Prev->TabOrder + 1;
		Layer = Prev->Layer;
	}
	else
	{
		TabOrder = 4152;
		Layer = FirstLayer;
	}
}

void CFormControl::ApplyDefaultFont()
{
	Font.SetElementDefault(BasicFlags.BitValue(6)); //ШрифтПоУмолчанию
}

CFormControl::CFormControl()
{
	Setup();
}

CFormControl::CFormControl(const char* aClass, const char* aIdentifier)
{
	Setup();
	Class = aClass;
	Identifier = aIdentifier;
}

CMMSObject* CFormControl::CreateChild(const char* Name, const char* Value)
{
	if( _stricoll(Name, "Шрифт") == 0 )
	{
		FontSet = true;
		Font.FontName = Value;
		return &Font;
	}
	return NULL;
}

bool CFormControl::SetValue(GCString& Name, GCString& Value)
{
	if( Name != "Положение" )
		return CFormControlBase::SetValue(Name, Value);

	int v[4];
	char* p = (char*)(LPCSTR)Value;
	for( int i = 0; i < 4; i++ )
	{
		char* end;
		while( *p == ' ' || *p == '\t' || (i > 0 && *p == ',') ) p++;
		v[i] = strtol(p, &end, 10);
		if( end == p ) BadPosition(Value);
		p = end;
		while( *p == ' ' || *p == '\t' ) p++;
		if( i < 3 && *p != ',' ) BadPosition(Value);
	}
	if( *p != '\0' ) BadPosition(Value);

	PosX = v[0];
	PosY = v[1];
	Width = v[2];
	Height = v[3];
	return true;
}

void CFormControl::BadPosition(GCString& Value)
{
	Msg(0, "ERR: '%s' - не 'X, Y, Ширина, Высота'", (LPCSTR)Value);
	throw NULL;
}

void CFormControl::BeautyString(CBeautyString& String, int level)
{
	//a type may be left out only if compiling without it gives the same one
	Type.NeedType = NeedType() || !Type.IsDefault();

	if( !FormatDefaults || TaskParameters.NoDefaults )
	{
		Font.FontNeeded = NeedFont();
		CFormControlBase::BeautyString(String, level);
		return;
	}

	CFormControl Def(Class, "");
	Def.Prev = Prev;
	Def.FirstLayer = FirstLayer;
	Def.ApplyDefaults();
	Def.Font.SetElementDefault(BasicFlags.BitValue(6)); //see ApplyDefaultFont
	Font.FontNeeded = !Font.SameAs(Def.Font);

	String.add_parameter(level, Class, Identifier, NULL, true);
	String.add_brace(level, '{');
	CPropertySet& PropSet = *GetPropSet();
	for( int i = 2; i < PropSet.nProps; i++ ) //пропускаем Class и Identifier
	{
		void* prop = PropSet.GetObjProp(this, i);
		if( prop == &PosY || prop == &Width || prop == &Height )
			continue;
		if( prop == &PosX )
		{
			GCString Position;
			Position.Format("%i, %i, %i, %i", PosX, PosY, Width, Height);
			String.add_parameter(level+1, "Положение", Position, "", true);
			continue;
		}
		//0 differs from the default -1 and would be written as an empty value
		if( prop == &ObjectID && ObjectID.iTypeObjectID == 0 )
		{
			String.add_parameter(level+1, PropSet[i].Name, "0", "", true);
			continue;
		}
		PropSet.BeautyString(String, level+1, this, i, &Def);
	}
	BeautyFieldCount(String, level+1);
	String.add_brace(level, '}');
}

void CFormControl::ToString(CMMSString& String)
{
	//Font.FontNeeded = NeedFont();
	CFormControlBase::ToString(String);
}

bool CFormControl::NeedFont()
{
	if( TaskParameters.NoDefaults ) return true;

	//the default-font flag (BasicFlags bit 6) does not mean that the stored
	//font is the one a compilation would write
	return !Font.IsDefault();
}
bool CFormControl::NeedType()
{
	if( TaskParameters.NoDefaults ) return true;

	if( strcmp(Class, "STATIC") == 0 ) return false;
	if( strcmp(Class, "BUTTON") == 0 ) return false;
	if( strcmp(Class, "COMBOBOX") == 0 ) return false;
	if( strcmp(Class, "LISTBOX") == 0 ) return false;
	if( strcmp(Class, "TABLE") == 0 ) return false;
	if( strcmp(Class, "PICTURE") == 0 ) return false;
	if( strcmp(Class, "1CGROUPBOX") == 0 ) return false;
	return true;
}

int __cdecl CFormControl::CompareByIdentifier(const void *elem1, const void *elem2)
{
	CFormControl *c1 = *(CFormControl**)elem1, *c2 = *(CFormControl**)elem2;
	
	int Comp = c1->Class.Compare(c2->Class);
	if( Comp != 0 )
		return Comp;

	if( !(c1->Identifier.IsEmpty() && c2->Identifier.IsEmpty()) )
	{
		if( c1->Identifier.IsEmpty() ) return 1;
		if( c2->Identifier.IsEmpty() ) return -1;

		return c1->Identifier.Compare(c2->Identifier);
	}

	if( !(c1->Caption.IsEmpty() && c2->Caption.IsEmpty()) )
	{
		if( c1->Caption.IsEmpty() ) return 1;
		if( c2->Caption.IsEmpty() ) return -1;

		Comp = c1->Caption.Compare(c2->Caption);
		if( Comp != 0 )
			return Comp;
	}

	if( !(c1->Formula.IsEmpty() && c2->Formula.IsEmpty()) )
	{
		if( c1->Formula.IsEmpty() ) return 1;
		if( c2->Formula.IsEmpty() ) return -1;

		Comp = c1->Formula.Compare(c2->Formula);
		if( Comp != 0 )
			return Comp;
	}

	if( c1->TabOrder < c2->TabOrder ) return -1;
	if( c1->TabOrder > c2->TabOrder ) return 1;
	return 0;
}

int __cdecl CFormControl::CompareByTabOrder(const void *elem1, const void *elem2)
{
	CFormControl *c1 = *(CFormControl**)elem1, *c2 = *(CFormControl**)elem2;
	
	if( c1->TabOrder < c2->TabOrder ) return -1;
	if( c1->TabOrder > c2->TabOrder ) return 1;
	return 0;
}

/////////////////////////////////////////////////////////////////
//////  CFormColumn  ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
void CFormColumn::Setup()
{
	ColumnType = 0;
	Width = 0;
	TabOrder = 0;
	ValueFlags = 0;  //1 - неотрицательный, 2 - разделять триады. Комбинируются по OR.
	TextAlignment = 0;
	BasicFlags = 0;

	Type.SetNumeric();
	BasicFlags.SetTable(BasicFlagList);
	ValueFlags.SetTable(ValueFlagList);
}

CFormColumn::CFormColumn()
{
	Setup();
}

CFormColumn::CFormColumn(const char* aClass, const char* aIdentifier)
{
	Setup();
	Class = aClass;
	Identifier = aIdentifier;
}

CFormColumns::CFormColumns()
{
	Parent = NULL;
}

void CFormColumns::BeautyString(CBeautyString& String, int level)
{
	for( int i = 0; i < GetNChildren(); i++ )
		((CFormControlBase*)GetChild(i))->CorrectObjectID(Parent);

	CMMSAttributes::BeautyString(String, level);
}

/////////////////////////////////////////////////////////////////
//////  CFormControls  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////
CFormControls::CFormControls()
{
	sID = "Controls";
	Parent = NULL;
	Layers = NULL;
}

bool CFormControls::UseFormatDefaults()
{
	return !TaskParameters.NoDefaults;
}

GCString CFormControls::FirstLayer()
{
	if( Layers == NULL || Layers->GetNChildren() == 0 )
		return GCString("");
	return Layers->GetChild(0)->sID;
}

CMMSObject* CFormControls::CreateChild(const char* Name, const char* Value)
{
	CFormControl* Control = new CFormControl(Name, Value);
	{
		int n = GetNChildren();
		Control->Prev = n > 0 ? (CFormControl*)GetChild(n - 1) : NULL;
		Control->FirstLayer = FirstLayer();
		Control->FormatDefaults = true;
		Control->ApplyDefaults();
	}
	return Control;
}

//the default font depends on ОснФлаги, which may follow it in the description
//(FormatDefaults is set only for elements read from a description)
CMMSObject* CFormControls::AddChild(CMMSObject* Child)
{
	CFormControl* Control = (CFormControl*)Child;
	if( Control->FormatDefaults && !Control->FontSet )
		Control->ApplyDefaultFont();
	return CMMSAttributes::AddChild(Child);
}

void CFormControls::BeautyString(CBeautyString& String, int level)
{
	if( TaskParameters.SortFormControls )
	{
		qsort(Children.GetData(), Children.GetSize(), sizeof(CMMSObject*), 
			CFormControl::CompareByIdentifier);
	}

	for( int i = 0; i < GetNChildren(); i++ )
	{
		CFormControl* Control = (CFormControl*)GetChild(i);
		Control->CorrectObjectID(Parent);
		Control->FormatDefaults = UseFormatDefaults();
		Control->Prev = i > 0 ? (CFormControl*)GetChild(i - 1) : NULL;
		Control->FirstLayer = FirstLayer();
	}

	CMMSAttributes::BeautyString(String, level);
}

void CFormControls::ToString(CMMSString& String)
{
	if( TaskParameters.SortFormControls )
	{
		qsort(Children.GetData(), Children.GetSize(), sizeof(CMMSObject*), 
			CFormControl::CompareByTabOrder);
	}

	CMMSAttributes::ToString(String);
}

/////////////////////////////////////////////////////////////////
//////  CFormBrowser  ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////
CMMSObject* CFormMulticolumn::CreateChild(const char* Name, const char* Value)
{
	CFormControl* control = new CFormControl(Name, Value);
	control->Font.FontBold.UnNeeded();
	return control;
}

void CFormMulticolumn::BeautyString(CBeautyString& String, int level)
{
	if( GetNChildren() > 0 )
	{
		CFormControl* browser = (CFormControl*)GetChild(0);
		//У параметров многострочной части не может быть типа. Откуда он берётся в 1С - непонятно
		browser->Type.NeedType = !browser->Type.IsDefault();
		//1C does store an identifier and a field reference here; GComp 2.2.15
		//cleared them, which changed the form on compilation
	}
	CMMSAttributes::BeautyString(String, level);
}

CFormBrowser::CFormBrowser()
{
	Param1 = 0;
	Param2 = 0;
	ToDestroyChildren = false;
}

bool CFormBrowser::SetValue(int MMS_Index, GCString& Value)
{
	return CStoredObject::SetValue(MMS_Index, Value);
}

CMMSObject* CFormBrowser::CreateChild(const char* Name, const char* Value)
{
	if( _stricoll(Name, Multicolumn.BeautyName()) == 0 )
		return &Multicolumn;
	if( _stricoll(Name, Fixed.BeautyName()) == 0 )
		return &Fixed;
	return NULL;
}

CMMSObject* CFormBrowser::CreateChild(const char* Name, int MMS_Index)
{
	switch( MMS_Index )
	{
	case 2:
		return &Multicolumn;
	case 3:
		return &Fixed;
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////
//////  CDialogFrame  ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////
CDialogFrameContent::CDialogFrameContent()
{
	Width = Height = 0;
	Flags1 = 0;
	FontDefault = true;
	AutoTabOrder = true;
	BackgroundColor = -1; //-1 - default. В hex-формате: 0xBBGGRR
	PictureID = 0;
	Flags2 = 0; //дублирует Flags1
	ShowPanel = true;
	AllowResize = true;

	Flags1.SetTable(FlagList);
	Flags2.SetTable(FlagList);
	Flags2Set = false;

	ToDestroyChildren = false;
}

CMMSObject* CDialogFrameContent::CreateChild(const char* Name, const char* Value)
{
	if( _stricoll(Name, Layers.BeautyName()) == 0 )
		return &Layers;
	if( _stricoll(Name, Font.BeautyName()) == 0 )
	{
		//the block lists what differs from CFormFont(), not from the default
		CFormFont Standard;
		Font = Standard;
		Font.FontName = Value; //format 3 has the name only in the header
		return &Font;
	}
	return NULL;
}

CMMSObject* CDialogFrameContent::CreateChild(const char* Name, int MMS_Index)
{
	switch( MMS_Index )
	{
		case 28: return &Layers;
	}
	return NULL;
}

void CDialogFrameContent::ApplyDefaults()
{
	Font.SetElementDefault(true);
	Param24 = "6";
	Param25 = "25";
}

void CDialogFrameContent::ParseFile(CLexer& Lexer)
{
	Flags2Set = false;
	ApplyDefaults();

	CMMSObject::ParseFile(Lexer);

	if( !Flags2Set )
		Flags2 = Flags1;
}

bool CDialogFrameContent::SetValue(GCString& Name, GCString& Value)
{
	if( Name == "Флаги2" )
		Flags2Set = true;
	return CMMSObject::SetValue(Name, Value);
}

void CDialogFrameContent::BeautyString(CBeautyString& String, int level)
{
	String.add_parameter(level, BeautyName(), "", "", true);
	String.add_brace(level, '{');

	if( !TaskParameters.NoDefaults )
	{
		CDialogFrameContent Def;
		Def.ApplyDefaults();
		Def.Flags2 = Flags1;
		Font.FontNeeded = !Font.SameAs(Def.Font);
		for( int i = 0; i < PropSet.nProps; i++ )
			PropSet.BeautyString(String, level+1, this, i, &Def);
	}
	else
	{
		if( !TaskParameters.NoDefaults && Font.IsDefault() )
			Font.FontNeeded = false;
		CMMSObject::BeautyString(String, level+1);
	}

	BeautyFieldCount(String, level+1);
	String.add_brace(level, '}');
}

CDialogFrame::CDialogFrame()
{
	ToDestroyChildren = false;
}

CMMSObject* CDialogFrame::CreateChild(const char* Name, int MMS_Index)
{
	return &Content;
}

//written whenever the form has it: a compilation adds it only if described
void CFrameBraces::BeautyString(CBeautyString& String, int level)
{
	String.add_parameter(level, BeautyName(), "", "", true);
	String.add_brace(level, '{');
	String.add_brace(level, '}');
}

void CFrameBraces::ToString(CMMSString& String)
{
	if( Compact )
		String += "\r\n{{{}},\r\n{}}";
	else
		String += "\r\n{\r\n{\r\n{}},\r\n{}}";
}

/////////////////////////////////////////////////////////////////
//////  CDialogForm  ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
CDialogForm::CDialogForm(CMMSObject* aParent)
{
	//Cnt_Ver = "10001";
	HadCntVer = true;
	CompactBraces = false;
	TrailingSpaces = true;
	Browser.Fixed.Parent = aParent;
	Fixed.Parent = aParent;
	Controls.Parent = aParent;
	Controls.Layers = &Frame.Content.Layers;
	ToDestroyChildren = false;
}

CMMSObject* CDialogForm::CreateChild(const char* Name, int MMS_Index)
{
	if( MMS_Index == 2 && Name[0] == '\0' )
		return &Braces;
	if( strcmp(Name, Frame.MMSName()) == 0 )
		return &Frame;
	if( strcmp(Name, Browser.MMSName()) == 0 )
		return &Browser;
	if( strcmp(Name, "Fixed") == 0 )
		return &Fixed;
	if( strcmp(Name, "Controls") == 0 )
		return &Controls;
	if( strcmp(Name, "Cnt_Ver") == 0 )
	{
		HadCntVer = true;
		return &Cnt_Ver;
	}

	return NULL;
}

CMMSObject* CDialogForm::CreateChild(const char* Name, const char* Value)
{
	if( _stricoll(Name, Frame.BeautyName()) == 0 )
		return &Frame;
	if( _stricoll(Name, Braces.BeautyName()) == 0 )
		return &Braces;
	if( _stricoll(Name, Browser.BeautyName()) == 0 )
		return &Browser;
	if( _stricoll(Name, Fixed.BeautyName()) == 0 )
		return &Fixed;
	if( _stricoll(Name, Controls.BeautyName()) == 0 )
		return &Controls;
	if( strcmp(Name, "Cnt_Ver") == 0 )
		return &Cnt_Ver;

	return NULL;
}


void CDialogForm::ToString(CMMSString& String)
{
	CFormControl::SetDefaulFont(Frame.Content.Font);

	if( FindChild(Controls.sID) == NULL )
		AddChild(&Controls);

	if( HadCntVer && FindChild(Cnt_Ver.sID) == NULL )
	{
		GCString CntVerValue("10001");
		Cnt_Ver.SetValue(-1, CntVerValue);
		AddChild(&Cnt_Ver);
	}

	Braces.Compact = CompactBraces;
	CMMSAttributes::ToString(String);

	if( TrailingSpaces )
		String += "  ";//В 1С накосячили, а мы повторяем
}

//Formatting variants seen in 1C forms; the defaults are those GComp always wrote
void CDialogForm::DetectFormatting(const GCString& Raw)
{
	CompactBraces = Raw.Find("\r\n{{{}},\r\n{}}") >= 0;
	TrailingSpaces = Raw.Right(2) == "  ";
}

bool CDialogForm::SetValue(GCString& Name, GCString& Value)
{
	if( Name == "Cnt_Ver" )
	{
		if( !IsBoolString(Value) ) return false;
		HadCntVer = StrToBool(Value);
		return true;
	}
	if( Name == "СкобкиКомпактно" )
	{
		if( !IsBoolString(Value) ) return false;
		CompactBraces = StrToBool(Value);
		return true;
	}
	if( Name == "ПробелыВКонце" )
	{
		if( !IsBoolString(Value) ) return false;
		TrailingSpaces = StrToBool(Value);
		return true;
	}
	return CStoredObject::SetValue(Name, Value);
}

//Most forms have Cnt_Ver 10001, which a compilation adds; mark the others
void CDialogForm::BeautyString(CBeautyString& String, int level)
{
	CBeautyString str;
	CMMSAttributes::BeautyString(str, level);
	CBeautyString mark;
	if( !HadCntVer )
		mark.add_parameter(level + 1, "Cnt_Ver", "Нет", "", true);
	if( CompactBraces )
		mark.add_parameter(level + 1, "СкобкиКомпактно", "Да", "", true);
	if( !TrailingSpaces )
		mark.add_parameter(level + 1, "ПробелыВКонце", "Нет", "", true);
	if( !mark.IsEmpty() )
	{
		int close = str.ReverseFind('}');
		int line = str.Left(close).ReverseFind('\n') + 1;
		CString head = str.Left(line), tail = str.Mid(line);
		str = head;
		str += mark;
		str += tail;
	}
	String += str;
}

void CDialogForm::WriteToFile(CStorage& Storage, GCString FileName)
{
	CBeautyString String;
	BeautyString(String, 0);
	Storage.CopyStringToFile(String, FileName);
}

