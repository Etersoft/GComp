#include <stdio.h>
#include <string.h>
#include "Yaml.h"
#include "CommonFunctions.h"

///////////////////////////////////////////////////////////////////////
//// Brace descriptions //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

struct CBraceNode
{
	enum {kv, bare, anon} Kind;
	GCString Key;
	GCString Value;
	bool Block;
	int Line; //of the YAML text
	CYamlArray<CBraceNode*> Children;

	CBraceNode() : Kind(kv), Block(false), Line(0) {}
	~CBraceNode()
	{
		for( size_t i = 0; i < Children.size(); i++ )
			delete Children[i];
	}
};
typedef CYamlArray<CBraceNode*> CBraceNodes;

static void FreeNodes(CBraceNodes& Nodes)
{
	for( size_t i = 0; i < Nodes.size(); i++ )
		delete Nodes[i];
	Nodes.clear();
}

static void ParseBrace(const GCString& Text, CBraceNodes& Root)
{
	CYamlArray<CBraceNodes*> Stack;
	Stack.push_back(&Root);
	CBraceNode* Last = NULL;
	const char* p = Text;
	const char* end = p + Text.GetLength();

	while( p < end )
	{
		const char* eol = strstr(p, "\r\n");
		if( eol == NULL ) eol = end;
		const char* s = p;
		while( s < eol && *s == '\t' ) s++;
		GCString Line(s, (int)(eol - s));
		p = eol < end ? eol + 2 : end;

		if( Line == "{" )
		{
			CBraceNode* n = Last;
			if( n == NULL )
			{
				n = new CBraceNode;
				n->Kind = CBraceNode::anon;
				Stack.back()->push_back(n);
			}
			n->Block = true;
			Stack.push_back(&n->Children);
			Last = NULL;
			continue;
		}
		if( Line == "}" )
		{
			if( Stack.size() > 1 ) Stack.pop_back();
			Last = NULL;
			continue;
		}
		CBraceNode* n = new CBraceNode;
		int i = Line.Find(": ");
		int len = Line.GetLength();
		if( i < 0 && len > 0 && Line[len-1] == ':' )
			i = len - 1;
		if( i < 0 )
		{
			n->Kind = CBraceNode::bare;
			n->Key = Line;
		}
		else
		{
			n->Key = Line.Left(i);
			n->Value = i + 2 <= len ? Line.Mid(i + 2) : GCString("");
		}
		Stack.back()->push_back(n);
		Last = n;
	}
}

static void BraceLines(const CBraceNodes& Nodes, int Level, GCString& Out,
	CYamlArray<int>* Map = NULL)
{
	GCString Tabs('\t', Level);
	for( size_t i = 0; i < Nodes.size(); i++ )
	{
		CBraceNode* n = Nodes[i];
		if( n->Kind == CBraceNode::bare )
		{
			Out += Tabs + n->Key + "\r\n";
			if( Map ) Map->push_back(n->Line);
			continue;
		}
		if( n->Kind == CBraceNode::kv )
		{
			Out += Tabs + n->Key + ": " + n->Value + "\r\n";
			if( Map ) Map->push_back(n->Line);
		}
		if( n->Block )
		{
			Out += Tabs + "{\r\n";
			if( Map ) Map->push_back(n->Line);
			BraceLines(n->Children, Level + 1, Out, Map);
			Out += Tabs + "}\r\n";
			if( Map ) Map->push_back(n->Line);
		}
	}
}

///////////////////////////////////////////////////////////////////////
//// Scalars /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

static const char* Reserved = "_";

//blocks written as sequences even when they could be mappings
static const char* SeqKeys[] = {
	"Ёлементы", "√лавноећеню", "—убменю", " нопки", "“екстовые нопки",
	" нопки—ѕиктограммами", "Controls", "MainMenu", "SubMenu", "Buttons",
	"TextButtons", "PictureButtons", NULL
};
//empty blocks written as [] (else {})
static const char* ListKeys[] = { "—сылки", "Refers", NULL };

static bool InList(const char** List, const GCString& Key)
{
	for( int i = 0; List[i] != NULL; i++ )
		if( Key == List[i] ) return true;
	return false;
}

static bool IsSeqKey(const GCString& Key) { return InList(SeqKeys, Key); }
static bool IsListKey(const GCString& Key) { return IsSeqKey(Key) || InList(ListKeys, Key); }

//CP1251: everything but C0, DEL and 0x98 (U+0098) is printable Unicode
static bool Printable(unsigned char c)
{
	return c == '\t' || (c >= 0x20 && c != 0x7F && c != 0x98);
}

static bool IsDigit(char c) { return c >= '0' && c <= '9'; }

//pattern helpers: advance p over characters of a class, return the count
static int Span(const char*& p, const char* Set)
{
	int n = 0;
	while( *p && strchr(Set, *p) ) { p++; n++; }
	return n;
}

static const char* Dec_ = "0123456789_";
static const char* Digits = "0123456789";

static void Sign(const char*& p) { if( *p == '+' || *p == '-' ) p++; }

//[-+]?(0|[1-9][0-9_]*|0b[01_]+|0o?[0-7_]+|0x[0-9a-fA-F_]+)
static bool MatchInt(const char* p)
{
	Sign(p);
	if( p[0] == '0' && p[1] == 'b' ) { p += 2; return Span(p, "01_") > 0 && !*p; }
	if( p[0] == '0' && p[1] == 'x' ) { p += 2; return Span(p, "0123456789abcdefABCDEF_") > 0 && !*p; }
	if( p[0] == '0' && p[1] == 'o' ) { p += 2; return Span(p, "01234567_") > 0 && !*p; }
	if( p[0] == '0' )
	{
		p++;
		if( !*p ) return true;
		return Span(p, "01234567_") > 0 && !*p;
	}
	if( *p >= '1' && *p <= '9' ) { Span(p, Dec_); return !*p; }
	return false;
}

//[-+]?[0-9][0-9_]*(:[0-5]?[0-9])+(\.[0-9_]*)?
static bool MatchSexagesimal(const char* p)
{
	Sign(p);
	if( !IsDigit(*p) ) return false;
	Span(p, Dec_);
	int groups = 0;
	while( *p == ':' )
	{
		p++;
		if( *p >= '0' && *p <= '5' && IsDigit(p[1]) ) p += 2;
		else if( IsDigit(*p) ) p++;
		else return false;
		groups++;
	}
	if( groups == 0 ) return false;
	if( *p == '.' ) { p++; Span(p, Dec_); }
	return !*p;
}

