// SPDX-License-Identifier: GPL-2.0-or-later
/* --ert: every report and data processor of a configuration is also written
   as an external report <object dir>\<name>.ert, as the Configurator saves
   it with "—охранить как внешний отчет (обработку)" (docs/md-file.md,
   "¬нешний отчЄт .ert"):

   - Main MetaData Stream: the text of the configuration with the lists of
     the objects an external report does not need emptied, the attributes,
     values and forms of catalogs, documents and enumerations left out,
     only HolidaysDef of the calendars, the accounting parameters without
     accounts, attributes, kinds of subconto and list forms; CRC counted
     again; a zero byte after it;
   - the streams of the work book of the object as they are, the
     description as "Inplace description";
   - the pictures the form refers to in a gallery of its own, numbered from
     0x1F000001, the form referring to the new numbers; the thumbnails come
     from the gallery of the configuration; a BMP gets the reserved bytes
     0xFF in its palette and a palette icon becomes a 32-bit one, as the
     Configurator writes them.

   The form is left as it is (the Configurator saves it again with the
   client area offsets of its own computer). */

#include <string.h>
#include <vector>
#include <string>
#include "Declarations.h"
#include "CommonFunctions.h"
#include "Storage.h"
#include "NameTable.h"
#include "MMS.h"
#include "ExportERT.h"

namespace {

//A brace list of 1C text with the positions of its items
struct CNode
{
	int Start, End;          //[Start, End) in the text, braces or quotes included
	bool List;
	std::string Name;        //the first string of a list
	std::vector<int> Kids;   //indices in the pool
};

class CBraceText
{
public:
	const char* T;
	int N;
	std::vector<CNode> Pool;

	CBraceText(const GCString& Text) : T(Text), N(Text.GetLength()) {}

	//parses the list at Pos ('{'); returns its index or -1
	int Parse(int& Pos)
	{
		int me = (int)Pool.size();
		Pool.push_back(CNode());
		Pool[me].Start = Pos;
		Pool[me].List = true;
		Pos++;
		bool first = true;
		while( Pos < N )
		{
			char c = T[Pos];
			if( c == ' ' || c == '\r' || c == '\n' || c == ',' ) { Pos++; continue; }
			if( c == '}' )
			{
				Pool[me].End = ++Pos;
				return me;
			}
			if( c == '{' )
			{
				int k = Parse(Pos);
				if( k < 0 ) return -1;
				Pool[me].Kids.push_back(k);
				first = false;
				continue;
			}
			if( c == '"' )
			{
				int e = Pos + 1;
				while( e < N )
				{
					if( T[e] == '"' )
					{
						if( e + 1 < N && T[e+1] == '"' ) { e += 2; continue; }
						break;
					}
					e++;
				}
				if( e >= N ) return -1;
				int k = (int)Pool.size();
				Pool.push_back(CNode());
				Pool[k].Start = Pos;
				Pool[k].End = e + 1;
				Pool[k].List = false;
				if( first ) Pool[me].Name.assign(T + Pos + 1, e - Pos - 1);
				Pool[me].Kids.push_back(k);
				Pos = e + 1;
				first = false;
				continue;
			}
			return -1;
		}
		return -1;
	}

	int Root()
	{
		const char* b = strchr(T, '{');
		if( b == NULL ) return -1;
		int pos = (int)(b - T);
		return Parse(pos);
	}

