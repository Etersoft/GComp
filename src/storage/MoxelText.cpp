// SPDX-License-Identifier: GPL-2.0-or-later
/* A Markdown view of a moxcel table (.mxl) for reviewing changes: the texts
   of the cells as tables by row sections, then the formatting of columns,
   rows and cells with the details and values of the cells, joins, objects,
   sections, page breaks and named areas as lists. It is written next to the
   table with --mxl-text and never read back: compiling takes the .mxl.

   The layout follows docs/moxcel.md and its "Уточнения формата (2026)":
   MFC CArchive data, CString lengths, WORD counts (FFFF + DWORD), 30-byte
   format records (32 in version 7, 28 in versions 1 and 2). */

#include <string.h>
#include <vector>
#include "Declarations.h"
#include "MoxelText.h"

namespace {

struct CMoxelError
{
	GCString What;
	CMoxelError(const GCString& w) : What(w) {}
};

class CReader
{
public:
	const unsigned char* d;
	long n, p;

	CReader(const unsigned char* data, long size) : d(data), n(size), p(0) {}

	const unsigned char* Take(long k)
	{
		if( k < 0 || p + k > n )
		{
			GCString s;
			s.Format("конец данных: нужно %ld байт по смещению %ld из %ld", k, p, n);
			throw CMoxelError(s);
		}
		const unsigned char* r = d + p;
		p += k;
		return r;
	}
	unsigned U8() { return *Take(1); }
	unsigned U16() { const unsigned char* b = Take(2); return b[0] | (b[1] << 8); }
	unsigned long U32()
	{
		const unsigned char* b = Take(4);
		return (unsigned long)b[0] | ((unsigned long)b[1] << 8) |
			((unsigned long)b[2] << 16) | ((unsigned long)b[3] << 24);
	}
	long I32() { return (long)U32(); }
	unsigned long Count()
	{
		unsigned long c = U16();
		if( c == 0xFFFF ) c = U32();
		return c;
	}
	GCString CStr()
	{
		unsigned long len = U8();
		if( len == 0xFF )
		{
			len = U16();
			if( len == 0xFFFE ) throw CMoxelError("строка Unicode");
			if( len == 0xFFFF ) len = U32();
		}
		const unsigned char* s = Take((long)len);
		return GCString((const char*)s, (int)len);
	}
};

static unsigned W16(const unsigned char* b) { return b[0] | (b[1] << 8); }
static short S16(const unsigned char* b) { return (short)W16(b); }

const unsigned long F_TEXT = 0x80000000UL, F_DESC = 0x40000000UL, F_VALUE = 0x00200000UL;

//A format record and what follows it
struct CFormat
{
	const unsigned char* Rec; //NULL for the 28-byte records of versions 1 and 2
	unsigned long Flags;
	bool HasText, HasDesc;
	GCString Text, Desc;
	const unsigned char* Value; //the value block after the length, or NULL
	long ValueLen;
};

class CMoxelText
{
public:
	CReader R;
	int Version;
	GCString Out;
	CArray<unsigned long, unsigned long> FontIDs;
	CStringArray FontNames;

	CMoxelText(const unsigned char* d, long n) : R(d, n), Version(6) {}

	void Prop(CFormat& f)
	{
		f.HasText = f.HasDesc = false;
		f.Value = NULL;
		f.ValueLen = 0;
		if( Version == 1 || Version == 2 )
		{
			const unsigned char* b = R.Take(28);
			f.Rec = NULL;
			f.Flags = b[0] | (b[1] << 8) | ((unsigned long)b[2] << 16) | ((unsigned long)b[3] << 24);
			return;
		}
		f.Rec = R.Take(30);
		f.Flags = f.Rec[0] | (f.Rec[1] << 8) | ((unsigned long)f.Rec[2] << 16) | ((unsigned long)f.Rec[3] << 24);
		if( Version == 7 ) R.Take(2);
	}