static bool Exponent(const char*& p, bool SignRequired)
{
	if( *p != 'e' && *p != 'E' ) return false;
	p++;
	if( *p == '+' || *p == '-' ) p++;
	else if( SignRequired ) return false;
	return Span(p, Digits) > 0;
}

//[-+]?([0-9][0-9_]*)?\.[0-9_]*([eE][-+]?[0-9]+)?  and
//[-+]?[0-9][0-9_]*(\.[0-9_]*)?[eE][-+]?[0-9]+
static bool MatchFloat(const char* s)
{
	const char* p = s;
	Sign(p);
	if( IsDigit(*p) ) Span(p, Dec_);
	if( *p == '.' )
	{
		p++;
		Span(p, Dec_);
		if( !*p ) return true;
		const char* q = p;
		if( Exponent(q, false) && !*q ) return true;
	}
	p = s;
	Sign(p);
	if( !IsDigit(*p) ) return false;
	Span(p, Dec_);
	if( *p == '.' ) { p++; Span(p, Dec_); }
	return Exponent(p, false) && !*p;
}

static bool MatchSpecialFloat(const char* p)
{
	const char* q = p;
	Sign(q);
	if( strcmp(q, ".inf") == 0 || strcmp(q, ".Inf") == 0 || strcmp(q, ".INF") == 0 ) return true;
	return strcmp(p, ".nan") == 0 || strcmp(p, ".NaN") == 0 || strcmp(p, ".NAN") == 0;
}

//[0-9]{4}-[0-9]{1,2}-[0-9]{1,2}.*
static bool MatchDate(const char* p)
{
	for( int i = 0; i < 4; i++ ) if( !IsDigit(*p++) ) return false;
	if( *p++ != '-' ) return false;
	if( !IsDigit(*p++) ) return false;
	if( IsDigit(*p) ) p++;
	if( *p++ != '-' ) return false;
	return IsDigit(*p) != 0;
}

//what YAML 1.1 or 1.2 resolvers may read as a number or a date
static bool NumberLike(const GCString& s)
{
	return MatchInt(s) || MatchSexagesimal(s) || MatchFloat(s) || MatchSpecialFloat(s) || MatchDate(s);
}

//0|-?[1-9][0-9]{0,14}
static bool Canonical(const GCString& s)
{
	const char* p = s;
	if( strcmp(p, "0") == 0 ) return true;
	if( *p == '-' ) p++;
	if( !(*p >= '1' && *p <= '9') ) return false;
	int n = Span(p, Digits);
	return !*p && n <= 15;
}

static bool AsciiNoCaseIn(const GCString& s, const char** Words)
{
	for( int i = 0; Words[i] != NULL; i++ )
	{
		const char* a = s;
		const char* b = Words[i];
		while( *a && *b && tolower((unsigned char)*a) == *b ) { a++; b++; }
		if( !*a && !*b ) return true;
	}
	return false;
}

static bool PlainOk(const GCString& s)
{
	static const char* Words[] = {"y", "yes", "n", "no", "true", "false", "on", "off",
		"null", "~", "=", "<<", NULL};
	int len = s.GetLength();
	if( len == 0 ) return false;
	if( s[0] == ' ' || s[0] == '\t' || s[len-1] == ' ' || s[len-1] == '\t' ) return false;
	for( int i = 0; i < len; i++ )
		if( !Printable(s[i]) || s[i] == '\t' ) return false;
	if( strchr("-?:,[]{}#&*!|>'\"%@`", s[0]) && !(s[0] == '-' && len > 1 && s[1] != ' ') )
		return false;
	if( s.Find(": ") >= 0 || s[len-1] == ':' || s.Find(" #") >= 0 ) return false;
	if( AsciiNoCaseIn(s, Words) ) return false;
	if( NumberLike(s) && !Canonical(s) ) return false;
	return true;
}

static bool SingleOk(const GCString& s)
{
	for( int i = 0; i < s.GetLength(); i++ )
		if( !Printable(s[i]) ) return false;
	return true;
}

static GCString Single(const GCString& s)
{
	GCString r = s;
	r.Replace("'", "''");
	return "'" + r + "'";
}

static GCString DoubleQuoted(const GCString& s)
{
	GCString r = "\"";
	for( int i = 0; i < s.GetLength(); i++ )
	{
		unsigned char c = s[i];
		if( c == '\\' ) r += "\\\\";
		else if( c == '"' ) r += "\\\"";
		else if( c == '\n' ) r += "\\n";
		else if( c == '\t' ) r += "\\t";
		else if( !Printable(c) )
		{
			GCString x;
			x.Format("\\x%02x", c);
			r += x;
		}
		else r += (char)c;
	}
	return r + "\"";
}

//the text of a value that GComp writes in quotes, or false
static bool GQDecode(const GCString& v, GCString& s)
{
	int len = v.GetLength();
	if( len < 2 || v[0] != '"' || v[len-1] != '"' ) return false;
	s.Empty();
	for( int i = 1; i < len - 1; )
	{
		char c = v[i];
		if( c == '"' )
		{
			if( i + 1 >= len - 1 || v[i+1] != '"' ) return false;
			s += '"';
			i += 2;
		}
		else if( c == '\\' )
		{
			char d = i + 1 < len - 1 ? v[i+1] : '\0';
			if( d == '\\' ) s += '\\';
			else if( d == 'n' ) s += '\n';
			else return false;
			i += 2;
		}
		else
		{
			s += c;
			i++;
		}
	}
	return true;
}

static GCString GQEncode(const GCString& s)
{
	GCString r = s;
	r.Replace("\\", "\\\\");
	r.Replace("\"", "\"\"");
	r.Replace("\n", "\\n");
	return "\"" + r + "\"";
}

static void SplitLines(const GCString& s, CYamlArray<GCString>& Lines)
{
	Lines.clear();
	int start = 0;
	for( int i = 0; i <= s.GetLength(); i++ )
		if( i == s.GetLength() || s[i] == '\n' )
		{
			Lines.push_back(s.Mid(start, i - start));
			start = i + 1;
		}
}

