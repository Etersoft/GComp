// SPDX-License-Identifier: GPL-2.0-or-later
/* Views of GUIDData and TagStream (docs/md-file.md, "GUIDData и TagStream").

   GUIDData is a 32-bit count N and N GUIDs of 16 bytes.

   TagStream is encrypted: RC4 with the key of the encrypted metadata stream,
   each byte also XORed with the previous byte of the cipher text (the first
   with S[0] after the key schedule). 1C swaps the bytes of the RC4 state by
   XOR, so a swap of a byte with itself zeroes it, and so does this code.
   Inside is an MFC CString with the brace text
   {{"_HASH_STRING_","..."},{"_AUTOR_SHOR_STRING_","..."},...}, quotes in
   values doubled; the logo and the splash are DIBs coded 4 characters
   (code - 0x20) to 3 bytes. */

#include <string.h>
#include "Declarations.h"
#include "ServiceText.h"

namespace {

unsigned long Dword(const unsigned char* b)
{
	return (unsigned long)b[0] | ((unsigned long)b[1] << 8) |
		((unsigned long)b[2] << 16) | ((unsigned long)b[3] << 24);
}

void Put16(GCString& s, unsigned v)
{
	s += (char)(v & 0xFF);
	s += (char)((v >> 8) & 0xFF);
}

void Put32(GCString& s, unsigned long v)
{
	Put16(s, (unsigned)(v & 0xFFFF));
	Put16(s, (unsigned)(v >> 16));
}

unsigned long Crc32(const unsigned char* d, long n)
{
	unsigned long c = 0xFFFFFFFFUL;
	for( long i = 0; i < n; i++ )
	{
		c ^= d[i];
		for( int k = 0; k < 8; k++ )
			c = (c >> 1) ^ (0xEDB88320UL & (0UL - (c & 1)));
	}
	return c ^ 0xFFFFFFFFUL;
}

void XorSwap(unsigned char* s, int i, int j)
{
	s[i] ^= s[j];
	s[j] ^= s[i];
	s[i] ^= s[j];
}

void Decrypt(const unsigned char* c, long n, unsigned char* p)
{
	static const unsigned char Key[16] = {0x60, 0x46, 0xD2, 0x72, 0x64, 0x25, 0x03, 0x00,
		0x09, 0x89, 0x00, 0xC0, 0xDD, 0x3B, 0xE6, 0x36};
	unsigned char s[256];
	int i, j = 0;
	for( i = 0; i < 256; i++ ) s[i] = (unsigned char)i;
	for( i = 0; i < 256; i++ )
	{
		j = (j + s[i] + Key[i % 16]) & 0xFF;
		XorSwap(s, i, j);
	}
	unsigned char prev = s[0];
	i = j = 0;
	for( long x = 0; x < n; x++ )
	{
		i = (i + 1) & 0xFF;
		j = (j + s[i]) & 0xFF;
		XorSwap(s, i, j);
		p[x] = (unsigned char)(c[x] ^ s[(s[i] + s[j]) & 0xFF] ^ prev);
		prev = c[x];
	}
}

//A DIB coded 4 characters to 3 bytes as a BMP file; empty when it is not one
GCString PictureToBmp(const GCString& Coded)
{
	GCString dib;
	int n = Coded.GetLength();
	for( int i = 0; i + 3 < n; i += 4 )
	{
		unsigned a = ((unsigned char)Coded[i] - 0x20) & 0x3F, b = ((unsigned char)Coded[i+1] - 0x20) & 0x3F;
		unsigned c = ((unsigned char)Coded[i+2] - 0x20) & 0x3F, d = ((unsigned char)Coded[i+3] - 0x20) & 0x3F;
		dib += (char)((a << 2) | (b >> 4));
		dib += (char)(((b << 4) | (c >> 2)) & 0xFF);
		dib += (char)(((c << 6) | d) & 0xFF);
	}
	const unsigned char* h = (const unsigned char*)(LPCSTR)dib;
	if( dib.GetLength() < 40 || Dword(h) != 40 ) return "";
	unsigned bpp = h[14] | (h[15] << 8);
	unsigned long colours = Dword(h + 32);
	if( colours == 0 && bpp <= 8 ) colours = 1UL << bpp;
	unsigned long image = Dword(h + 20);
	unsigned long size = 40 + colours * 4 + image;
	if( image && size < (unsigned long)dib.GetLength() ) dib = dib.Left((int)size);
	GCString bmp = "BM";
	Put32(bmp, 14 + dib.GetLength());
	Put32(bmp, 0);
	Put32(bmp, 14 + 40 + colours * 4);
	return bmp + dib;
}

//"64x64, 8 бит, 5160 байт, CRC32 1a2b3c4d" of a coded picture
GCString PictureInfo(const GCString& Coded, const GCString& Bmp)
{
	GCString s;
	if( Bmp.IsEmpty() )
	{
		s.Format("%d символов, не картинка", Coded.GetLength());
		return s;
	}
	const unsigned char* h = (const unsigned char*)(LPCSTR)Bmp + 14;
	long w = (long)Dword(h + 4), hh = (long)Dword(h + 8);
	if( hh < 0 ) hh = -hh;
	s.Format("%ldx%ld, %u бит, %d байт, CRC32 %08lx", w, hh, h[14] | (h[15] << 8),
		Bmp.GetLength(), Crc32((const unsigned char*)(LPCSTR)Bmp, Bmp.GetLength()));
	return s;
}

//A value on one line, or on the following lines indented by a tab
GCString Value(const GCString& v)
{
	if( v.IsEmpty() ) return "";
	if( v.Find('\n') < 0 && v.Find('\r') < 0 ) return " " + v;
	GCString r = "\r\n\t";
	for( int i = 0; i < v.GetLength(); i++ )
	{
		if( v[i] == '\r' && i + 1 < v.GetLength() && v[i+1] == '\n' ) { r += "\r\n\t"; i++; }
		else if( v[i] == '\n' || v[i] == '\r' ) r += "\r\n\t";
		else r += v[i];
	}
	return r;
}

} //namespace