	void Ext(CFormat& f)
	{
		Prop(f);
		if( f.Flags & F_TEXT ) { f.Text = R.CStr(); f.HasText = true; }
		if( f.Flags & F_DESC ) { f.Desc = R.CStr(); f.HasDesc = true; }
		if( f.Flags & F_VALUE )
		{
			unsigned long len = R.U32();
			if( len < 2 ) throw CMoxelError("неверная длина значения ячейки");
			f.ValueLen = (long)len - 2;
			f.Value = R.Take(f.ValueLen);
		}
	}

	static GCString Escape(const GCString& s)
	{
		GCString r;
		for( int i = 0; i < s.GetLength(); i++ )
		{
			unsigned char c = (unsigned char)s[i];
			if( c == '\\' ) r += "\\\\";
			else if( c == '\r' && i + 1 < s.GetLength() && s[i+1] == '\n' ) { r += "\\n"; i++; }
			else if( c == '\n' ) r += "\\n";
			else if( c == '\r' ) r += "\\r";
			else if( c == '\t' ) r += "\\t";
			else if( c < 0x20 ) { GCString x; x.Format("\\x%02x", c); r += x; }
			else r += (char)c;
		}
		return r;
	}

	GCString FontName(unsigned id)
	{
		for( int i = 0; i < FontIDs.GetSize(); i++ )
			if( FontIDs[i] == id ) return GCString("«") + GCString(FontNames[i]) + "»";
		GCString s;
		s.Format("%u", id);
		return s;
	}

	//Border styles, by the picture of the style list in docs/moxcel.md
	static GCString Border(unsigned v)
	{
		static const char* names[] = {"Нет", "Точечная", "Тонкая", "Толстая",
			"ОченьТолстая", "Двойная", "РедкийПунктир", "Пунктир", "СераяТочечная",
			"ТолстыйПунктир"};
		if( v < 10 ) return names[v];
		GCString s;
		s.Format("%u", v);
		return s;
	}

	//Colours of the 56-colour palette (docs/moxcel.md, "Таблица цветов"):
	//the basic ones by name, the rest as #rrggbb
	static GCString Colour(unsigned v)
	{
		static const unsigned long palette[56] = {
			0x000000, 0xffffff, 0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0xff00ff, 0x00ffff,
			0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0x808080, 0xc0c0c0,
			0x8080ff, 0x802060, 0xffffc0, 0xa0e0e0, 0x600080, 0xff8080, 0x0080c0, 0xc0c0ff,
			0x00cfff, 0x69ffff, 0xe0ffe0, 0xdd9cb3, 0xb38fee, 0x2a6ff9, 0x3fb8cd, 0x488436,
			0x958c41, 0x8e5e42, 0xa0627a, 0x624fac, 0x1d2fbe, 0x286676, 0x004500, 0x453e01,
			0x6a2813, 0x85396a, 0x4a3285, 0xc0dcc0, 0xa6caf0, 0x800000, 0x008000, 0x000080,
			0x808000, 0x800080, 0x008080, 0x808080, 0xfffbf0, 0xa0a0a4, 0x313900, 0xd9853e};
		static const struct { unsigned long rgb; const char* name; } names[] = {
			{0x000000, "Черный"}, {0xffffff, "Белый"}, {0xff0000, "Красный"},
			{0x00ff00, "Зеленый"}, {0x0000ff, "Синий"}, {0xffff00, "Желтый"},
			{0xff00ff, "Пурпурный"}, {0x00ffff, "Голубой"}, {0x800000, "ТемноКрасный"},
			{0x008000, "ТемноЗеленый"}, {0x808000, "Оливковый"}, {0x000080, "ТемноСиний"},
			{0x800080, "Фиолетовый"}, {0x008080, "СинеЗеленый"}, {0x808080, "Серый"},
			{0xc0c0c0, "СветлоСерый"}, {0, NULL}};
		GCString s;
		if( v >= 56 ) { s.Format("%u", v); return s; }
		for( int i = 0; names[i].name; i++ )
			if( names[i].rgb == palette[v] ) return names[i].name;
		s.Format("#%06lx", palette[v]);
		return s;
	}

