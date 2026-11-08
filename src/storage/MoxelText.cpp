// SPDX-License-Identifier: GPL-2.0-or-later
/* A text view of a moxcel table (.mxl) for reviewing changes: the texts,
   details and values of the cells by rows, the formatting of cells, rows and
   columns, sections, named areas, joins and objects. It is written next to
   the table with --mxl-text and never read back: compiling takes the .mxl.

   The layout follows docs/moxcel.md and its "Уточнения формата (2026)":
   MFC CArchive data, CString lengths, WORD counts (FFFF + DWORD), 30-byte
   format records (32 in version 7, 28 in versions 1 and 2). */

#include <string.h>
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

	//Formatting set in a record; Kind: 0 cell, 1 row, 2 column
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
			GCString fr = "Рамка";
			if( f0 & 0x20 ) { s.Format(" Л%u", p[0x12]); fr += s; }
			if( f0 & 0x40 ) { s.Format(" В%u", p[0x13]); fr += s; }
			if( f0 & 0x80 ) { s.Format(" П%u", p[0x14]); fr += s; }
			if( f1 & 0x01 ) { s.Format(" Н%u", p[0x15]); fr += s; }
			ADD(fr);
		}
		if( f1 & 0x02 ) { s.Format("ЦветРамки %u", p[0x17]); ADD(s); }
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
		if( f1 & 0x40 ) { s.Format("ЦветШрифта %u", p[0x18]); ADD(s); }
		if( f1 & 0x80 ) { s.Format("Фон %u", p[0x19]); ADD(s); }
		if( f2 & 0x01 ) { s.Format("Узор %u", p[0x11]); ADD(s); }
		if( f2 & 0x02 ) { s.Format("ЦветУзора %u", p[0x16]); ADD(s); }
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
		return r.IsEmpty() ? r : " {" + r + "}";
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

	//Lines of a cell: text with its formatting, details, value
	void CellLines(const GCString& Indent, const GCString& Ref, const CFormat& f)
	{
		Out += Indent + Ref + ":";
		if( f.HasText ) Out += " " + Escape(f.Text);
		Out += Format(f, 0) + "\r\n";
		if( f.HasDesc ) Out += Indent + Ref + " расшифровка: " + Escape(f.Desc) + "\r\n";
		if( f.Value ) Out += Indent + Ref + " значение: " + Value(f) + "\r\n";
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

	void Sections(LPCSTR Title, bool Rows, GCString& Lines)
	{
		unsigned long c = R.Count();
		GCString s;
		for( unsigned long i = 0; i < c; i++ )
		{
			const unsigned char* b = R.Take(12);
			unsigned long beg = b[0] | (b[1] << 8) | ((unsigned long)b[2] << 16) | ((unsigned long)b[3] << 24);
			unsigned long end = b[4] | (b[5] << 8) | ((unsigned long)b[6] << 16) | ((unsigned long)b[7] << 24);
			unsigned parent = W16(b + 8);
			GCString name = R.CStr();
			if( beg == end ) s.Format("\t%s: %c%lu", (LPCSTR)Escape(name), Rows ? 'R' : 'C', beg + 1);
			else s.Format("\t%s: %c%lu-%c%lu", (LPCSTR)Escape(name), Rows ? 'R' : 'C', beg + 1, Rows ? 'R' : 'C', end + 1);
			Lines += s;
			if( parent ) { s.Format(" (вложена в %u)", parent); Lines += s; }
			Lines += "\r\n";
		}
		if( c ) Lines = GCString(Title) + ":\r\n" + Lines;
	}

	void Breaks(LPCSTR Title, char Letter, GCString& Lines)
	{
		CArray<unsigned long, unsigned long> n;
		Numbers(n);
		if( n.GetSize() == 0 ) return;
		Lines = GCString(Title) + ":";
		GCString s;
		for( int i = 0; i < n.GetSize(); i++ )
		{
			s.Format(" %c%lu", Letter, n[i] + 1);
			Lines += s;
		}
		Lines += "\r\n";
	}

	bool Run()
	{
		const unsigned char* sig = R.Take(6);
		if( memcmp(sig, "MOXCEL", 6) != 0 ) throw CMoxelError("нет сигнатуры MOXCEL");
		R.Take(5);
		Version = R.U16();
		unsigned long cols = R.U32(), rows = R.U32();
		R.U32(); //objects

		GCString s;
		s.Format("Версия: %d\r\nРазмер: %lu колонок, %lu строк\r\n", Version, cols, rows);
		GCString Head = s;
		bool Content = false;

		CFormat tab;
		Prop(tab);
		GCString tf = Format(tab, 0);
		if( !tf.IsEmpty() ) { Head += "Таблица:" + tf + "\r\n"; Content = true; }

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
			Head += "Шрифты:";
			for( int i = 0; i < ids.GetSize(); i++ )
			{
				s.Format(" %lu «%s»", ids[i], (LPCSTR)FontNames[i]);
				Head += s;
			}
			Head += "\r\n";
		}
		R.U32();

		CFormat hdr, ftr;
		Ext(hdr);
		Ext(ftr);
		if( hdr.HasText || hdr.HasDesc || !Format(hdr, 0).IsEmpty() ) { CellLines("", "ВерхнийКолонтитул", hdr); Content = true; }
		if( ftr.HasText || ftr.HasDesc || !Format(ftr, 0).IsEmpty() ) { CellLines("", "НижнийКолонтитул", ftr); Content = true; }
		Out = Head + Out;

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
			if( !cf.IsEmpty() ) { s.Format("\tC%lu", cn[i] + 1); ColLines += s + cf + "\r\n"; }
		}
		if( !ColLines.IsEmpty() ) { Out += "Колонки:\r\n" + ColLines; Content = true; }

		//rows and cells
		CArray<unsigned long, unsigned long> rn;
		Numbers(rn);
		ItemCount(rn);
		GCString RowLines;
		for( int i = 0; i < rn.GetSize(); i++ )
		{
			CFormat f;
			Prop(f);
			GCString rf = Format(f, 1);
			CArray<unsigned long, unsigned long> ccn;
			Numbers(ccn);
			ItemCount(ccn);
			GCString Saved = Out;
			Out = "";
			for( int j = 0; j < ccn.GetSize(); j++ )
			{
				CFormat c;
				Ext(c);
				CellLines("\t\t", Cell(rn[i], ccn[j]), c);
			}
			GCString Cells = Out;
			Out = Saved;
			if( !rf.IsEmpty() || !Cells.IsEmpty() )
			{
				s.Format("\tR%lu", rn[i] + 1);
				RowLines += s + rf + "\r\n" + Cells;
			}
		}
		if( !RowLines.IsEmpty() ) { Out += "Строки:\r\n" + RowLines; Content = true; }

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
			s.Format("\t%lu %s %s:%s, уровень %ld", i + 1, (LPCSTR)kind,
				(LPCSTR)Cell(v[2], v[1]), (LPCSTR)Cell(v[6], v[5]), v[9]);
			GCString Saved = Out;
			Out = "";
			CellLines("", s, f);
			ObjLines += Out;
			Out = Saved;
		}
		if( !ObjLines.IsEmpty() ) { Out += "Объекты:\r\n" + ObjLines; Content = true; }

		if( Version == 1 || Version == 2 ) R.U16();

		//joins
		unsigned long nj = R.Count();
		GCString JoinLines;
		for( unsigned long i = 0; i < nj; i++ )
		{
			unsigned long c0 = R.U32(), r0 = R.U32(), c1 = R.U32(), r1 = R.U32();
			JoinLines += "\t" + Cell(r0, c0) + ":" + Cell(r1, c1) + "\r\n";
		}
		if( !JoinLines.IsEmpty() ) { Out += "Объединения:\r\n" + JoinLines; Content = true; }

		GCString ColSect, RowSect, ColBreaks, RowBreaks;
		Sections("Секции колонок", false, ColSect);
		Sections("Секции строк", true, RowSect);
		Breaks("Разрывы страниц после колонок", 'C', ColBreaks);
		Breaks("Разрывы страниц после строк", 'R', RowBreaks);
		GCString Rest = RowSect + ColSect + RowBreaks + ColBreaks;

		if( Version != 1 && Version != 2 )
		{
			unsigned long nn = R.U32();
			GCString NameLines;
			for( unsigned long i = 0; i < nn; i++ )
			{
				GCString name = R.CStr();
				R.Take(12);
				unsigned long c0 = R.U32(), r0 = R.U32(), c1 = R.U32(), r1 = R.U32();
				NameLines += "\t" + Escape(name) + ": " + Cell(r0, c0) + ":" + Cell(r1, c1) + "\r\n";
			}
			if( !NameLines.IsEmpty() ) Rest += "Именованные области:\r\n" + NameLines;
		}
		if( !Rest.IsEmpty() ) { Out += Rest; Content = true; }

		if( R.p < R.n )
		{
			s.Format("После таблицы: %ld байт\r\n", R.n - R.p);
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
		Text = "// Текстовый вид таблицы для просмотра изменений; GComp пишет его с ключом\r\n"
			"// --mxl-text и при сборке не читает: таблица собирается из .mxl.\r\n" + m.Out;
		return content ? 1 : 0;
	}
	catch( CMoxelError& e )
	{
		Error = e.What;
		return -1;
	}
}