static bool LiteralOk(const GCString& s)
{
	if( s.Find('\n') < 0 ) return false;
	GCString t = s;
	t.TrimLeft("\n");
	t.TrimRight("\n");
	if( t.IsEmpty() ) return false;
	CYamlArray<GCString> Lines;
	SplitLines(s, Lines);
	for( size_t i = 0; i < Lines.size(); i++ )
	{
		const GCString& l = Lines[i];
		int len = l.GetLength();
		if( len == 0 ) continue;
		if( l[0] == ' ' || l[0] == '\t' || l[len-1] == ' ' || l[len-1] == '\t' ) return false;
		if( !SingleOk(l) ) return false;
	}
	return true;
}

struct CYamlValue
{
	bool Literal;
	GCString Text;       //inline text, or the block indicator
	CYamlArray<GCString> Lines; //of a block scalar, or of a double quoted one after Text
};

//a double quoted string over several lines: \n\ at the end of each
static void DoubleQuotedLines(const GCString& s, CYamlValue& r)
{
	CYamlArray<GCString> Parts;
	SplitLines(s, Parts);
	for( size_t i = 0; i < Parts.size(); i++ )
	{
		GCString e = DoubleQuoted(Parts[i]);
		e = e.Mid(1, e.GetLength() - 2);
		if( i > 0 && !e.IsEmpty() && e[0] == ' ' ) e = "\\" + e;
		GCString l = (i == 0 ? "\"" : "") + e + (i + 1 < Parts.size() ? "\\n\\" : "\"");
		if( i == 0 ) r.Text = l;
		else r.Lines.push_back(l);
	}
}

static void Value(const GCString& v, CYamlValue& r)
{
	r.Literal = false;
	r.Lines.clear();
	if( v.IsEmpty() ) { r.Text.Empty(); return; }
	GCString s;
	if( GQDecode(v, s) )
	{
		if( LiteralOk(s) )
		{
			int len = s.GetLength();
			r.Literal = true;
			if( len >= 2 && s[len-1] == '\n' && s[len-2] == '\n' ) r.Text = "|+";
			else if( s[len-1] == '\n' ) r.Text = "|";
			else r.Text = "|-";
			SplitLines(s, r.Lines);
			if( s[len-1] == '\n' ) r.Lines.pop_back();
			return;
		}
		if( s.Find('\n') >= 0 )
			DoubleQuotedLines(s, r);
		else
			r.Text = DoubleQuoted(s);
		return;
	}
	int len = v.GetLength();
	bool quotes = len >= 2 && v[0] == '"' && v[len-1] == '"'; //but not GComp's quoting
	if( PlainOk(v) ) r.Text = v;
	else if( SingleOk(v) && !quotes ) r.Text = Single(v);
	else r.Text = "!u " + DoubleQuoted(v); //the value as is, with characters YAML must escape
}

static GCString Key(const GCString& k)
{
	if( k == Reserved ) return Single(k);
	if( PlainOk(k) ) return k;
	if( SingleOk(k) ) return Single(k);
	return "!u " + DoubleQuoted(k);
}

///////////////////////////////////////////////////////////////////////
//// YAML writer //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

static bool HasKey(const CBraceNodes& Ch, const GCString& k)
{
	for( size_t i = 0; i < Ch.size(); i++ )
		if( Ch[i]->Kind == CBraceNode::kv && Ch[i]->Key == k ) return true;
	return false;
}

static bool Mapable(const CBraceNodes& Ch)
{
	for( size_t i = 0; i < Ch.size(); i++ )
	{
		CBraceNode* n = Ch[i];
		if( n->Kind != CBraceNode::kv ) return false;
		for( size_t j = 0; j < i; j++ )
			if( Ch[j]->Key == n->Key ) return false;
		if( n->Block && !n->Value.IsEmpty() && !Mapable(n->Children) ) return false;
	}
	return true;
}

static GCString Spaces(int n) { return GCString(' ', n); }

static GCString EmptyBlock(const GCString& k) { return IsListKey(k) ? "[]" : "{}"; }

static void Line(GCString& Out, const GCString& l)
{
	Out += l;
	Out += "\n";
}

static void EmitValue(GCString& Out, const GCString& Prefix, int Col, const GCString& v)
{
	CYamlValue t;
	Value(v, t);
	if( !t.Literal )
	{
		Line(Out, t.Text.IsEmpty() ? Prefix : Prefix + " " + t.Text);
		for( size_t i = 0; i < t.Lines.size(); i++ )
			Line(Out, Spaces(Col + 2) + t.Lines[i]);
		return;
	}
	Line(Out, Prefix + " " + t.Text);
	for( size_t i = 0; i < t.Lines.size(); i++ )
		Line(Out, t.Lines[i].IsEmpty() ? GCString("") : Spaces(Col + 2) + t.Lines[i]);
}

static void EmitContent(GCString& Out, const CBraceNodes& Ch, int Col, const GCString* k);

static void EmitMap(GCString& Out, const CBraceNodes& Ch, int Col)
{
	for( size_t i = 0; i < Ch.size(); i++ )
	{
		CBraceNode* n = Ch[i];
		GCString p = Spaces(Col) + Key(n->Key) + ":";
		if( !n->Block )
			EmitValue(Out, p, Col, n->Value);
		else if( n->Value.IsEmpty() )
		{
			if( n->Children.empty() )
				Line(Out, p + " " + EmptyBlock(n->Key));
			else
			{
				Line(Out, p);
				EmitContent(Out, n->Children, Col + 2, &n->Key);
			}
		}
		else
		{
			Line(Out, p);
			EmitValue(Out, Spaces(Col + 2) + Reserved + ":", Col + 2, n->Value);
			EmitMap(Out, n->Children, Col + 2);
		}
	}
}

