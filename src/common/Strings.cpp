
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

static bool NeedsQuotes(const GCString& v)
{
	int len = v.GetLength();
	if( len == 0 ) return false;
	if( isspace((unsigned char)v[0]) || isspace((unsigned char)v[len-1]) ) return true;
	if( v.Find('\n') >= 0 || v.Find('\r') >= 0 ) return true;
	return len >= 2 && v[0] == '"' && v[len-1] == '"';
}

GCString QuoteEdgeSpaces(const GCString& v)
{
	if( !NeedsQuotes(v) ) return v;
	GCString q = v;
	q.quote_linefeeds();
	q.Replace("\"", "\"\"");
	return "\"" + q + "\"";
}

void UnquoteEdgeSpaces(GCString& v)
{
	int len = v.GetLength();
	if( len < 2 || v[0] != '"' || v[len-1] != '"' ) return;
	GCString s;
	for( int i = 1; i < len - 1; i++ )
	{
		char c = v[i];
		if( c == '"' && i + 1 < len - 1 && v[i+1] == '"' )
			i++;
		else if( c == '\\' && i + 1 < len - 1 && v[i+1] == '\\' )
			i++;
		else if( c == '\\' && i + 1 < len - 1 && v[i+1] == 'n' )
		{
			s += "\r\n";
			i++;
			continue;
		}
		s += c;
	}
	v = s;
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

/* CP1251 <-> UTF-8. Only 0x80-0xBF need the table: 0xC0-0xFF are U+0410-U+044F.
   Byte 0x98 has no character in CP1251; it maps to U+0098 so that every byte
   of a module survives the round trip. */
static const unsigned short Cp1251High[64] = {
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
	0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
	0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
	0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
};

void Cp1251ToUtf8(const char* src, int len, GCString& dst)
{
	if( len <= 0 )
	{
		dst.Empty();
		return;
	}

	char* out = dst.GetBufferSetLength(len * 3);
	int n = 0;
	for( int i = 0; i < len; i++ )
	{
		unsigned int c = (unsigned char)src[i];
		if( c >= 0xC0 )
			c += 0x410 - 0xC0;
		else if( c >= 0x80 )
			c = Cp1251High[c - 0x80];

		if( c < 0x80 )
			out[n++] = (char)c;
		else if( c < 0x800 )
		{
			out[n++] = (char)(0xC0 | (c >> 6));
			out[n++] = (char)(0x80 | (c & 0x3F));
		}
		else
		{
			out[n++] = (char)(0xE0 | (c >> 12));
			out[n++] = (char)(0x80 | ((c >> 6) & 0x3F));
			out[n++] = (char)(0x80 | (c & 0x3F));
		}
	}
	dst.ReleaseBuffer(n);
}

int UnicodeToCp1251(unsigned int c)
{
	if( c < 0x80 ) return (int)c;
	if( c >= 0x410 && c <= 0x44F ) return (int)(c - 0x410 + 0xC0);
	for( int i = 0; i < 64; i++ )
		if( Cp1251High[i] == c ) return 0x80 + i;
	return -1;
}

int Utf8ToCp1251(const char* src, int len, GCString& dst)
{
	if( len <= 0 )
	{
		dst.Empty();
		return -1;
	}

	char* out = dst.GetBufferSetLength(len);
	int n = 0;
	int i = 0;
	while( i < len )
	{
		unsigned int c = (unsigned char)src[i];
		int extra;
		unsigned int min;
		if( c < 0x80 )
		{
			extra = 0;
			min = 0;
		}
		else if( (c & 0xE0) == 0xC0 )
		{
			extra = 1;
			min = 0x80;
			c &= 0x1F;
		}
		else if( (c & 0xF0) == 0xE0 )
		{
			extra = 2;
			min = 0x800;
			c &= 0x0F;
		}
		else
		{
			//4-byte sequences are outside CP1251 anyway
			dst.ReleaseBuffer(n);
			return i;
		}

		if( i + extra >= len ) //truncated sequence
		{
			dst.ReleaseBuffer(n);
			return i;
		}
		for( int k = 1; k <= extra; k++ )
		{
			unsigned int b = (unsigned char)src[i + k];
			if( (b & 0xC0) != 0x80 )
			{
				dst.ReleaseBuffer(n);
				return i;
			}
			c = (c << 6) | (b & 0x3F);
		}

		int ch = (c < min) ? -1 : UnicodeToCp1251(c);
		if( ch < 0 )
		{
			dst.ReleaseBuffer(n);
			return i;
		}
		out[n++] = (char)ch;
		i += extra + 1;
	}
	dst.ReleaseBuffer(n);
	return -1;
}