	//Formatting set in a record as a list; Kind: 0 cell, 1 row, 2 column
	GCString Format(const CFormat& f, int Kind)
	{
		if( f.Rec == NULL ) return "";
		const unsigned char* p = f.Rec;
		unsigned f0 = p[0], f1 = p[1], f2 = p[2];
		GCString r, s;
		#define ADD(x) do { if( !r.IsEmpty() ) r += ", "; r += (x); } while(0)
		if( f0 & 0x01 ) ADD("Шрифт " + FontName(W16(p + 0x08)));
		if( f0 & 0x02 )
		{
			short v = S16(p + 0x0a);
			if( v < 0 && v % 4 == 0 ) s.Format("Размер %d", -v / 4);
			else s.Format("Размер (%d)", v);
			ADD(s);
		}
		if( f0 & 0x04 )
		{
			if( p[0x0c] == 7 ) ADD("Жирный");
			else if( p[0x0c] == 4 || p[0x0c] == 0 ) ADD("НеЖирный");
			else { s.Format("Жирный %u", p[0x0c]); ADD(s); }
		}
		if( f0 & 0x08 ) ADD(p[0x0d] ? "Наклонный" : "НеНаклонный");
		if( f0 & 0x10 ) ADD(p[0x0e] ? "Подчёркнутый" : "НеПодчёркнутый");
		if( (f0 & 0xE0) || (f1 & 0x01) )
		{
			bool set[4] = {(f0 & 0x20) != 0, (f0 & 0x40) != 0, (f0 & 0x80) != 0, (f1 & 0x01) != 0};
			unsigned val[4] = {p[0x12], p[0x13], p[0x14], p[0x15]};
			static const char* side[4] = {"слева", "сверху", "справа", "снизу"};
			if( set[0] && set[1] && set[2] && set[3] &&
				val[0] == val[1] && val[1] == val[2] && val[2] == val[3] )
				ADD("Рамка " + Border(val[0]));
			else
			{
				GCString fr = "Рамка";
				bool first = true;
				for( int i = 0; i < 4; i++ )
				{
					if( !set[i] ) continue;
					fr += first ? " " : ", ";
					fr += GCString(side[i]) + " " + Border(val[i]);
					first = false;
				}
				ADD(fr);
			}
		}
		if( f1 & 0x02 ) ADD("ЦветРамки " + Colour(p[0x17]));
		if( f1 & 0x04 )
		{
			if( Kind == 1 ) s.Format("Высота %u", W16(p + 0x04));
			else s.Format("Значение1 %u", W16(p + 0x04));
			ADD(s);
		}
		if( f1 & 0x08 )
		{
			if( Kind == 2 ) s.Format("Ширина %u", W16(p + 0x06));
			else s.Format("Значение2 %u", W16(p + 0x06));
			ADD(s);
		}
		if( f1 & 0x10 )
		{
			unsigned v = p[0x0f];
			switch( v & ~0x20u )
			{
				case 0: s = "ПоГоризонтали Лево"; break;
				case 2: s = "ПоГоризонтали Право"; break;
				case 4: s = "ПоГоризонтали ПоШирине"; break;
				case 6: s = "ПоГоризонтали Центр"; break;
				default: s.Format("ПоГоризонтали %u", v & ~0x20u);
			}
			if( v & 0x20 ) s += " ПоВыделеннымКолонкам";
			ADD(s);
		}
		if( f1 & 0x20 )
		{
			switch( p[0x10] )
			{
				case 0: s = "ПоВертикали Верх"; break;
				case 0x08: s = "ПоВертикали Низ"; break;
				case 0x18: s = "ПоВертикали Центр"; break;
				default: s.Format("ПоВертикали %u", p[0x10]);
			}
			ADD(s);
		}
		if( f1 & 0x40 ) ADD("ЦветШрифта " + Colour(p[0x18]));
		if( f1 & 0x80 ) ADD("Фон " + Colour(p[0x19]));
		if( f2 & 0x01 ) { s.Format("Узор %u", p[0x11]); ADD(s); }
		if( f2 & 0x02 ) ADD("ЦветУзора " + Colour(p[0x16]));
		if( f2 & 0x04 )
		{
			static const char* ctl[] = {"Авто", "Обрезать", "Забивать", "Переносить", "Красный", "ЗабиватьКрасный"};
			if( p[0x1a] < 6 ) s = GCString("Контроль ") + ctl[p[0x1a]];
			else s.Format("Контроль %u", p[0x1a]);
			ADD(s);
		}
		if( f2 & 0x08 )
		{
			static const char* typ[] = {"Текст", "Выражение", "Шаблон", "ФиксированныйШаблон"};
			if( p[0x1b] < 4 ) s = GCString("Тип ") + typ[p[0x1b]];
			else s.Format("Тип %u", p[0x1b]);
			ADD(s);
		}
		if( f2 & 0x10 ) ADD(p[0x1c] ? "Защита Нет" : "Защита Да");
		#undef ADD
		return r;
	}