static void EmitSeq(GCString& Out, const CBraceNodes& Ch, int Col)
{
	for( size_t i = 0; i < Ch.size(); i++ )
	{
		CBraceNode* n = Ch[i];
		GCString d = Spaces(Col) + "- ";
		if( n->Kind == CBraceNode::bare )
		{
			CYamlValue t;
			Value(n->Key, t);
			if( !t.Literal )
			{
				Line(Out, t.Text.IsEmpty() ? Spaces(Col) + "-" : d + t.Text);
				for( size_t j = 0; j < t.Lines.size(); j++ )
					Line(Out, Spaces(Col + 2) + t.Lines[j]);
			}
			else
			{
				Line(Out, d + t.Text);
				for( size_t j = 0; j < t.Lines.size(); j++ )
					Line(Out, t.Lines[j].IsEmpty() ? GCString("") : Spaces(Col + 2) + t.Lines[j]);
			}
		}
		else if( n->Kind == CBraceNode::anon )
		{
			GCString p = d + Reserved + ":";
			if( n->Children.empty() )
				Line(Out, p + " {}");
			else
			{
				Line(Out, p);
				EmitContent(Out, n->Children, Col + 4, NULL);
			}
		}
		else
		{
			EmitValue(Out, d + Key(n->Key) + ":", Col + 2, n->Value);
			if( !n->Block ) continue;
			if( !n->Children.empty() && Mapable(n->Children) &&
				!HasKey(n->Children, n->Key) && !HasKey(n->Children, Reserved) )
				EmitMap(Out, n->Children, Col + 2);
			else
			{
				GCString p2 = Spaces(Col + 2) + Reserved + ":";
				if( n->Children.empty() )
					Line(Out, p2 + " " + EmptyBlock(n->Key));
				else
				{
					Line(Out, p2);
					EmitContent(Out, n->Children, Col + 4, &n->Key);
				}
			}
		}
	}
}

static void EmitContent(GCString& Out, const CBraceNodes& Ch, int Col, const GCString* k)
{
	if( (k == NULL || !IsSeqKey(*k)) && Mapable(Ch) )
		EmitMap(Out, Ch, Col);
	else
		EmitSeq(Out, Ch, Col);
}

void BraceToYaml(const GCString& Brace, GCString& Yaml, int Flags)
{
	CBraceNodes Nodes;
	ParseBrace(Brace, Nodes);
	Yaml.Empty();
	if( !Nodes.empty() )
	{
		if( !(Flags & YAML_TOP_SEQUENCE) && Mapable(Nodes) )
			EmitMap(Yaml, Nodes, 0);
		else
			EmitSeq(Yaml, Nodes, 0);
	}
	FreeNodes(Nodes);
}

///////////////////////////////////////////////////////////////////////
//// YAML reader //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

struct CYamlNode
{
	enum TType {scalar, mapping, sequence} Type;
	GCString Value;
	char Style;  //0 plain, '\'', '"', '|', '>'
	bool Raw;    //!u
	int Line;
	CYamlArray<CYamlNode*> Keys;  //mapping
	CYamlArray<CYamlNode*> Items; //mapping values, sequence items

	CYamlNode(int t, int line) : Type((TType)t), Style(0), Raw(false), Line(line) {}
	~CYamlNode()
	{
		size_t i;
		for( i = 0; i < Keys.size(); i++ ) delete Keys[i];
		for( i = 0; i < Items.size(); i++ ) delete Items[i];
	}
	bool IsNull() const { return Type == scalar && Style == 0 && !Raw &&
		(Value.IsEmpty() || Value == "~" || Value == "null" || Value == "Null" || Value == "NULL"); }
};

class CYamlParser
{
public:
	CYamlParser(const GCString& Text, LPCSTR FileName);
	CYamlNode* Document();

private:
	GCString Src;  //LF only, ends with '\n'
	const char* s;
	int pos;
	int len;
	LPCSTR FileName;

	void Error(const char* What, int at = -1);
	int LineOf(int at);
	int LineStart(int at) { while( at > 0 && s[at-1] != '\n' ) at--; return at; }
	int Column(int at) { return at - LineStart(at); }
	bool AtLineEnd(int at) { return s[at] == '\n'; }
	bool BlankOrComment(int at);
	int NextContent(int at);
	void SkipSpaces() { while( s[pos] == ' ' || s[pos] == '\t' ) pos++; }
	void SkipToLineEnd();
	bool IsSeqEntry(int at) { return s[at] == '-' && (s[at+1] == ' ' || s[at+1] == '\n'); }
	bool IsKeyLine(int at);

	CYamlNode* BlockNode(int Indent);
	CYamlNode* InlineNode(int Indent, int Col);
	CYamlNode* BlockSequence(int Col);
	CYamlNode* BlockMapping(int Col);
	CYamlNode* ValueAfterColon(int Indent);
	CYamlNode* KeyScalar();
	CYamlNode* Scalar(int Indent, bool Flow);
	void Plain(int Indent, bool Flow, GCString& Out);
	void Quoted(char q, GCString& Out);
	void BlockScalar(int Indent, CYamlNode* n);
	CYamlNode* FlowNode();
	void FlowSkip();
	void Properties(bool& Raw);
	void AppendCodePoint(unsigned int c, GCString& Out);
};

CYamlParser::CYamlParser(const GCString& Text, LPCSTR fname)
{
	FileName = fname;
	Src = Text;
	Src.Replace("\r\n", "\n");
	Src.Replace("\r", "\n");
	if( Src.IsEmpty() || Src[Src.GetLength()-1] != '\n' ) Src += "\n";
	s = Src;
	len = Src.GetLength();
	pos = 0;
	for( int i = 0; i < len; i++ )
		if( s[i] == '\0' ) Error("символ с кодом 0", i);
}

int CYamlParser::LineOf(int at)
{
	int line = 1;
	for( int i = 0; i < at && i < len; i++ )
		if( s[i] == '\n' ) line++;
	return line;
}

void CYamlParser::Error(const char* What, int at)
{
	if( at < 0 ) at = pos;
	Msg(0, "ERR: %s, строка %i: %s", FileName, LineOf(at), What);
	throw NULL;
}

//a line of spaces and maybe a comment
bool CYamlParser::BlankOrComment(int at)
{
	while( s[at] == ' ' || s[at] == '\t' ) at++;
	return s[at] == '\n' || s[at] == '#';
}

