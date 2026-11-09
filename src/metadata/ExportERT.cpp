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
     0x1F000001, the form referring to the new numbers; the picture streams
     are copied as they are, the thumbnails drawn again as the Configurator
     draws them (DrawThumb).

   The form is left as it is (the Configurator saves it again with the
   client area offsets of its own computer). */

#include <string.h>
#include <stdlib.h>
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

//where the number of pictures stands in the tail of a gallery: after the
//mark 0xFF000001, or right at the tail in galleries of older releases
int ListAt(const GCString& Src, int Tail)
{
	return Dword(Src, Tail) == 0xFF000001UL ? Tail + 4 : Tail;
}

//the picture numbers listed in a gallery
void GalleryIDs(const GCString& Src, CArray<unsigned long, unsigned long>& IDs)
{
	if( Src.GetLength() < 28 + 54 || Src.Left(2) != "IL" ) return;
	const int bmp = 28;
	unsigned long off = Dword(Src, bmp + 10);
	long w = (long)Dword(Src, bmp + 18), h = (long)Dword(Src, bmp + 22);
	unsigned bpp = (unsigned char)Src[bmp + 28] | ((unsigned char)Src[bmp + 29] << 8);
	long stride = ((w * bpp + 31) / 32) * 4;
	int tail = bmp + (int)(off + (unsigned long)(stride * h));
	if( w <= 0 || h <= 0 || bpp == 0 || tail + 8 > Src.GetLength() ) return;
	int list = ListAt(Src, tail);
	unsigned long n = Dword(Src, list);
	for( unsigned long i = 0; i < n && list + 8 + 4 * (int)i <= Src.GetLength(); i++ )
		IDs.Add(Dword(Src, list + 4 + 4 * (int)i));
}

//A picture of the gallery decoded for drawing its thumbnail: the colours
//of the points top down, and for an icon its AND mask
struct CPicture
{
	int W, H;
	bool Icon;
	std::vector<unsigned long> RGB;     //0x00RRGGBB
	std::vector<unsigned char> Mask;    //1 where the AND mask is set
};

unsigned Word(const unsigned char* p) { return p[0] | (p[1] << 8); }
unsigned long Dword(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned long)p[3] << 24); }

//the points of a device independent bitmap at Bi (BITMAPINFOHEADER) of
//Rows rows; false for a compressed one or a depth not met in galleries
bool DecodeDIB(const unsigned char* d, int Len, int Bi, int Rows, int MaskRows, CPicture& P)
{
	if( Bi + 40 > Len ) return false;
	unsigned long size = Dword(d + Bi);
	long w = (long)Dword(d + Bi + 4), h = (long)Dword(d + Bi + 8);
	unsigned bpp = Word(d + Bi + 14);
	unsigned long comp = Dword(d + Bi + 16), used = Dword(d + Bi + 32);
	bool topdown = h < 0;
	if( Rows ) h = Rows; else if( topdown ) h = -h;
	if( comp != 0 || w <= 0 || h <= 0 || w > 4096 || h > 4096 ) return false;
	if( bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32 ) return false;
	unsigned long colours = bpp <= 8 ? (used ? used : 1UL << bpp) : 0;
	int pal = Bi + (int)size;
	int bits = pal + 4 * (int)colours;
	long stride = ((w * bpp + 31) / 32) * 4;
	long mstride = ((w + 31) / 32) * 4;
	if( bits + stride * h + (MaskRows ? mstride * MaskRows : 0) > Len ) return false;
	P.W = (int)w; P.H = (int)h;
	P.RGB.assign(w * h, 0);
	P.Mask.assign(w * h, 0);
	for( long y = 0; y < h; y++ )
	{
		const unsigned char* row = d + bits + stride * (topdown ? y : h - 1 - y);
		for( long x = 0; x < w; x++ )
		{
			unsigned long c;
			if( bpp <= 8 )
			{
				unsigned long bit = (unsigned long)x * bpp;
				unsigned v = (row[bit / 8] >> (8 - bpp - bit % 8)) & ((1 << bpp) - 1);
				if( v >= colours ) v = 0;
				const unsigned char* q = d + pal + 4 * v;
				c = ((unsigned long)q[2] << 16) | (q[1] << 8) | q[0];
			}
			else if( bpp == 16 )     //5:5:5
			{
				unsigned v = Word(row + x * 2);
				c = ((unsigned long)((v >> 10) & 31) << 19) | (((v >> 5) & 31) << 11) | ((v & 31) << 3);
			}
			else
			{
				const unsigned char* q = row + x * (bpp / 8);
				c = ((unsigned long)q[2] << 16) | (q[1] << 8) | q[0];
			}
			P.RGB[y * w + x] = c;
		}
	}
	if( MaskRows )
	{
		int m = bits + (int)(stride * h);
		for( long y = 0; y < h; y++ )
			for( long x = 0; x < w; x++ )
				P.Mask[y * w + x] = (d[m + mstride * (h - 1 - y) + x / 8] >> (7 - x % 8)) & 1;
	}
	return true;
}