	//The value block of a cell of a dialog table: type, length and precision,
	//format and mask; on a block of unexpected layout only its size
	GCString Value(const CFormat& f)
	{
		CReader v(f.Value, f.ValueLen);
		GCString r;
		try
		{
			unsigned w = v.U16();
			char type = (char)v.U8();
			unsigned long len = v.U32(), prec = v.U32(), kind = v.U32();
			unsigned nonneg = v.U8(), triads = v.U8();
			static const struct { char c; const char* name; } types[] = {
				{'U', "Неопределенный"}, {'N', "Число"}, {'S', "Строка"}, {'D', "Дата"},
				{'E', "Перечисление"}, {'B', "Справочник"}, {'O', "Документ"},
				{'C', "Календарь"}, {'A', "ВидРасчета"}, {'T', "Счет"},
				{'K', "ВидСубконто"}, {'P', "ПланСчетов"}, {0, NULL}};
			GCString tn;
			for( int i = 0; types[i].name; i++ )
				if( types[i].c == type ) tn = types[i].name;
			if( w != 3 || tn.IsEmpty() ) throw CMoxelError("");
			r = tn;
			GCString s;
			if( type == 'N' ) { s.Format(" %lu.%lu", len, prec); r += s; }
			else if( type == 'S' && len ) { s.Format(" %lu", len); r += s; }
			if( kind ) { s.Format(" вид %lu", kind); r += s; }
			if( nonneg ) r += " Неотрицательный";
			if( triads ) r += " РазделятьТриады";
			if( type != 'S' && type != 'U' )
			{
				GCString val = v.CStr();
				if( !val.IsEmpty() ) r += ", значение «" + Escape(val) + "»";
			}
			GCString fmt = v.CStr();
			if( !fmt.IsEmpty() ) r += ", формат «" + Escape(fmt) + "»";
			v.Take(6);
			if( type == 'S' )
			{
				GCString mask = v.CStr();
				if( !mask.IsEmpty() ) r += ", маска «" + Escape(mask) + "»";
			}
		}
		catch( CMoxelError& )
		{
			r.Format("%ld байт", f.ValueLen);
		}
		return r;
	}

	static GCString Cell(unsigned long row, unsigned long col)
	{
		GCString s;
		s.Format("R%luC%lu", row + 1, col + 1);
		return s;
	}

	//A text for Markdown: punctuation that would mark it up is escaped, line
	//breaks become <br>, control characters \xNN
	static GCString Md(const GCString& s)
	{
		GCString r;
		for( int i = 0; i < s.GetLength(); i++ )
		{
			unsigned char c = (unsigned char)s[i];
			if( c == '\r' && i + 1 < s.GetLength() && s[i+1] == '\n' ) { r += "<br>"; i++; }
			else if( c == '\n' ) r += "<br>";
			else if( c == '&' ) r += "&amp;";
			else if( strchr("\\`*_[]<>|~#", c) ) { r += '\\'; r += (char)c; }
			else if( c == '\t' ) r += "\\t";
			else if( c < 0x20 ) { GCString x; x.Format("\\x%02x", c); r += x; }
			else r += (char)c;
		}
		return r;
	}

	struct CCellInfo
	{
		unsigned long Row, Col;
		GCString Text, Fmt, Desc, Value;
		bool HasDesc, HasValue;
	};
	struct CRowInfo
	{
		unsigned long Row;
		GCString Fmt;
		int First, Last; //its cells in Cells
	};
	struct CSection
	{
		GCString Name;
		unsigned long Beg, End;
		unsigned Parent;
	};