//the first character of the next line with content, from the line of at; len at the end
int CYamlParser::NextContent(int at)
{
	at = LineStart(at);
	while( at < len )
	{
		if( !BlankOrComment(at) )
		{
			while( s[at] == ' ' ) at++;
			if( s[at] == '\t' ) Error("табул€ци€ в отступе", at);
			int ls = LineStart(at);
			if( at - ls == 0 && (strncmp(s + at, "---", 3) == 0 || strncmp(s + at, "...", 3) == 0)
				&& (s[at+3] == '\n' || s[at+3] == ' ') )
				return len; //the end of the document
			return at;
		}
		while( s[at] != '\n' ) at++;
		at++;
	}
	return len;
}

//the rest of the line may only be a comment; pos goes to the next line
void CYamlParser::SkipToLineEnd()
{
	SkipSpaces();
	if( s[pos] == '#' ) while( s[pos] != '\n' ) pos++;
	if( s[pos] != '\n' ) Error("лишний текст в конце строки");
	pos++;
}

CYamlNode* CYamlParser::Document()
{
	//directives and the document start
	int at = 0;
	while( at < len )
	{
		if( BlankOrComment(at) || s[at] == '%' )
		{
			while( s[at] != '\n' ) at++;
			at++;
			continue;
		}
		if( strncmp(s + at, "---", 3) == 0 && (s[at+3] == '\n' || s[at+3] == ' ') )
		{
			at += 3;
			while( s[at] != '\n' ) at++;
			at++;
		}
		break;
	}
	pos = NextContent(at);
	if( pos >= len ) return NULL;
	CYamlNode* n = BlockNode(-1);
	pos = pos < len ? NextContent(pos) : len;
	if( pos < len ) { delete n; Error("лишний текст после конца документа"); }
	return n;
}

//a key: a plain or quoted scalar on this line followed by ':' and a space or the line end
bool CYamlParser::IsKeyLine(int at)
{
	if( s[at] == '?' && (s[at+1] == ' ' || s[at+1] == '\n') ) return true; //explicit key
	if( s[at] == '!' && s[at+1] == 'u' && s[at+2] == ' ' ) at += 3;
	if( s[at] == '"' || s[at] == '\'' )
	{
		char q = s[at++];
		while( s[at] != '\n' )
		{
			if( q == '\'' && s[at] == '\'' && s[at+1] == '\'' ) { at += 2; continue; }
			if( q == '"' && s[at] == '\\' && s[at+1] != '\n' ) { at += 2; continue; }
			if( s[at] == q ) break;
			at++;
		}
		if( s[at] != q ) return false;
		at++;
		while( s[at] == ' ' ) at++;
		return s[at] == ':' && (s[at+1] == ' ' || s[at+1] == '\n');
	}
	if( strchr("[]{},#&*!|>%@`", s[at]) ) return false;
	if( IsSeqEntry(at) ) return false;
	for( ; s[at] != '\n'; at++ )
	{
		if( s[at] == ':' && (s[at+1] == ' ' || s[at+1] == '\n') ) return true;
		if( s[at] == ' ' && s[at+1] == '#' ) return false;
	}
	return false;
}

//pos is at the first character of a node on its own line (column > Indent)
CYamlNode* CYamlParser::BlockNode(int Indent)
{
	return InlineNode(Indent, Column(pos));
}

//pos is at a node that starts at column Col (maybe after "- ")
CYamlNode* CYamlParser::InlineNode(int Indent, int Col)
{
	if( IsSeqEntry(pos) ) return BlockSequence(Col);
	if( IsKeyLine(pos) ) return BlockMapping(Col);
	return Scalar(Indent, false);
}

CYamlNode* CYamlParser::BlockSequence(int Col)
{
	CYamlNode* n = new CYamlNode(CYamlNode::sequence, LineOf(pos));
	try
	{
		while( true )
		{
			pos++; //'-'
			SkipSpaces();
			CYamlNode* item;
			if( s[pos] == '\n' || s[pos] == '#' )
			{
				int line = LineOf(pos);
				SkipToLineEnd();
				int next = NextContent(pos);
				if( next < len && Column(next) > Col )
				{
					pos = next;
					item = BlockNode(Col);
				}
				else
					item = new CYamlNode(CYamlNode::scalar, line);
			}
			else
				item = InlineNode(Col, Column(pos));
			n->Items.push_back(item);

			int next = pos < len ? NextContent(pos) : len;
			if( next >= len || Column(next) < Col ) { pos = next >= len ? len : LineStart(next); break; }
			if( Column(next) > Col ) Error("неверный отступ", next);
			if( !IsSeqEntry(next) ) { pos = LineStart(next); break; } //a key of the parent mapping
			pos = next;
		}
	}
	catch(...) { delete n; throw; }
	return n;
}

CYamlNode* CYamlParser::BlockMapping(int Col)
{
	CYamlNode* n = new CYamlNode(CYamlNode::mapping, LineOf(pos));
	try
	{
		while( true )
		{
			if( !IsKeyLine(pos) ) Error("ожидалс€ ключ");
			if( s[pos] == '?' )
			{
				//? key
				//: value
				pos++;
				SkipSpaces();
				if( s[pos] == '\n' || s[pos] == '#' ) Error("сложные ключи не поддерживаютс€");
				n->Keys.push_back(Scalar(Col, false));
				n->Items.push_back(NULL);
				int next = pos < len ? NextContent(pos) : len;
				if( next < len && Column(next) == Col && s[next] == ':' &&
					(s[next+1] == ' ' || s[next+1] == '\n') )
				{
					pos = next + 1;
					SkipSpaces();
					if( s[pos] == '\n' || s[pos] == '#' )
						n->Items.back() = ValueAfterColon(Col);
					else
						n->Items.back() = InlineNode(Col, Column(pos));
				}
				else
				{
					n->Items.back() = new CYamlNode(CYamlNode::scalar, LineOf(pos));
					if( next < len ) pos = LineStart(next);
				}
			}
			else
			{
				CYamlNode* k = KeyScalar();
				n->Keys.push_back(k);
				n->Items.push_back(NULL);
				SkipSpaces();
				if( s[pos] != ':' ) Error("ожидалось ':'");
				pos++;
				n->Items.back() = ValueAfterColon(Col);
			}

			int next = pos < len ? NextContent(pos) : len;
			if( next >= len || Column(next) < Col ) { pos = next >= len ? len : LineStart(next); break; }
			if( Column(next) > Col ) Error("неверный отступ", next);
			pos = next;
			if( IsSeqEntry(pos) ) Error("элемент списка на уровне ключей");
		}
	}
	catch(...) { delete n; throw; }
	return n;
}