//a picture of the gallery: a BMP or the first image of an icon
bool DecodePicture(const GCString& Pic, CPicture& P)
{
	const unsigned char* d = (const unsigned char*)(LPCSTR)Pic;
	int len = Pic.GetLength();
	if( len >= 54 && d[0] == 'B' && d[1] == 'M' )
	{
		P.Icon = false;
		return DecodeDIB(d, len, 14, 0, 0, P);
	}
	if( len >= 22 && Word(d) == 0 && Word(d + 2) == 1 && Word(d + 4) >= 1 )
	{
		P.Icon = true;
		unsigned long off = Dword(d + 18);
		if( off + 40 > (unsigned long)len ) return false;
		long h2 = (long)Dword(d + off + 8);
		if( h2 <= 0 || h2 % 2 ) return false;
		return DecodeDIB(d, len, (int)off, (int)(h2 / 2), (int)(h2 / 2), P);
	}
	return false;
}

//the margin before a picture of size Num/Den in a cell of Cell points:
//centred, at least 16 points, rounded half down, the same on both sides
int Margin(long Num, long Den, int Cell)
{
	if( Num < 16 * Den ) return (Cell - 16) / 2;
	long t = Num + Den - (long)Cell * Den;          //ceil((Cell*Den - Num - Den) / 2Den)
	return t >= 0 ? -(int)(t / (2 * Den)) : (int)((-t + 2 * Den - 1) / (2 * Den));
}

/* The thumbnail of a picture as the Configurator draws it when it saves
   an external report: on white, the picture scaled to fit the cell keeping
   its proportions, centred; every point takes the point of the picture under
   its middle, without blending; a picture within a point of the size it
   gets is copied as it is; an icon is drawn through its mask (the white
   AND the mask, XOR the colour); 16-bit colours 5:5:5, the low bits cut. */
void DrawThumb(const CPicture& P, int Cell, std::vector<unsigned short>& Out)
{
	Out.assign(Cell * Cell, 0x7FFF);
	int x0, y0;
	if( P.W >= P.H ) { x0 = Margin(Cell, 1, Cell); y0 = Margin((long)Cell * P.H, P.W, Cell); }
	else { x0 = Margin((long)Cell * P.W, P.H, Cell); y0 = Margin(Cell, 1, Cell); }
	int dw = Cell - 2 * x0, dh = Cell - 2 * y0;
	bool copy = abs(P.W - dw) <= 1 && abs(P.H - dh) <= 1;
	for( int y = 0; y < dh; y++ )
	{
		long sy = copy ? y : (long)(2 * y + 1) * P.H / (2 * dh);
		if( sy >= P.H ) sy = P.H - 1;
		for( int x = 0; x < dw; x++ )
		{
			long sx = copy ? x : (long)(2 * x + 1) * P.W / (2 * dw);
			if( sx >= P.W ) sx = P.W - 1;
			unsigned long c = P.RGB[sy * P.W + sx];
			unsigned short v = (unsigned short)((((c >> 19) & 31) << 10) | (((c >> 11) & 31) << 5) | ((c >> 3) & 31));
			if( P.Mask[sy * P.W + sx] ) v ^= 0x7FFF;
			Out[(y0 + y) * Cell + x0 + x] = v;
		}
	}
}