	std::vector<CCellInfo> Cells;
	std::vector<CRowInfo> Rows;
	std::vector<CSection> RowSect, ColSect;

	CCellInfo Info(unsigned long row, unsigned long col, const CFormat& f)
	{
		CCellInfo c;
		c.Row = row;
		c.Col = col;
		if( f.HasText ) c.Text = f.Text;
		c.Fmt = Format(f, 0);
		c.HasDesc = f.HasDesc;
		if( f.HasDesc ) c.Desc = f.Desc;
		c.HasValue = f.Value != NULL;
		if( c.HasValue ) c.Value = Value(f);
		return c;
	}

	//List items of a cell: formatting, details, value; the text too when
	//WithText (cells outside the grid)
	static void CellItems(GCString& Out, const GCString& Indent, const GCString& Ref,
		const CCellInfo& c, bool WithText)
	{
		GCString line;
		if( WithText && !c.Text.IsEmpty() ) line = " " + Md(c.Text);
		if( !c.Fmt.IsEmpty() ) line += (line.IsEmpty() ? " " : " — ") + c.Fmt;
		if( !line.IsEmpty() ) Out += Indent + "- " + Ref + ":" + line + "\r\n";
		if( c.HasDesc ) Out += Indent + "- " + Ref + " расшифровка: " + Md(c.Desc) + "\r\n";
		if( c.HasValue ) Out += Indent + "- " + Ref + " значение: " + Md(c.Value) + "\r\n";
	}

	static bool HasItems(const CCellInfo& c)
	{
		return !c.Fmt.IsEmpty() || c.HasDesc || c.HasValue;
	}

	void Numbers(CArray<unsigned long, unsigned long>& n)
	{
		unsigned long c = R.Count();
		for( unsigned long i = 0; i < c; i++ ) n.Add(R.U32());
	}

	void ItemCount(CArray<unsigned long, unsigned long>& n)
	{
		unsigned long c = R.Count();
		if( c != (unsigned long)n.GetSize() )
		{
			GCString s;
			s.Format("номеров %d, а элементов %lu (смещение %ld)", (int)n.GetSize(), c, R.p);
			throw CMoxelError(s);
		}
	}

	void Sections(std::vector<CSection>& v)
	{
		unsigned long c = R.Count();
		for( unsigned long i = 0; i < c; i++ )
		{
			const unsigned char* b = R.Take(12);
			CSection s;
			s.Beg = b[0] | (b[1] << 8) | ((unsigned long)b[2] << 16) | ((unsigned long)b[3] << 24);
			s.End = b[4] | (b[5] << 8) | ((unsigned long)b[6] << 16) | ((unsigned long)b[7] << 24);
			s.Parent = W16(b + 8);
			s.Name = R.CStr();
			v.push_back(s);
		}
	}

	static GCString Range(char Letter, unsigned long beg, unsigned long end)
	{
		GCString s;
		if( beg == end ) s.Format("%c%lu", Letter, beg + 1);
		else s.Format("%c%lu–%c%lu", Letter, beg + 1, Letter, end + 1);
		return s;
	}

	static void SectionItems(GCString& Out, LPCSTR Title, char Letter, const std::vector<CSection>& v)
	{
		if( v.empty() ) return;
		Out += GCString("\r\n## ") + Title + "\r\n\r\n";
		for( size_t i = 0; i < v.size(); i++ )
		{
			Out += "- " + Md(v[i].Name) + ": " + Range(Letter, v[i].Beg, v[i].End);
			if( v[i].Parent )
			{
				GCString s;
				s.Format(" (вложена в %u)", v[i].Parent);
				Out += s;
			}
			Out += "\r\n";
		}
	}

	GCString Breaks(char Letter)
	{
		CArray<unsigned long, unsigned long> n;
		Numbers(n);
		GCString r, s;
		for( int i = 0; i < n.GetSize(); i++ )
		{
			s.Format(" %c%lu", Letter, n[i] + 1);
			r += s;
		}
		return r;
	}