CYamlNode* CYamlParser::ValueAfterColon(int Indent)
{
	SkipSpaces();
	if( s[pos] == '\n' || s[pos] == '#' )
	{
		int line = LineOf(pos);
		SkipToLineEnd();
		int next = NextContent(pos);
		if( next < len && Column(next) > Indent )
		{
			pos = next;
			return BlockNode(Indent);
		}
		if( next < len && Column(next) == Indent && IsSeqEntry(next) )
		{
			pos = next;
			return BlockSequence(Indent);
		}
		return new CYamlNode(CYamlNode::scalar, line);
	}
	return Scalar(Indent, false);
}

void CYamlParser::Properties(bool& Raw)
{
	Raw = false;
	while( true )
	{
		if( s[pos] == '!' )
		{
			int start = pos;
			while( s[pos] != ' ' && s[pos] != '\n' ) pos++;
			GCString Tag(s + start, pos - start);
			if( Tag == "!u" ) Raw = true;
			else if( Tag.Left(2) != "!!" ) Error("неизвестный тег", start);
			SkipSpaces();
		}
		else if( s[pos] == '&' || s[pos] == '*' )
			Error("€кор€ и ссылки не поддерживаютс€");
		else
			break;
	}
}

CYamlNode* CYamlParser::KeyScalar()
{
	bool Raw;
	Properties(Raw);
	CYamlNode* k = new CYamlNode(CYamlNode::scalar, LineOf(pos));
	k->Raw = Raw;
	if( s[pos] == '"' || s[pos] == '\'' )
	{
		k->Style = s[pos];
		try { Quoted(s[pos], k->Value); } catch(...) { delete k; throw; }
		return k;
	}
	int start = pos;
	while( !(s[pos] == ':' && (s[pos+1] == ' ' || s[pos+1] == '\n')) ) pos++;
	k->Value = GCString(s + start, pos - start);
	k->Value.TrimRight(" \t");
	return k;
}

CYamlNode* CYamlParser::Scalar(int Indent, bool Flow)
{
	bool Raw;
	Properties(Raw);
	if( !Flow && (s[pos] == '[' || s[pos] == '{') )
	{
		if( Raw ) Error("тег !u у коллекции");
		CYamlNode* n = FlowNode();
		SkipToLineEnd();
		return n;
	}
	CYamlNode* n = new CYamlNode(CYamlNode::scalar, LineOf(pos));
	n->Raw = Raw;
	try
	{
		if( s[pos] == '"' || s[pos] == '\'' )
		{
			n->Style = s[pos];
			Quoted(s[pos], n->Value);
			if( !Flow ) SkipToLineEnd();
		}
		else if( !Flow && (s[pos] == '|' || s[pos] == '>') )
			BlockScalar(Indent, n);
		else
		{
			Plain(Indent, Flow, n->Value);
			if( !Flow ) SkipToLineEnd();
		}
	}
	catch(...) { delete n; throw; }
	return n;
}

//a plain scalar, maybe over several lines more indented than Indent
void CYamlParser::Plain(int Indent, bool Flow, GCString& Out)
{
	Out.Empty();
	int breaks = 0;
	bool first = true;
	while( true )
	{
		int start = pos;
		while( s[pos] != '\n' )
		{
			if( s[pos] == ':' && (s[pos+1] == ' ' || s[pos+1] == '\n' ||
				(Flow && strchr(",[]{}", s[pos+1]))) ) break;
			if( (s[pos] == ' ' || s[pos] == '\t') && s[pos+1] == '#' ) break;
			if( Flow && strchr(",[]{}", s[pos]) ) break;
			pos++;
		}
		GCString Part(s + start, pos - start);
		Part.TrimRight(" \t");
		if( !Part.IsEmpty() )
		{
			if( !first )
			{
				if( breaks == 0 ) Out += ' ';
				else for( int i = 0; i < breaks; i++ ) Out += '\n';
			}
			Out += Part;
			first = false;
			breaks = 0;
		}
		if( s[pos] != '\n' ) return; //': ', ' #' or a flow indicator
		//continuation lines
		int at = pos + 1;
		int empty = 0;
		while( at < len )
		{
			int b = at;
			while( s[b] == ' ' || s[b] == '\t' ) b++;
			if( s[b] != '\n' ) break;
			empty++;
			at = b + 1;
		}
		if( at >= len ) return;
		int b = at;
		while( s[b] == ' ' ) b++;
		if( (!Flow && b - at <= Indent) || s[b] == '#' ) return;
		if( !Flow && strncmp(s + at, "---", 3) == 0 ) return;
		while( s[b] == '\t' ) b++;
		if( Flow && strchr(",[]{}", s[b]) ) { pos = b; return; }
		pos = b;
		breaks = empty;
		if( !Flow && IsKeyLine(b) ) Error("ожидалось продолжение значени€, а не ключ");
	}
}

void CYamlParser::AppendCodePoint(unsigned int c, GCString& Out)
{
	int b = UnicodeToCp1251(c);
	if( b < 0 ) Error("символ, которого нет в CP1251");
	Out += (char)b;
}