unsigned short To555(unsigned long c)
{
	return (unsigned short)((((c >> 19) & 31) << 10) | (((c >> 11) & 31) << 5) | ((c >> 3) & 31));
}

/* The gallery of the external report, written as the Configurator writes
   it whatever the gallery of the configuration is: an image list of 80x80
   cells, 4 in a row, 16 bits a point, growing by 8, the count and the
   capacity (10 at first; an image that does not fit makes it the number of
   the image + 8); a thumbnail drawn for every picture (DrawThumb), or for a
   picture that cannot be decoded (a metafile) the cell of the configuration;
   the tail: the mark 0xFF000001, the numbers, the last number given and an
   empty list of names. */
bool BuildGallery(const GCString& Src, const CArray<unsigned long, unsigned long>& Old,
	const std::vector<GCString>& Pics, GCString& Out, GCString& Error)
{
	const int Cell = 80, Cols = 4;
	//the gallery of the configuration, for the cells of undecodable pictures
	CPicture strip;
	bool haveStrip = false;
	int srcCols = 0, list = 0;
	unsigned long n = 0;
	unsigned scx = 0, scy = 0;
	if( Src.GetLength() >= 28 + 54 && Src.Left(2) == "IL" )
	{
		scx = (unsigned char)Src[10] | ((unsigned char)Src[11] << 8);
		scy = (unsigned char)Src[12] | ((unsigned char)Src[13] << 8);
		const unsigned char* d = (const unsigned char*)(LPCSTR)Src;
		unsigned long off = Dword(Src, 28 + 10);
		long w = (long)Dword(Src, 28 + 18), h = (long)Dword(Src, 28 + 22);
		unsigned bpp = Word(d + 28 + 28);
		long stride = ((w * bpp + 31) / 32) * 4;
		int tail = 28 + (int)off + (int)(stride * h);
		if( scx && scy && w > 0 && h > 0 && tail + 8 <= Src.GetLength()
			&& DecodeDIB(d + 28, Src.GetLength() - 28, 14, 0, 0, strip) )
		{
			list = ListAt(Src, tail);
			n = Dword(Src, list);
			srcCols = (int)(w / scx);
			haveStrip = srcCols > 0 && list + 4 + 4 * (int)n <= Src.GetLength();
		}
	}

	int count = Old.GetSize();
	int capacity = 10;
	for( int i = 1; i <= count; i++ )
		if( i > capacity ) capacity = i + 8;
	int rows = (capacity + Cols - 1) / Cols;
	long w = Cols * Cell, nh = rows * (long)Cell;
	unsigned long stride = (unsigned long)w * 2;

	GCString head = "IL";
	head += (char)1; head += (char)1;                    //version
	head += (char)0; head += (char)0; head += (char)0; head += (char)0;
	SetWord(head, 4, count);
	SetWord(head, 6, capacity);
	head += (char)8; head += (char)0;                    //grow
	head += (char)Cell; head += (char)0; head += (char)Cell; head += (char)0;
	PutDword(head, 0xFFFFFFFFUL);                        //no background colour
	head += (char)0x10; head += (char)0;                 //ILC_COLOR16
	for( int i = 0; i < 4; i++ ) { head += (char)0xFF; head += (char)0xFF; }   //no overlays

	GCString hdr = "BM";
	PutDword(hdr, 54); PutDword(hdr, 0); PutDword(hdr, 54);
	PutDword(hdr, 40); PutDword(hdr, w); PutDword(hdr, nh);
	hdr += (char)1; hdr += (char)0; hdr += (char)16; hdr += (char)0;
	PutDword(hdr, 0); PutDword(hdr, stride * nh);
	PutDword(hdr, 0); PutDword(hdr, 0); PutDword(hdr, 0); PutDword(hdr, 0);

	GCString px;
	char* p = px.GetBufferSetLength((int)(stride * nh));
	memset(p, 0, stride * nh);
	for( int k = 0; k < count; k++ )
	{
		long dx = (long)(k % Cols) * Cell, dy = (long)(k / Cols) * Cell;
		std::vector<unsigned short> cell;
		CPicture pic;
		if( DecodePicture(Pics[k], pic) )
			DrawThumb(pic, Cell, cell);
		else
		{
			unsigned long idx = n;
			if( haveStrip && scx == (unsigned)Cell && scy == (unsigned)Cell )
				for( idx = 0; idx < n; idx++ ) if( Dword(Src, list + 4 + 4 * idx) == Old[k] ) break;
			if( idx == n ) { px.ReleaseBuffer((int)(stride * nh)); Error.Format("нет миниатюры картинки %08lx", Old[k]); return false; }
			long sx = (long)(idx % srcCols) * Cell, sy = (long)(idx / srcCols) * Cell;
			cell.resize(Cell * Cell);
			for( int y = 0; y < Cell; y++ )
				for( int x = 0; x < Cell; x++ )
					cell[y * Cell + x] = To555(strip.RGB[(sy + y) * strip.W + sx + x]);
		}
		for( int y = 0; y < Cell; y++ )      //rows are bottom-up
			for( int x = 0; x < Cell; x++ )
			{
				char* q = p + (nh - 1 - (dy + y)) * stride + (dx + x) * 2;
				q[0] = (char)(cell[y * Cell + x] & 0xFF);
				q[1] = (char)(cell[y * Cell + x] >> 8);
			}
	}
	px.ReleaseBuffer((int)(stride * nh));

	GCString t;
	PutDword(t, 0xFF000001UL);
	PutDword(t, count);
	for( int k = 0; k < count; k++ ) PutDword(t, 0x1F000001UL + k);
	PutDword(t, 0x0F000000UL + count);        //the last number given
	t += (char)2;
	t += "{}";                                //no names
	Out = head + hdr + px + t;
	return true;
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
	//the pictures of the gallery with a stream (a picture listed without one
	//is lost as well as one not listed)
	CArray<unsigned long, unsigned long> Listed, Known;
	GalleryIDs(Gallery, Listed);
	GCString PicStorName = NameTable.Find(NT_PictureGallery)->StorageName;
	for( int i = 0; i < Listed.GetSize(); i++ )
	{
		GCString name;
		name.Format("%s\\__Picture.%08lx", (LPCSTR)PicStorName, Listed[i]);
		CStorage s(Root);
		if( s.OpenStream(name) ) Known.Add(Listed[i]);
	}
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
		std::vector<GCString> Packed(Pictures.GetSize()), Unpacked(Pictures.GetSize());
		for( int k = 0; k < Pictures.GetSize(); k++ )
		{
			GCString name;
			name.Format("%s\\__Picture.%08lx", (LPCSTR)Pic, Pictures[k]);
			if( !ReadStream(Root, name, Packed[k], false) ) { Error.Format("нет картинки %08lx", Pictures[k]); return false; }
			CStorage::Inflate(Packed[k], Unpacked[k]);
			Unpacked[k] = Unpacked[k].GetLength() > 8 ? GCString(Unpacked[k].Mid(8)) : GCString("");   //after the signature and the size
		}
		GCString NewGallery;
		if( !BuildGallery(Gallery, Pictures, Unpacked, NewGallery, Error) ) return false;
		CStorage PicStor(Ert);
		PicStor.Create(Pic);
		PicStor.WriteRaw(GCString("Gallery"), NewGallery);
		PicStor.WriteRaw(GCString("Container.Contents"),
			GCString("{\"Container.Contents\",{\"PictureGalleryContainer\",\"Gallery\",\"Gallery\",\"\"}}\n"));
		for( int k = 0; k < Pictures.GetSize(); k++ )   //as they are
		{
			GCString name;
			name.Format("__Picture.%08lx", 0x1F000001UL + k);
			PicStor.WriteRaw(name, Packed[k]);
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