	//The section of the top level a row belongs to, -1 for none
	int SectionOf(unsigned long row)
	{
		for( size_t i = 0; i < RowSect.size(); i++ )
			if( RowSect[i].Parent == 0 && RowSect[i].Beg <= row && row <= RowSect[i].End )
				return (int)i;
		return -1;
	}

	//The texts of rows [b, e) of Rows as a Markdown table with the columns
	//that have text
	GCString Grid(size_t b, size_t e)
	{
		std::vector<unsigned long> cols;
		for( size_t i = b; i < e; i++ )
			for( int j = Rows[i].First; j < Rows[i].Last; j++ )
				if( !Cells[j].Text.IsEmpty() )
				{
					size_t k = 0;
					while( k < cols.size() && cols[k] < Cells[j].Col ) k++;
					if( k == cols.size() || cols[k] != Cells[j].Col )
						cols.insert(cols.begin() + k, Cells[j].Col);
				}
		GCString r = "|  |", s;
		for( size_t k = 0; k < cols.size(); k++ ) { s.Format(" C%lu |", cols[k] + 1); r += s; }
		r += "\r\n|---|";
		for( size_t k = 0; k < cols.size(); k++ ) r += "---|";
		r += "\r\n";
		for( size_t i = b; i < e; i++ )
		{
			s.Format("| R%lu |", Rows[i].Row + 1);
			r += s;
			int j = Rows[i].First;
			for( size_t k = 0; k < cols.size(); k++ )
			{
				while( j < Rows[i].Last && Cells[j].Col < cols[k] ) j++;
				if( j < Rows[i].Last && Cells[j].Col == cols[k] && !Cells[j].Text.IsEmpty() )
					r += " " + Md(Cells[j].Text) + " |";
				else
					r += " |";
			}
			r += "\r\n";
		}
		return r;
	}

	static bool RowHasText(const CRowInfo& row, const std::vector<CCellInfo>& cells)
	{
		for( int j = row.First; j < row.Last; j++ )
			if( !cells[j].Text.IsEmpty() ) return true;
		return false;
	}