//a quoted scalar, maybe over several lines
void CYamlParser::Quoted(char q, GCString& Out)
{
	Out.Empty();
	int start = pos++;
	GCString Ws; //whitespace that ends a line is dropped on folding
	while( true )
	{
		char c = s[pos];
		if( pos >= len ) Error("нет закрывающей кавычки", start);
		if( c == q )
		{
			if( q == '\'' && s[pos+1] == '\'' ) { Out += Ws; Ws.Empty(); Out += '\''; pos += 2; continue; }
			Out += Ws;
			pos++;
			return;
		}
		if( c == '\n' )
		{
			//folding: a line break is a space, n empty lines are n line breaks
			int breaks = 0;
			pos++;
			while( true )
			{
				while( s[pos] == ' ' || s[pos] == '\t' ) pos++;
				if( s[pos] != '\n' || pos >= len ) break;
				breaks++;
				pos++;
			}
			Ws.Empty();
			if( breaks == 0 ) Out += ' ';
			else for( int i = 0; i < breaks; i++ ) Out += '\n';
			continue;
		}
		if( c == ' ' || c == '\t' ) { Ws += c; pos++; continue; }
		Out += Ws;
		Ws.Empty();
		if( q == '"' && c == '\\' )
		{
			char e = s[pos+1];
			pos += 2;
			switch( e )
			{
			case '0': Out += '\0'; break;
			case 'a': Out += '\a'; break;
			case 'b': Out += '\b'; break;
			case 't': case '\t': Out += '\t'; break;
			case 'n': Out += '\n'; break;
			case 'v': Out += '\v'; break;
			case 'f': Out += '\f'; break;
			case 'r': Out += '\r'; break;
			case 'e': Out += '\x1B'; break;
			case ' ': Out += ' '; break;
			case '"': Out += '"'; break;
			case '/': Out += '/'; break;
			case '\\': Out += '\\'; break;
			case 'N': AppendCodePoint(0x85, Out); break;
			case '_': AppendCodePoint(0xA0, Out); break;
			case 'L': AppendCodePoint(0x2028, Out); break;
			case 'P': AppendCodePoint(0x2029, Out); break;
			case 'x': case 'u': case 'U':
				{
					int n = e == 'x' ? 2 : e == 'u' ? 4 : 8;
					unsigned int v = 0;
					for( int i = 0; i < n; i++, pos++ )
					{
						char h = s[pos];
						int d = IsDigit(h) ? h - '0' : (h >= 'a' && h <= 'f') ? h - 'a' + 10 :
							(h >= 'A' && h <= 'F') ? h - 'A' + 10 : -1;
						if( d < 0 ) Error("неверна€ escape-последовательность", pos);
						v = v * 16 + d;
					}
					AppendCodePoint(v, Out);
				}
				break;
			case '\n': //escaped line break: no space
				while( s[pos] == ' ' || s[pos] == '\t' ) pos++;
				break;
			default:
				Error("неверна€ escape-последовательность", pos - 2);
			}
			continue;
		}
		Out += c;
		pos++;
	}
}

//| and > scalars; Indent is the indentation of the parent node
void CYamlParser::BlockScalar(int Indent, CYamlNode* n)
{
	n->Style = s[pos++];
	char chomp = 0;
	int ind = 0;
	for( int i = 0; i < 2; i++ )
	{
		if( s[pos] == '+' || s[pos] == '-' ) chomp = s[pos++];
		else if( s[pos] >= '1' && s[pos] <= '9' ) ind = s[pos++] - '0';
	}
	SkipToLineEnd();

	int base = Indent < 0 ? 0 : Indent;
	int Ind = ind > 0 ? base + ind : -1;
	CYamlArray<GCString> Lines;
	while( pos < len )
	{
		int b = pos;
		while( s[b] == ' ' ) b++;
		int col = b - pos;
		if( s[b] == '\n' && (Ind < 0 || col <= Ind) ) //empty line
		{
			Lines.push_back(Ind >= 0 && col > Ind ? GCString(s + pos + Ind, col - Ind) : GCString(""));
			pos = b + 1;
			continue;
		}
		if( Ind < 0 )
		{
			if( col <= Indent ) break;
			Ind = col;
		}
		if( col < Ind ) break;
		int e = b;
		while( s[e] != '\n' ) e++;
		Lines.push_back(GCString(s + pos + Ind, e - pos - Ind));
		pos = e + 1;
	}
	//the lines that ended the scalar belong to what follows
	int trailing = 0;
	while( trailing < (int)Lines.size() && Lines[Lines.size() - 1 - trailing].IsEmpty() ) trailing++;
	int content = (int)Lines.size() - trailing;

	GCString& Out = n->Value;
	Out.Empty();
	if( content > 0 ) Out = Lines[0];
	for( int i = 1; i < content; )
	{
		//empty lines between Lines[i-1] and the next text line
		int j = i;
		while( j < content && Lines[j].IsEmpty() ) j++;
		int k = j - i;
		const GCString& prev = Lines[i-1];
		const GCString& next = Lines[j];
		bool normal = !prev.IsEmpty() && prev[0] != ' ' && prev[0] != '\t' &&
			next[0] != ' ' && next[0] != '\t';
		if( n->Style == '>' && normal )
		{
			if( k == 0 ) Out += ' ';
			else for( int m = 0; m < k; m++ ) Out += '\n';
		}
		else
			for( int m = 0; m <= k; m++ ) Out += '\n';
		Out += next;
		i = j + 1;
	}
	if( content > 0 && chomp != '-' ) Out += '\n';
	if( chomp == '+' ) for( int i = 0; i < trailing; i++ ) Out += '\n';
}