int GuidDataToText(const GCString& Data, GCString& Text, GCString& Error)
{
	const unsigned char* d = (const unsigned char*)(LPCSTR)Data;
	long n = Data.GetLength();
	if( n < 4 ) { Error = "поток короче 4 байт"; return -1; }
	unsigned long count = Dword(d);
	GCString s;
	Text = "// Вид потока GUIDData (история сохранений конфигурации) для просмотра\r\n"
		"// изменений; GComp при сборке его не читает: поток собирается из GUIDData.\r\n";
	s.Format("Записей: %lu\r\n", count);
	Text += s;
	unsigned long have = (unsigned long)(n - 4) / 16;
	long p = 4;
	for( unsigned long i = 0; i < count && i < have; i++, p += 16 )
	{
		const unsigned char* g = d + p;
		s.Format("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\r\n", Dword(g),
			g[4] | (g[5] << 8), g[6] | (g[7] << 8), g[8], g[9], g[10], g[11], g[12], g[13], g[14], g[15]);
		Text += s;
	}
	if( have < count ) { s.Format("В потоке только %lu записей\r\n", have); Text += s; }
	else if( p < n ) { s.Format("После записей: %ld байт\r\n", n - p); Text += s; }
	return 1;
}

int TagStreamToText(const GCString& Data, GCString& Text, GCString& Logo,
	GCString& Splash, GCString& Error)
{
	long n = Data.GetLength();
	GCString Plain;
	char* buf = Plain.GetBufferSetLength((int)n);
	Decrypt((const unsigned char*)(LPCSTR)Data, n, (unsigned char*)buf);
	Plain.ReleaseBuffer((int)n);
	const unsigned char* p = (const unsigned char*)(LPCSTR)Plain;

	//the CString
	long o = 1, len = n ? p[0] : 0;
	if( len == 0xFF && n >= 3 )
	{
		len = p[1] | (p[2] << 8);
		o = 3;
		if( len == 0xFFFF && n >= 7 ) { len = (long)Dword(p + 3); o = 7; }
	}
	if( n == 0 || o + len > n ) { Error = "не расшифровалась строка"; return -1; }
	GCString t((const char*)p + o, (int)len);
	long tail = n - o - len;

	static const struct { const char* tag; const char* name; } names[] = {
		{"_HASH_STRING_", "СвёрткаПароля"}, {"_AUTOR_SHOR_STRING_", "КраткаяИнформация"},
		{"_AUTOR_LONG_STRING_", "РазвернутаяИнформация"}, {"_AUTOR_LOGO_INDEX_", "Логотип"},
		{"_AUTOR_SPLASH_INDEX_", "Заставка"}, {"_AUTOR_LOGO_PICTURE_POS", "ПоложениеЛоготипа"},
		{NULL, NULL}};

	Text = "// Вид потока TagStream (закладки «Автор» и «Заставка» свойств конфигурации)\r\n"
		"// для просмотра изменений; GComp при сборке его не читает: поток собирается\r\n"
		"// из TagStream. Картинки — рядом, в TagStream.Логотип.bmp и TagStream.Заставка.bmp.\r\n";
	Logo = Splash = "";
	//{{"tag","value"},{"tag","value"}}
	int i = 0, m = t.GetLength();
	if( m < 2 || t[0] != '{' ) { Error = "нет списка в скобках"; return -1; }
	i = 1;
	while( i < m && t[i] == '{' )
	{
		GCString parts[2];
		i++;
		for( int k = 0; k < 2; k++ )
		{
			if( i >= m || t[i] != '"' ) { Error = "нет кавычки"; return -1; }
			i++;
			while( i < m )
			{
				if( t[i] == '"' )
				{
					if( i + 1 < m && t[i+1] == '"' ) { parts[k] += '"'; i += 2; continue; }
					break;
				}
				parts[k] += t[i++];
			}
			if( i >= m ) { Error = "строка не закрыта"; return -1; }
			i++;
			if( k == 0 ) { if( i >= m || t[i] != ',' ) { Error = "нет запятой"; return -1; } i++; }
		}
		if( i >= m || t[i] != '}' ) { Error = "нет закрывающей скобки"; return -1; }
		i++;
		if( i < m && t[i] == ',' ) i++;

		GCString name = parts[0];
		for( int k = 0; names[k].tag; k++ )
			if( parts[0] == names[k].tag ) name = names[k].name;
		GCString v = parts[1];
		if( parts[0] == "_HASH_STRING_" )
		{
			//MD5 of an empty string; short values like A208 are met in
			//configurations that are not encrypted, their sense is unknown
			if( v == "D41D8CD98F00B204E9800998ECF8427E" ) v += " (пустой пароль)";
		}
		else if( parts[0] == "_AUTOR_LOGO_INDEX_" || parts[0] == "_AUTOR_SPLASH_INDEX_" )
		{
			if( v.IsEmpty() ) v = "нет";
			else
			{
				GCString bmp = PictureToBmp(v);
				v = PictureInfo(v, bmp);
				if( parts[0] == "_AUTOR_LOGO_INDEX_" ) Logo = bmp; else Splash = bmp;
			}
		}
		Text += name + ":" + Value(v) + "\r\n";
	}
	if( i >= m || t[i] != '}' ) { Error = "список не закрыт"; return -1; }
	GCString s;
	if( i + 1 < m ) { s.Format("После списка: %d байт\r\n", m - i - 1); Text += s; }
	if( tail ) { s.Format("После строки: %ld байт (остаток прежнего содержимого)\r\n", tail); Text += s; }
	return 1;
}
