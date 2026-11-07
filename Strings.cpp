
#include "Strings.h"
#include "DLL/GComp.h"

const GCString GCString::EmptyString;

/*
int GCString::Resize(int delta)
{
	int OldDataLen = GetData()->nDataLength;
	int Len = GetLength();
	int NewLen = Len + delta;
	int AllocLen = GetData()->nAllocLength;
	if( NewLen > AllocLen )
	{
		int NewAllocLen = GetData()->nAllocLength * 2;
		if( NewAllocLen < 256 )
			NewAllocLen = 256;
		while( NewAllocLen <= NewLen )
			NewAllocLen *= 2;
		GetBufferSetLength(NewAllocLen);
	}

	GetData()->nDataLength = OldDataLen;
	return GetData()->nAllocLength;
}

const GCString& GCString::operator+=(TCHAR ch)
{
	//Resize(1);
	return CString::operator+=(ch);
}

const GCString& GCString::operator+=(LPCTSTR lpsz)
{
	//Resize(strlen(lpsz));
	return CString::operator+=(lpsz);
}

const GCString& GCString::operator+=(const GCString& str)
{
	//Resize(str.GetLength());
	return CString::operator+=(str);
}
//*/

GCString operator+(const GCString& string1, const GCString& string2)
{
	return operator+((const CString&)string1, (const CString&)string2);
};

GCString operator+(const GCString& string, const char* str)
{
	return operator+((const CString&)string, str);
}

GCString operator+(const char* str, const GCString& string)
{
	return operator+(str, (const CString&)string);
}


bool GCString::IsNumber()
{
	int iVal = atoi((LPCSTR)(*this));
	char str[32];
	return this->Compare(itoa(iVal, str, 10)) == 0;
}

void GCString::Add(const char* str, int count)
{
	while( count-- > 0 ) operator +=(str);
}



//Все символы перевода строки заменяем на "\n", и соответственно "\" -> "\\"
//Замена производится прямо в строке - строк с такой фигнёй не должно быть много, 
//поэтому inplace замена будет выгоднее.
void GCString::quote_linefeeds()
{
	Replace("\\", "\\\\");
	Replace("\r\n", "\\n");
	Replace("\n", "\\n");
}
//Обратное преобразование
void GCString::unquote_linefeeds()
{
	int len = GetLength();
	
	for( int i = 0; i < len; i++ )
	{
		if( GetAt(i) != '\\' ) continue;
		switch( GetAt(i+1) )
		{
		case 'n':
			SetAt(i++,  '\r');
			SetAt(i,  '\n');
			break;
		case '\\':
			Delete(i);
			len--;
			break;
		}
	}
}


/*////////////////////////////////////////////////////////////////////////////////
 Методы для "красивого" вывода параметров в текст. Предполагается, что файл состоит
 из строчек вида
 <ИмяПараметра>: [ЗначениеПараметра]
////////////////////////////////////////////////////////////////////////////////*/

void CBeautyString::add_offset(int level)
{
	Add("\t", level);
}

void CBeautyString::add_brace(int level, const char brace)
{
	char str[] = "{\r\n";
	Add("\t", level);
	str[0] = brace;
	operator+= (str);
}

void CBeautyString::add_parameter(int level, const char* id, const char* value, const char* default_value, bool force)
{
	if( !force )
	{
		if( value == default_value ) return;
		if( value == NULL ) return;
		if( default_value == NULL && value[0] == '\0' ) return;
		if( default_value != NULL && strcmp(value, default_value) == 0) return;
	}

	add_offset(level);
	operator+=(id);
	operator+=(": ");
	operator+=(value);
	operator+=("\r\n");
}

void CBeautyString::add_parameter(int level, const char* id, int value, int default_value, bool force)
{
	if( !force && value == default_value ) return;

	char str[64];
	sprintf(str, "%i", value);
	add_parameter(level, id, str, "");
}

void CBeautyString::add_parameter(int level, const char* id, unsigned int value, unsigned int default_value, bool force)
{
	if( !force && value == default_value ) return;

	char str[64];
	sprintf(str, "0x%x", value);
	add_parameter(level, id, str, "");
}

void CBeautyString::add_parameter(int level, const char* id, bool value, bool default_value, bool force)
{
	if( !force && value == default_value ) return;

	if( value )
		add_parameter(level, id, "Да", "");
	else
		add_parameter(level, id, "Нет", "");
}

void CBeautyString::add_parameter(int level, const char* id, double value, double default_value, bool force)
{
	if( !force && value == default_value ) return;

	char str[64];
	sprintf(str, "%g", value);
	add_parameter(level, id, str, "");
}



/*/////////////////////////////////////////////////////////////////////
	Методы для вывода в строку, в формате, который будет понимать 1С
	Каждое значение заключается в кавычки; значения отделяются запятыми
/////////////////////////////////////////////////////////////////////*/

void CMMSString::add_quoted(const char* value, bool add_comma)
{
	GCString str;

	if( add_comma )
		str = ",\"";
	else
		str = '"';
	str += value;
	str += '"';
	operator+= (str);
}

void CMMSString::add_quoted(int value, bool add_comma)
{
	char str[64];
	sprintf(str, "%i", value);
	add_quoted(str, add_comma);
}
void CMMSString::add_quoted(unsigned int value, bool add_comma)
{
	char str[64];
	sprintf(str, "%u", value);
	add_quoted(str, add_comma);
}
void CMMSString::add_quoted(bool value, bool add_comma)
{
	if( value )
		add_quoted("1", add_comma);
	else
		add_quoted("0", add_comma);
}
void CMMSString::add_quoted(double value, bool add_comma)
{
	char str[64];
	sprintf(str, "%g", value);
	add_quoted(str, add_comma);
}