	bool Run()
	{
		const unsigned char* sig = R.Take(6);
		if( memcmp(sig, "MOXCEL", 6) != 0 ) throw CMoxelError("нет сигнатуры MOXCEL");
		R.Take(5);
		Version = R.U16();
		unsigned long cols = R.U32(), rows = R.U32();
		R.U32(); //objects

		GCString s, Head;
		bool Content = false;
		s.Format("- Версия %d; колонок %lu, строк %lu\r\n", Version, cols, rows);
		Head = s;

		CFormat tab;
		Prop(tab);
		GCString tf = Format(tab, 0);
		if( !tf.IsEmpty() ) { Head += "- Таблица: " + tf + "\r\n"; Content = true; }

		//fonts
		CArray<unsigned long, unsigned long> ids;
		Numbers(ids);
		ItemCount(ids);
		for( int i = 0; i < ids.GetSize(); i++ )
		{
			const unsigned char* f = R.Take(60);
			int len = 0;
			while( len < 32 && f[28 + len] ) len++;
			FontIDs.Add(ids[i]);
			FontNames.Add(GCString((const char*)f + 28, len));
		}
		if( ids.GetSize() )
		{
			Head += "- Шрифты:";
			for( int i = 0; i < ids.GetSize(); i++ )
			{
				s.Format(" %lu «%s»", ids[i], (LPCSTR)Md(GCString(FontNames[i])));
				Head += s;
			}
			Head += "\r\n";
		}
		R.U32();

		CFormat hdr, ftr;
		Ext(hdr);
		Ext(ftr);
		CCellInfo hi = Info(0, 0, hdr), fi = Info(0, 0, ftr);
		if( !hi.Text.IsEmpty() || HasItems(hi) ) { CellItems(Head, "", "ВерхнийКолонтитул", hi, true); Content = true; }
		if( !fi.Text.IsEmpty() || HasItems(fi) ) { CellItems(Head, "", "НижнийКолонтитул", fi, true); Content = true; }

		//columns
		CArray<unsigned long, unsigned long> cn;
		Numbers(cn);
		ItemCount(cn);
		GCString ColLines;
		for( int i = 0; i < cn.GetSize(); i++ )
		{
			CFormat f;
			Prop(f);
			GCString cf = Format(f, 2);
			if( !cf.IsEmpty() ) { s.Format("- C%lu: ", cn[i] + 1); ColLines += s + cf + "\r\n"; }
		}

		//rows and cells
		CArray<unsigned long, unsigned long> rn;
		Numbers(rn);
		ItemCount(rn);
		for( int i = 0; i < rn.GetSize(); i++ )
		{
			CFormat f;
			Prop(f);
			CRowInfo row;
			row.Row = rn[i];
			row.Fmt = Format(f, 1);
			CArray<unsigned long, unsigned long> ccn;
			Numbers(ccn);
			ItemCount(ccn);
			row.First = (int)Cells.size();
			for( int j = 0; j < ccn.GetSize(); j++ )
			{
				CFormat c;
				Ext(c);
				Cells.push_back(Info(rn[i], ccn[j], c));
			}
			row.Last = (int)Cells.size();
			Rows.push_back(row);
		}

		//objects
		unsigned long no = R.Count();
		GCString ObjLines;
		for( unsigned long i = 0; i < no; i++ )
		{
			CFormat f;
			Ext(f);
			const unsigned char* g = R.Take(40);
			long v[10];
			for( int k = 0; k < 10; k++ )
				v[k] = (long)(g[k*4] | (g[k*4+1] << 8) | ((unsigned long)g[k*4+2] << 16) | ((unsigned long)g[k*4+3] << 24));
			static const char* kinds[] = {"", "Линия", "Прямоугольник", "Текст", "OLE", "Рисунок"};
			GCString kind;
			if( v[0] >= 1 && v[0] <= 5 ) kind = kinds[v[0]];
			else
			{
				s.Format("неизвестный вид объекта %ld (смещение %ld)", v[0], R.p);
				throw CMoxelError(s);
			}
			if( v[0] == 5 )
			{
				R.U32();
				R.Take((long)R.U32());
			}
			else if( v[0] == 4 )
			{
				unsigned tag = R.U16();
				if( tag == 0xFFFF )
				{
					R.U16();
					R.Take((long)R.U16());
				}
				else if( !(tag & 0x8000) )
					throw CMoxelError("ссылка на объект вместо класса OLE");
				R.Take(18);
				R.Take((long)R.U32());
			}
			s.Format("%lu %s %s:%s, уровень %ld", i + 1, (LPCSTR)kind,
				(LPCSTR)Cell(v[2], v[1]), (LPCSTR)Cell(v[6], v[5]), v[9]);
			CCellInfo oi = Info(0, 0, f);
			if( oi.Text.IsEmpty() && !HasItems(oi) ) ObjLines += "- " + s + "\r\n";
			else CellItems(ObjLines, "", s, oi, true);
		}

		if( Version == 1 || Version == 2 ) R.U16();

		//joins
		unsigned long nj = R.Count();
		GCString JoinLines;
		for( unsigned long i = 0; i < nj; i++ )
		{
			unsigned long c0 = R.U32(), r0 = R.U32(), c1 = R.U32(), r1 = R.U32();
			JoinLines += "- " + Cell(r0, c0) + ":" + Cell(r1, c1) + "\r\n";
		}

		Sections(ColSect);
		Sections(RowSect);
		GCString ColBreaks = Breaks('C'), RowBreaks = Breaks('R');

		GCString NameLines;
		if( Version != 1 && Version != 2 )
		{
			unsigned long nn = R.U32();
			for( unsigned long i = 0; i < nn; i++ )
			{
				GCString name = R.CStr();
				R.Take(12);
				unsigned long c0 = R.U32(), r0 = R.U32(), c1 = R.U32(), r1 = R.U32();
				NameLines += "- " + Md(name) + ": " + Cell(r0, c0) + ":" + Cell(r1, c1) + "\r\n";
			}
		}

		//the texts by row sections of the top level; rows without text are
		//left out, and the rows outside sections go by runs
		Out = Head;
		size_t i = 0;
		while( i < Rows.size() )
		{
			if( !RowHasText(Rows[i], Cells) ) { i++; continue; }
			int sect = SectionOf(Rows[i].Row);
			size_t e = i + 1, last = i;
			while( e < Rows.size() && SectionOf(Rows[e].Row) == sect )
			{
				if( RowHasText(Rows[e], Cells) ) last = e;
				e++;
			}
			std::vector<CRowInfo> keep;
			for( size_t k = i; k <= last; k++ )
				if( RowHasText(Rows[k], Cells) ) keep.push_back(Rows[k]);
			if( sect >= 0 )
				Out += "\r\n## " + Md(RowSect[sect].Name) + " (" +
					Range('R', RowSect[sect].Beg, RowSect[sect].End) + ")\r\n\r\n";
			else
				Out += "\r\n## " + Range('R', Rows[i].Row, Rows[last].Row) + "\r\n\r\n";
			std::vector<CRowInfo> all;
			all.swap(Rows);
			Rows = keep;
			Out += Grid(0, Rows.size());
			Rows.swap(all);
			Content = true;
			i = e;
		}

		GCString Fmt;
		if( !ColLines.IsEmpty() ) Fmt += "\r\n### Колонки\r\n\r\n" + ColLines;
		GCString RowLines;
		for( size_t k = 0; k < Rows.size(); k++ )
		{
			GCString items;
			for( int j = Rows[k].First; j < Rows[k].Last; j++ )
				CellItems(items, "  ", Cell(Cells[j].Row, Cells[j].Col), Cells[j], false);
			if( Rows[k].Fmt.IsEmpty() && items.IsEmpty() ) continue;
			s.Format("- R%lu", Rows[k].Row + 1);
			RowLines += s + (Rows[k].Fmt.IsEmpty() ? GCString("") : ": " + Rows[k].Fmt) + "\r\n" + items;
		}
		if( !RowLines.IsEmpty() ) Fmt += "\r\n### Строки и ячейки\r\n\r\n" + RowLines;
		if( !Fmt.IsEmpty() ) { Out += "\r\n## Оформление\r\n" + Fmt; Content = true; }

		if( !JoinLines.IsEmpty() ) { Out += "\r\n## Объединения\r\n\r\n" + JoinLines; Content = true; }
		if( !ObjLines.IsEmpty() ) { Out += "\r\n## Объекты\r\n\r\n" + ObjLines; Content = true; }
		GCString Rest;
		SectionItems(Rest, "Секции строк", 'R', RowSect);
		SectionItems(Rest, "Секции колонок", 'C', ColSect);
		if( !RowBreaks.IsEmpty() || !ColBreaks.IsEmpty() )
		{
			Rest += "\r\n## Разрывы страниц\r\n\r\n";
			if( !RowBreaks.IsEmpty() ) Rest += "- После строк:" + RowBreaks + "\r\n";
			if( !ColBreaks.IsEmpty() ) Rest += "- После колонок:" + ColBreaks + "\r\n";
		}
		if( !NameLines.IsEmpty() ) Rest += "\r\n## Именованные области\r\n\r\n" + NameLines;
		if( !Rest.IsEmpty() ) { Out += Rest; Content = true; }

		if( R.p < R.n )
		{
			s.Format("\r\nПосле таблицы: %ld байт\r\n", R.n - R.p);
			Out += s;
		}
		return Content;
	}
};

} //namespace

int MoxelToText(const GCString& Data, GCString& Text, GCString& Error)
{
	CMoxelText m((const unsigned char*)(LPCSTR)Data, Data.GetLength());
	try
	{
		bool content = m.Run();
		Text = "<!-- Вид таблицы для просмотра изменений; GComp пишет его с ключом --mxl-text\r\n"
			"     и при сборке не читает: таблица собирается из .mxl. -->\r\n\r\n" + m.Out;
		return content ? 1 : 0;
	}
	catch( CMoxelError& e )
	{
		Error = e.What;
		return -1;
	}
}