	CNode& operator[](int i) { return Pool[i]; }
	GCString Value(int i) { return GCString(T + Pool[i].Start + 1, Pool[i].End - Pool[i].Start - 2); }
};

struct CCut
{
	int Start, End;
	GCString With;
};

GCString ApplyCuts(const GCString& Text, std::vector<CCut>& Cuts)
{
	//from the end, so the positions stay valid; the ranges do not overlap
	for( size_t i = 1; i < Cuts.size(); i++ )
		for( size_t j = i; j > 0 && Cuts[j-1].Start < Cuts[j].Start; j-- )
			std::swap(Cuts[j-1], Cuts[j]);
	GCString r = Text;
	for( size_t i = 0; i < Cuts.size(); i++ )
		r = GCString(r.Left(Cuts[i].Start)) + Cuts[i].With + GCString(r.Mid(Cuts[i].End));
	return r;
}

//keep the opening brace and the name of a list
void EmptyList(CBraceText& B, int n, std::vector<CCut>& Cuts)
{
	if( B[n].Kids.size() < 2 ) return;
	CCut c;
	c.Start = B[B[n].Kids[0]].End;
	c.End = B[n].End - 1;
	Cuts.push_back(c);
}

void EmptyKids(CBraceText& B, int n, const char* const* Names, std::vector<CCut>& Cuts)
{
	for( size_t i = 0; i < B[n].Kids.size(); i++ )
	{
		int k = B[n].Kids[i];
		if( !B[k].List ) continue;
		for( int j = 0; Names[j]; j++ )
			if( B[k].Name == Names[j] ) EmptyList(B, k, Cuts);
	}
}

//the text of the Main MetaData Stream of an external report, from that of
//the configuration; false when the text cannot be read
bool ReduceMMS(const GCString& Text, GCString& Out)
{
	static const char* const EmptyRoot[] = {"GenJrnlFldDef", "DocSelRefObj", "DocNumDef",
		"Consts", "Registers", "Journalisters", "ReportList", "CJ", "Algorithms",
		"CalcVars", "Groups", NULL};
	static const char* const SbCnts[] = {"Params", "Form", NULL};
	static const char* const Documents[] = {"Head Fields", "Table Fields", NULL};
	static const char* const Enums[] = {"EnumVal", NULL};
	static const char* const Buh[] = {"AccParams", "ProvParams", "SbKind", "Form", NULL};
	static const char* const Plans[] = {"Accounts", NULL};

	CBraceText B(Text);
	int root = B.Root();
	if( root < 0 ) return false;
	std::vector<CCut> Cuts;
	CNode& R = B[root];
	for( size_t s = 0; s < R.Kids.size(); s++ )
	{
		int sec = R.Kids[s];
		if( !B[sec].List ) continue;
		const std::string& nm = B[sec].Name;
		bool empty = false;
		for( int j = 0; EmptyRoot[j]; j++ ) if( nm == EmptyRoot[j] ) empty = true;
		if( empty )
			EmptyList(B, sec, Cuts);
		else if( nm == "SbCnts" || nm == "Documents" || nm == "EnumList" )
		{
			const char* const* want = nm == "SbCnts" ? SbCnts : nm == "Documents" ? Documents : Enums;
			for( size_t i = 1; i < B[sec].Kids.size(); i++ )
				if( B[B[sec].Kids[i]].List ) EmptyKids(B, B[sec].Kids[i], want, Cuts);
		}
		else if( nm == "Calendars" )
		{
			//only HolidaysDef stays: a calendar goes from the end of the item before it
			for( size_t i = 1; i < B[sec].Kids.size(); i++ )
				if( B[B[sec].Kids[i]].Name != "HolidaysDef" )
				{
					CCut c;
					c.Start = B[B[sec].Kids[i-1]].End;
					c.End = B[B[sec].Kids[i]].End;
					Cuts.push_back(c);
				}
		}
		else if( nm == "Buh" )
		{
			for( size_t i = 1; i < B[sec].Kids.size(); i++ )
			{
				int item = B[sec].Kids[i];
				if( !B[item].List ) continue;
				EmptyKids(B, item, Buh, Cuts);
				for( size_t j = 0; j < B[item].Kids.size(); j++ )
				{
					int k = B[item].Kids[j];
					if( B[k].List && B[k].Name == "Plans" )
						for( size_t p = 1; p < B[k].Kids.size(); p++ )
							if( B[B[k].Kids[p]].List ) EmptyKids(B, B[k].Kids[p], Plans, Cuts);
				}
			}
		}
		else if( nm == "CRC" && s > 0 )
		{
			CCut c;
			c.Start = B[R.Kids[s-1]].End;
			c.End = B[sec].End;
			Cuts.push_back(c);
		}
	}
	Out = ApplyCuts(Text.Left(R.End), Cuts).Mid(R.Start);
	GCString crc;
	crc.Format(",\r\n{\"CRC\",\"%u\"}}\r\n", CMMS::CalcCRC(Out, Out.GetLength()));
	Out = GCString(Out.Left(Out.GetLength() - 1)) + crc;
	return true;
}

//the picture numbers of a form: the Frame parameter 27 and the parameter 41
//of every control; Old gets the pictures as the Configurator numbers them,
//0x1F000001 and on: going over the controls from the last one, a picture
//gets its number where it is met first (checked on two forms of 13 and 8
//pictures; the place of the picture of the form itself, Frame, is not
//known Ч it comes last); a number the gallery lacks (a lost picture) stays
bool RenumberPictures(const GCString& Text, GCString& Out, CArray<unsigned long, unsigned long>& Old,
	const CArray<unsigned long, unsigned long>& Known)
{
	CBraceText B(Text);
	int root = B.Root();
	if( root < 0 ) return false;
	std::vector<int> refs;
	for( size_t s = 0; s < B[root].Kids.size(); s++ )
	{
		int sec = B[root].Kids[s];
		if( !B[sec].List ) continue;
		if( B[sec].Name == "Frame" && B[sec].Kids.size() > 1 )
		{
			int params = B[sec].Kids[1];
			if( B[params].List && B[params].Kids.size() > 26 && !B[B[params].Kids[26]].List )
				refs.push_back(B[params].Kids[26]);
		}
		else if( B[sec].Name == "Controls" )
			for( size_t i = 1; i < B[sec].Kids.size(); i++ )
			{
				int ctl = B[sec].Kids[i];
				if( B[ctl].List && B[ctl].Kids.size() > 40 && !B[B[ctl].Kids[40]].List )
					refs.push_back(B[ctl].Kids[40]);
			}
	}
	std::vector<CCut> Cuts;
	for( size_t r = refs.size(); r-- > 0; )
	{
		size_t i = r;
		unsigned long id = strtoul(B.Value(refs[i]), NULL, 10);
		if( id == 0 ) continue;
		int k;
		for( k = 0; k < Known.GetSize(); k++ ) if( Known[k] == id ) break;
		if( k == Known.GetSize() ) continue;
		for( k = 0; k < Old.GetSize(); k++ ) if( Old[k] == id ) break;
		if( k == Old.GetSize() ) Old.Add(id);
		CCut c;
		c.Start = B[refs[i]].Start;
		c.End = B[refs[i]].End;
		c.With.Format("\"%lu\"", 0x1F000001UL + k);
		Cuts.push_back(c);
	}
	Out = ApplyCuts(Text, Cuts);
	return true;
}

unsigned long Dword(const GCString& s, int at)
{
	const unsigned char* p = (const unsigned char*)(LPCSTR)s + at;
	return p[0] | (p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

void SetDword(GCString& s, int at, unsigned long v)
{
	for( int i = 0; i < 4; i++ ) s.SetAt(at + i, (char)((v >> (8 * i)) & 0xFF));
}

void SetWord(GCString& s, int at, unsigned v)
{
	s.SetAt(at, (char)(v & 0xFF));
	s.SetAt(at + 1, (char)((v >> 8) & 0xFF));
}

void PutDword(GCString& s, unsigned long v)
{
	for( int i = 0; i < 4; i++ ) s += (char)((v >> (8 * i)) & 0xFF);
}

//the picture numbers listed in a gallery
void GalleryIDs(const GCString& Src, CArray<unsigned long, unsigned long>& IDs)
{
	if( Src.GetLength() < 28 + 54 || Src.Left(2) != "IL" ) return;
	const int bmp = 28;
	unsigned long off = Dword(Src, bmp + 10);
	long w = (long)Dword(Src, bmp + 18), h = (long)Dword(Src, bmp + 22);
	int tail = bmp + (int)(off + (unsigned long)w * 2 * h);
	if( w <= 0 || h <= 0 || tail + 8 > Src.GetLength() ) return;
	unsigned long n = Dword(Src, tail + 4);
	for( unsigned long i = 0; i < n && tail + 12 + 4 * (int)i <= Src.GetLength(); i++ )
		IDs.Add(Dword(Src, tail + 8 + 4 * (int)i));
}

//The gallery of the external report: the image list header of the
//configuration with the new count and capacity, the thumbnails of the
//pictures copied from their cells, the numbers and an empty list of names
bool BuildGallery(const GCString& Src, const CArray<unsigned long, unsigned long>& Old, GCString& Out, GCString& Error)
{
	if( Src.GetLength() < 28 + 54 || Src.Left(2) != "IL" ) { Error = "заголовок галереи"; return false; }
	unsigned cx = (unsigned char)Src[10] | ((unsigned char)Src[11] << 8);
	unsigned cy = (unsigned char)Src[12] | ((unsigned char)Src[13] << 8);
	const int bmp = 28;
	unsigned long off = Dword(Src, bmp + 10);
	long w = (long)Dword(Src, bmp + 18), h = (long)Dword(Src, bmp + 22);
	unsigned bpp = (unsigned char)Src[bmp + 28] | ((unsigned char)Src[bmp + 29] << 8);
	if( cx == 0 || cy == 0 || w <= 0 || h <= 0 || bpp != 16 ) { Error = "формат миниатюр"; return false; }
	unsigned long stride = (unsigned long)w * 2;
	unsigned long image = stride * h;
	int tail = bmp + (int)(off + image);
	if( tail + 8 > Src.GetLength() ) { Error = "длина галереи"; return false; }
	unsigned long n = Dword(Src, tail + 4);
	if( tail + 8 + 4 * (int)n > Src.GetLength() ) { Error = "хвост галереи"; return false; }
	int cols = (int)(w / cx);

	int count = Old.GetSize();
	int capacity = 10;                 //2 initial + 8 to grow, as in the corpus
	while( capacity < count ) capacity += 8;
	int rows = (capacity + cols - 1) / cols;
	long nh = rows * (long)cy;

	GCString head = Src.Left(28);
	SetWord(head, 4, count);
	SetWord(head, 6, capacity);
	GCString hdr = Src.Mid(bmp, off);
	SetDword(hdr, 22, nh);
	SetDword(hdr, 34, stride * nh);
	GCString px;
	char* p = px.GetBufferSetLength((int)(stride * nh));
	memset(p, 0, stride * nh);
	const char* sp = (LPCSTR)Src + bmp + off;
	for( int k = 0; k < count; k++ )
	{
		unsigned long idx;
		for( idx = 0; idx < n; idx++ ) if( Dword(Src, tail + 8 + 4 * idx) == Old[k] ) break;
		if( idx == n ) { px.ReleaseBuffer((int)(stride * nh)); Error.Format("нет миниатюры картинки %08lx", Old[k]); return false; }
		long sx = (long)(idx % cols) * cx, sy = (long)(idx / cols) * cy;
		long dx = (long)(k % cols) * cx, dy = (long)(k / cols) * cy;
		for( unsigned y = 0; y < cy; y++ )   //rows are bottom-up
			memcpy(p + (nh - 1 - (dy + y)) * stride + dx * 2,
				sp + (h - 1 - (sy + y)) * stride + sx * 2, cx * 2);
	}
	px.ReleaseBuffer((int)(stride * nh));

	GCString t;
	PutDword(t, Dword(Src, tail));            //0xFF000001
	PutDword(t, count);
	for( int k = 0; k < count; k++ ) PutDword(t, 0x1F000001UL + k);
	PutDword(t, 0x0F000000UL + count);        //the last number given
	t += (char)2;
	t += "{}";                                //no names
	Out = head + hdr + px + t;
	return true;
}

//a one-image palette icon rewritten as the Configurator writes it: 32 bits
//a point, BI_BITFIELDS, the colours from the palette with alpha 0, the
//AND mask as it is
void IconTo32(GCString& Pic, int At)
{
	const unsigned char* p = (const unsigned char*)(LPCSTR)Pic + At;
	int len = Pic.GetLength() - At;
	if( len < 22 + 40 || p[0] || p[1] || p[2] != 1 || p[3] || p[4] != 1 || p[5] ) return;
	unsigned long off = Dword(Pic, At + 18);
	if( off + 40 > (unsigned long)len ) return;
	int b = At + (int)off;
	unsigned long bsize = Dword(Pic, b);
	long bw = (long)Dword(Pic, b + 4), bh = (long)Dword(Pic, b + 8);
	unsigned bpp = (unsigned char)Pic[b + 14] | ((unsigned char)Pic[b + 15] << 8);
	if( bpp == 0 || bpp > 8 || bw <= 0 || bh <= 0 ) return;
	unsigned long used = Dword(Pic, b + 32);
	if( used == 0 ) used = 1UL << bpp;
	long H = bh / 2;
	unsigned long xstride = ((bw * bpp + 31) / 32) * 4, astride = ((bw + 31) / 32) * 4;
	int pal = b + (int)bsize, xor_ = pal + 4 * (int)used, and_ = xor_ + (int)(xstride * H);
	if( and_ + (int)(astride * H) > Pic.GetLength() ) return;
	const unsigned char* q = (const unsigned char*)(LPCSTR)Pic;
	GCString px;
	for( long y = 0; y < H; y++ )
		for( long x = 0; x < bw; x++ )
		{
			unsigned long bit = (unsigned long)x * bpp;
			unsigned v = (q[xor_ + y * xstride + bit / 8] >> (8 - bpp - bit % 8)) & ((1 << bpp) - 1);
			px += (char)q[pal + 4 * v];
			px += (char)q[pal + 4 * v + 1];
			px += (char)q[pal + 4 * v + 2];
			px += (char)0;
		}
	GCString mask = Pic.Mid(and_, (int)(astride * H));
	GCString img;
	PutDword(img, 40); PutDword(img, bw); PutDword(img, bh);
	img += (char)1; img += (char)0; img += (char)32; img += (char)0;
	PutDword(img, 3);                                  //BI_BITFIELDS
	PutDword(img, px.GetLength() + mask.GetLength());
	PutDword(img, 0); PutDword(img, 0); PutDword(img, 0); PutDword(img, 0);
	PutDword(img, 0x00FF0000UL); PutDword(img, 0x0000FF00UL); PutDword(img, 0x000000FFUL);
	img += px + mask;
	GCString ico;
	ico += (char)0; ico += (char)0; ico += (char)1; ico += (char)0; ico += (char)1; ico += (char)0;
	ico += (char)q[At + 6]; ico += (char)q[At + 7];   //width, height
	ico += (char)0; ico += (char)q[At + 9];           //colours, reserved
	ico += (char)1; ico += (char)0; ico += (char)32; ico += (char)0;
	PutDword(ico, img.GetLength());
	PutDword(ico, 22);
	Pic = GCString(Pic.Left(At)) + ico + img;
}

//the reserved byte of every palette entry of a BMP set to 0xFF
void FixPalette(GCString& Pic, int At)
{
	if( Pic.GetLength() < At + 54 || Pic.Mid(At, 2) != "BM" ) return;
	unsigned long biSize = Dword(Pic, At + 14);
	unsigned bpp = (unsigned char)Pic[At + 28] | ((unsigned char)Pic[At + 29] << 8);
	unsigned long used = Dword(Pic, At + 46);
	unsigned long colours = used ? used : (bpp <= 8 ? (1UL << bpp) : 0);
	int pal = At + 14 + (int)biSize;
	for( unsigned long i = 0; i < colours && pal + 4 * (int)i + 3 < Pic.GetLength(); i++ )
		Pic.SetAt(pal + 4 * (int)i + 3, (char)0xFF);
}

bool ReadStream(CStorage& Root, const GCString& Path, GCString& Data, bool Clean)
{
	CStorage s(Root);
	if( !s.OpenStream(Path) ) return false;
	if( Clean ) s.GetCleanStream();
	return s.ReadAll(Data);
}

} //namespace

/* One external report from the storage of a report or data processor
   (StorPath, e.g. "Report\Report_Number5162"); MMS is the text of the
   Main MetaData Stream of the configuration. */
static bool ExportOne(CStorage& Root, CStorage& TypedText, const GCString& StorPath, int ID,
	const GCString& MMS, const GCString& Gallery, const GCString& FileName, GCString& Error)
{
	CArray<unsigned long, unsigned long> Known;
	GalleryIDs(Gallery, Known);
	CNameTableRecord* nt_wb = NameTable.Find(NT_Workbook);
	GCString WB = StorPath + "\\" + nt_wb->StorageName;

	CStorage Ert;
	if( !Ert.CreateCompound(FileName) ) { Error = "не создан файл"; return false; }

	//metadata
	GCString Text;
	if( !ReduceMMS(MMS, Text) ) { Error = "не разобран поток метаданных"; return false; }
	Ert.StreamFromString(GCString("Main MetaData Stream"), Text);
	Ert.AddZeroByte();
	Ert.CloseStream();

	//the work book: every stream as it is, the form with new picture numbers
	CStorage Book(Root);
	if( !Book.Open(WB) ) { Error = "нет рабочей книги"; return false; }
	CStringArray Names;
	Book.StreamNames(Names);
	CArray<unsigned long, unsigned long> Pictures;
	GCString Contents;
	for( int i = 0; i < Names.GetSize(); i++ )
	{
		GCString name = Names[i], data;
		if( name == "Container.Contents" )
		{
			ReadStream(Book, name, Contents, false);
			continue;
		}
		if( name == "Dialog Stream" )
		{
			GCString form, renumbered;
			if( ReadStream(Book, name, form, true) && RenumberPictures(form, renumbered, Pictures, Known) && Pictures.GetSize() )
			{
				Ert.StreamFromString(name, renumbered);
				Ert.CloseStream();
				continue;
			}
		}
		if( !ReadStream(Book, name, data, false) ) { Error = "не прочитан поток " + name; return false; }
		Ert.WriteRaw(name, data);
	}

	//the description
	GCString Descr, Help;
	Help.Format("%s%i\\%s", (LPCSTR)NameTable.Find(NT_UserHelpStor)->StorageName, ID,
		(LPCSTR)NameTable.Find(NT_UserHelp)->StorageName);
	if( !ReadStream(TypedText, Help, Descr, false) )   //TypedText is open already
		CStorage::Deflate(GCString(""), Descr);
	Ert.WriteRaw(GCString("Inplace description"), Descr);

	//Container.Contents: the metadata, the streams of the work book and the
	//description after the module
	GCString Items = Contents;
	int a = Items.Find(',');
	int b = Items.ReverseFind('}');
	Items = a >= 0 && b > a ? Items.Mid(a, b - a) : GCString("");
	GCString DescrItem = ",{\"MetaDataDescription\",\"Inplace description\",\"ќписание\",\"\"}";
	int m = Items.Find("{\"TextDocument\"");
	if( m >= 0 )
	{
		int e = Items.Find('}', m);
		Items.Insert(e + 1, DescrItem);
	}
	else
		Items += DescrItem;
	GCString CC = "{\"Container.Contents\",{\"MetaDataHolderContainer\",\"Main MetaData Stream\",\"Main MetaData Stream\",\"\"}"
		+ Items + "}\n";
	Ert.WriteRaw(GCString("Container.Contents"), CC);

	//pictures
	if( Pictures.GetSize() )
	{
		GCString Pic = NameTable.Find(NT_PictureGallery)->StorageName;
		GCString NewGallery;
		if( !BuildGallery(Gallery, Pictures, NewGallery, Error) ) return false;
		CStorage PicStor(Ert);
		PicStor.Create(Pic);
		PicStor.WriteRaw(GCString("Gallery"), NewGallery);
		PicStor.WriteRaw(GCString("Container.Contents"),
			GCString("{\"Container.Contents\",{\"PictureGalleryContainer\",\"Gallery\",\"Gallery\",\"\"}}\n"));
		for( int k = 0; k < Pictures.GetSize(); k++ )
		{
			GCString src, packed, unpacked, name;
			name.Format("%s\\__Picture.%08lx", (LPCSTR)Pic, Pictures[k]);
			if( !ReadStream(Root, name, packed, false) ) { Error.Format("нет картинки %08lx", Pictures[k]); return false; }
			CStorage::Inflate(packed, unpacked);
			FixPalette(unpacked, 8);    //after the signature and the size
			IconTo32(unpacked, 8);
			SetDword(unpacked, 4, unpacked.GetLength() - 8);
			CStorage::Deflate(unpacked, packed);
			name.Format("__Picture.%08lx", 0x1F000001UL + k);
			PicStor.WriteRaw(name, packed);
		}
		PicStor.Close();
	}
	Ert.Close(true);
	return true;
}

void ExportERTs(CMMS& MMS, GCString& Dir, CStorage& Root, CStorage& TypedText)
{
	GCString Text;
	CNameTableRecord* nt_mms = NameTable.Find(NT_MainMetadataStream);
	if( !ReadStream(Root, "Metadata\\" + nt_mms->StorageName, Text, true) )
	{
		Msg(0, "WARN: внешние отчЄты не записаны: нет потока метаданных");
		return;
	}
	GCString Gallery;
	ReadStream(Root, NameTable.Find(NT_PictureGallery)->StorageName + "\\Gallery", Gallery, false);
	CMMSReportList* Lists[2] = {&MMS.ReportList, &MMS.CalcVarList};
	for( int l = 0; l < 2; l++ )
	{
		CMMSReportList& List = *Lists[l];
		if( List.GetNChildren() == 0 ) continue;
		CNameTableRecord* nt_rec = NameTable.Find(List.sID);
		GCString ListDir = nt_rec->PrepareDirName(Dir);
		for( int i = 0; i < List.GetNChildren(); i++ )
		{
			CMMSReport* rpt = (CMMSReport*)List.GetChild(i);
			GCString ObjDir = rpt->GetObjectDir(ListDir);
			if( !DirectoryExist(ObjDir) ) continue;      //filtered out
			GCString StorPath, FileName, Error;
			StorPath.Format("%s\\%s_Number%i", (LPCSTR)nt_rec->StorageName, (LPCSTR)nt_rec->StorageName, rpt->ID);
			FileName = ObjDir + "\\" + rpt->sID + ".ert";
			Root.AddToExtractedList(FileName, NULL);   //kept by the pruning of the tree
			try
			{
				if( !ExportOne(Root, TypedText, StorPath, rpt->ID, Text, Gallery, FileName, Error) )
					Msg(0, "WARN: %s: внешний отчЄт не записан: %s", (LPCSTR)FileName, (LPCSTR)Error);
			}
			catch( ... )
			{
				Msg(0, "WARN: %s: внешний отчЄт не записан", (LPCSTR)FileName);
			}
		}
	}
}