//Read Pascal-like string from binary stream
bool ReadSizedString(GCString& String, IStream* pStream)
{
	BYTE len;
	char buf[256];
	ULONG BytesRead;

	pStream->Read(&len, sizeof(len), &BytesRead);
	if( BytesRead != sizeof(len) ) return false;
	pStream->Read(buf, len, &BytesRead);
	if( BytesRead != len ) return false;

	buf[len] = '\0';
	String = buf;
	return true;
}

//Write Pascal-like string into binary stream
void WriteSizedString(IStream* pStream, GCString& String)
{
	BYTE len = String.GetLength();
	pStream->Write(&len, sizeof(len), NULL);
	if( len > 0 ) pStream->Write((LPCSTR)String, len, NULL);
}

//"1", "0", "Да", "Нет", "Yes", "No" - the forms GComp reads and writes
bool IsBoolString(const char* str)
{
	return strcmp(str, "1") == 0 || strcmp(str, "0") == 0 ||
		_stricoll(str, "Да") == 0 || _stricoll(str, "Нет") == 0 ||
		_stricoll(str, "Yes") == 0 || _stricoll(str, "No") == 0;
}

bool StrToBool(const char* str)
{
	if( strcmp(str, "1") == 0 ) return true;
	if( _stricoll(str, "Да") == 0 ) return true;
	return false;
}


//Класс нужен только для того, чтобы при создании статического объекта 
//он автоматом инициализировался
class CTranslitTable
{
public:
	static CMapWordToPtr Map;
public:
	CTranslitTable();
};
CMapWordToPtr CTranslitTable::Map;

CTranslitTable::CTranslitTable()
{
	Map.SetAt('а', (void*)"a");
	Map.SetAt('б', (void*)"b");
	Map.SetAt('в', (void*)"v");
	Map.SetAt('г', (void*)"g");
	Map.SetAt('д', (void*)"d");
	Map.SetAt('е', (void*)"e");
	Map.SetAt('ё', (void*)"jo");
	Map.SetAt('ж', (void*)"zh");
	Map.SetAt('з', (void*)"z");
	Map.SetAt('и', (void*)"i");
	Map.SetAt('й', (void*)"j");
	Map.SetAt('к', (void*)"k");
	Map.SetAt('л', (void*)"l");
	Map.SetAt('м', (void*)"m");
	Map.SetAt('н', (void*)"n");
	Map.SetAt('о', (void*)"o");
	Map.SetAt('п', (void*)"p");
	Map.SetAt('р', (void*)"r");
	Map.SetAt('с', (void*)"s");
	Map.SetAt('т', (void*)"t");
	Map.SetAt('у', (void*)"u");
	Map.SetAt('ф', (void*)"f");
	Map.SetAt('х', (void*)"h");
	Map.SetAt('ц', (void*)"c");
	Map.SetAt('ч', (void*)"ch");
	Map.SetAt('ш', (void*)"sh");
	Map.SetAt('щ', (void*)"shh");
	Map.SetAt('ъ', (void*)"#");
	Map.SetAt('ы', (void*)"y");
	Map.SetAt('ь', (void*)"'");
	Map.SetAt('э', (void*)"je");
	Map.SetAt('ю', (void*)"ju");
	Map.SetAt('я', (void*)"ja");

	Map.SetAt('А', (void*)"A");
	Map.SetAt('Б', (void*)"B");
	Map.SetAt('В', (void*)"V");
	Map.SetAt('Г', (void*)"G");
	Map.SetAt('Д', (void*)"D");
	Map.SetAt('Е', (void*)"E");
	Map.SetAt('Ё', (void*)"Jo");
	Map.SetAt('Ж', (void*)"Zh");
	Map.SetAt('З', (void*)"Z");
	Map.SetAt('И', (void*)"I");
	Map.SetAt('Й', (void*)"J");
	Map.SetAt('К', (void*)"K");
	Map.SetAt('Л', (void*)"L");
	Map.SetAt('М', (void*)"M");
	Map.SetAt('Н', (void*)"N");
	Map.SetAt('О', (void*)"O");
	Map.SetAt('П', (void*)"P");
	Map.SetAt('Р', (void*)"R");
	Map.SetAt('С', (void*)"S");
	Map.SetAt('Т', (void*)"T");
	Map.SetAt('У', (void*)"U");
	Map.SetAt('Ф', (void*)"F");
	Map.SetAt('Х', (void*)"H");
	Map.SetAt('Ц', (void*)"C");
	Map.SetAt('Ч', (void*)"Ch");
	Map.SetAt('Ш', (void*)"Sh");
	Map.SetAt('Щ', (void*)"Shh");
	Map.SetAt('Ъ', (void*)"#");
	Map.SetAt('Ы', (void*)"Y");
	Map.SetAt('Ь', (void*)"'");
	Map.SetAt('Э', (void*)"Je");
	Map.SetAt('Ю', (void*)"Ju");
	Map.SetAt('Я', (void*)"Ja");
}
static CTranslitTable TranslitTable;

void InplaceTranslit(GCString& str)
{
	if ( lang_Rus == TaskParameters.Language ) return;

	GCString Result;
	for (int i=0; i < str.GetLength(); i++)
	{
		TCHAR ch = str.GetAt(i);
		void *ptr;
		if( CTranslitTable::Map.Lookup(ch, ptr) )
			Result += (LPCSTR)ptr;
		else
			Result += ch;
	}

	str = Result;
}

GCString Translit(GCString& str)
{
	GCString Result = (LPCSTR)str;
	InplaceTranslit(Result);
	return Result;
}