void CYamlParser::FlowSkip()
{
	while( true )
	{
		while( s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' ) pos++;
		if( s[pos] == '#' ) { while( s[pos] != '\n' ) pos++; continue; }
		break;
	}
	if( pos >= len ) Error("незакрыта€ скобка");
}

CYamlNode* CYamlParser::FlowNode()
{
	char open = s[pos];
	char close = open == '[' ? ']' : '}';
	CYamlNode* n = new CYamlNode(open == '[' ? CYamlNode::sequence : CYamlNode::mapping, LineOf(pos));
	pos++;
	try
	{
		while( true )
		{
			FlowSkip();
			if( s[pos] == close ) { pos++; return n; }
			CYamlNode* a = (s[pos] == '[' || s[pos] == '{') ? FlowNode() : Scalar(-1, true);
			FlowSkip();
			CYamlNode* b = NULL;
			if( s[pos] == ':' )
			{
				pos++;
				FlowSkip();
				if( s[pos] == ',' || s[pos] == close )
					b = new CYamlNode(CYamlNode::scalar, LineOf(pos));
				else
					b = (s[pos] == '[' || s[pos] == '{') ? FlowNode() : Scalar(-1, true);
				FlowSkip();
			}
			if( open == '{' )
			{
				if( b == NULL ) b = new CYamlNode(CYamlNode::scalar, a->Line);
				n->Keys.push_back(a);
				n->Items.push_back(b);
			}
			else if( b != NULL ) //[k: v] is a mapping with one pair
			{
				CYamlNode* m = new CYamlNode(CYamlNode::mapping, a->Line);
				m->Keys.push_back(a);
				m->Items.push_back(b);
				n->Items.push_back(m);
			}
			else
				n->Items.push_back(a);
			if( s[pos] == ',' ) { pos++; continue; }
			if( s[pos] != close ) Error("ожидалась ',' или закрывающа€ скобка");
		}
	}
	catch(...) { delete n; throw; }
}

///////////////////////////////////////////////////////////////////////
//// YAML -> brace ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

class CYamlToBrace
{
public:
	LPCSTR FileName;
	void Error(const CYamlNode* n, const char* What)
	{
		Msg(0, "ERR: %s, строка %i: %s", FileName, n->Line, What);
		throw NULL;
	}
	GCString RawValue(const CYamlNode* n);
	bool IsReserved(const CYamlNode* k) { return k->Type == CYamlNode::scalar && k->Style == 0 && !k->Raw && k->Value == Reserved; }
	void NodesOf(const CYamlNode* n, CBraceNodes& Out);
	CBraceNode* Entry(const CYamlNode* k, const CYamlNode* v);
	CBraceNode* Item(const CYamlNode* n);
	GCString KeyText(const CYamlNode* k)
	{
		if( k->Type != CYamlNode::scalar ) Error(k, "ключ должен быть строкой");
		return k->Value;
	}
};

GCString CYamlToBrace::RawValue(const CYamlNode* n)
{
	if( n->Type != CYamlNode::scalar ) Error(n, "ожидалось значение, а не список или блок");
	if( n->Raw ) return n->Value;
	//a line break fits only into a quoted value, whatever the style
	if( n->Style == '"' || n->Style == '|' || n->Style == '>' || n->Value.Find('\n') >= 0 )
		return GQEncode(n->Value);
	//GComp trims an unquoted value and takes "..." for its quoting: such
	//text of another program is quoted (GComp itself writes it with !u)
	int len = n->Value.GetLength();
	if( len > 0 && (strchr(" \t", n->Value[0]) || strchr(" \t", n->Value[len-1])) )
		return GQEncode(n->Value);
	if( len >= 2 && n->Value[0] == '"' && n->Value[len-1] == '"' )
		return GQEncode(n->Value);
	if( n->IsNull() ) return "";
	return n->Value;
}

void CYamlToBrace::NodesOf(const CYamlNode* n, CBraceNodes& Out)
{
	if( n->Type == CYamlNode::mapping )
	{
		for( size_t i = 0; i < n->Keys.size(); i++ )
			Out.push_back(Entry(n->Keys[i], n->Items[i]));
	}
	else if( n->Type == CYamlNode::sequence )
	{
		for( size_t i = 0; i < n->Items.size(); i++ )
			Out.push_back(Item(n->Items[i]));
	}
	else if( !n->IsNull() )
		Error(n, "ожидалс€ список или блок");
}

CBraceNode* CYamlToBrace::Entry(const CYamlNode* k, const CYamlNode* v)
{
	CBraceNode* e = new CBraceNode;
	e->Key = KeyText(k);
	e->Line = k->Line;
	try
	{
		if( v->Type == CYamlNode::scalar )
			e->Value = RawValue(v);
		else if( v->Type == CYamlNode::mapping && !v->Keys.empty() && IsReserved(v->Keys[0]) )
		{
			e->Value = RawValue(v->Items[0]);
			e->Block = true;
			for( size_t i = 1; i < v->Keys.size(); i++ )
				e->Children.push_back(Entry(v->Keys[i], v->Items[i]));
		}
		else
		{
			e->Block = true;
			NodesOf(v, e->Children);
		}
	}
	catch(...) { delete e; throw; }
	return e;
}

CBraceNode* CYamlToBrace::Item(const CYamlNode* n)
{
	if( n->Type == CYamlNode::scalar )
	{
		CBraceNode* e = new CBraceNode;
		e->Kind = CBraceNode::bare;
		e->Key = RawValue(n);
		e->Line = n->Line;
		return e;
	}
	if( n->Type == CYamlNode::sequence ) Error(n, "список внутри списка");
	if( n->Keys.empty() ) Error(n, "пустой элемент списка");

	if( n->Keys.size() == 1 && IsReserved(n->Keys[0]) )
	{
		CBraceNode* e = new CBraceNode;
		e->Kind = CBraceNode::anon;
		e->Block = true;
		e->Line = n->Line;
		try { NodesOf(n->Items[0], e->Children); } catch(...) { delete e; throw; }
		return e;
	}
	if( n->Keys.size() == 1 )
		return Entry(n->Keys[0], n->Items[0]);

	CBraceNode* e = new CBraceNode;
	e->Key = KeyText(n->Keys[0]);
	e->Line = n->Keys[0]->Line;
	try
	{
		e->Value = RawValue(n->Items[0]);
		e->Block = true;
		if( n->Keys.size() == 2 && IsReserved(n->Keys[1]) )
			NodesOf(n->Items[1], e->Children);
		else
			for( size_t i = 1; i < n->Keys.size(); i++ )
				e->Children.push_back(Entry(n->Keys[i], n->Items[i]));
	}
	catch(...) { delete e; throw; }
	return e;
}

void YamlToBrace(const GCString& Yaml, GCString& Brace, LPCSTR FileName, CYamlArray<int>* LineMap)
{
	CYamlParser Parser(Yaml, FileName);
	CYamlNode* Root = Parser.Document();
	Brace.Empty();
	if( Root == NULL ) return;

	CYamlToBrace Conv;
	Conv.FileName = FileName;
	CBraceNodes Nodes;
	try
	{
		if( Root->Type == CYamlNode::scalar && !Root->IsNull() )
			Conv.Error(Root, "ожидалс€ список или блок");
		Conv.NodesOf(Root, Nodes);
	}
	catch(...)
	{
		FreeNodes(Nodes);
		delete Root;
		throw;
	}
	delete Root;
	if( LineMap ) LineMap->clear();
	BraceLines(Nodes, 0, Brace, LineMap);
	FreeNodes(Nodes);

	//CLexer reads lines into a 4096 byte buffer
	int start = 0;
	for( int i = 0; i < Brace.GetLength(); i++ )
		if( Brace[i] == '\n' )
		{
			if( i - start > 4000 )
			{
				Msg(0, "ERR: %s: слишком длинное значение (больше 4000 байт в одной строке описани€)", FileName);
				throw NULL;
			}
			start = i + 1;
		}
}
