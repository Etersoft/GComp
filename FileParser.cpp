
#include "Declarations.h"
#include "CommonFunctions.h"
#include "Strings.h"
#include "Lexer.h"
#include "MMS_Basic.h"
#include "DialogForm.h"

bool CMMSObject::ParseFile(const GCString& FileName)
{
	if( !FileExist(FileName) )
		return false;

	CLexer Lexer(FileName);
	ParseFile(Lexer);

	GCString Rest;
	if( Lexer.GetToken(Rest, ":\t\r\n") )
	{
		Msg(0, "ERR: %s, строка %i: лишний текст после конца описания: '%s'",
			(LPCSTR)FileName, Lexer.CurrentLine(), (LPCSTR)Rest);
		throw NULL;
	}

	return true;
}

/* Set a property read from a description file; an unknown name or an
   invalid value stops the compilation instead of being silently lost. */
static void SetFileValue(CMMSObject* Object, CLexer& Lexer, int Line,
	GCString& Name, GCString& Value)
{
	bool known;
	try
	{
		StrictPropertyValues = true;
		known = Object->SetValue(Name, Value);
		StrictPropertyValues = false;
	}
	catch(...)
	{
		StrictPropertyValues = false;
		Msg(0, "ERR: %s, строка %i: недопустимое значение свойства '%s': '%s'",
			(LPCSTR)Lexer.GetFileName(), Line, (LPCSTR)Name, (LPCSTR)Value);
		throw NULL;
	}
	if( !known )
	{
		Msg(0, "ERR: %s, строка %i: неизвестное свойство '%s' объекта '%s'",
			(LPCSTR)Lexer.GetFileName(), Line, (LPCSTR)Name, (LPCSTR)Object->sID);
		throw NULL;
	}
}

void CMMSObject::ParseFile(CLexer& Lexer)
{
	GCString Token, Value, PrevToken, PrevValue;
	int PrevLine = 0;
	bool Started = false;
	CMMSObject* Child;

	while( Lexer.GetToken(Token, ":\t\r\n") )
	{
		Started = true;
		Lexer.GetRest(Value);
		Token.TrimRight();
		Value.TrimLeft();

		if( Token == '{' )
		{
			if( Lexer.CurrentLine() <= 2 )
			{
				//просто установим ID и sID для текущего объекта
				PrevValue.TrimLeft();
				PrevValue.TrimRight();
				SetIDs(PrevToken, PrevValue);
			}
			else
			{
				Child = CreateChild(PrevToken, PrevValue);
				if( Child == NULL )
				{
					Msg(0, "ERR: Can't create child %s of %s", (LPCSTR)PrevToken, (LPCSTR)sID);
					throw NULL;
				}

				if( PrevValue.IsNumber() ) Child->ID = atoi(PrevValue);

				try {
					Child->ParseFile(Lexer);
				}
				catch(...) {
					Msg(0, "INFO: parsing %s", (LPCSTR)sID);
					throw NULL;
				}

				AddChild(Child);
			}

			Token = "";
			Value = "";
		}
		else
		{
			if( !PrevToken.IsEmpty() )
			{
				SetFileValue(this, Lexer, PrevLine, PrevToken, PrevValue);
			}
			
			if( Token == '}' )
			{
				return;
			}
		}

		PrevToken = Token;
		PrevValue = Value;
		PrevLine = Lexer.CurrentLine();
	}

	//an empty file is an empty list; otherwise every block ends with '}'
	if( !Started )
		return;
	Msg(0, "ERR: %s: конец файла внутри описания '%s', не хватает '}'",
		(LPCSTR)Lexer.GetFileName(), (LPCSTR)sID);
	throw NULL;
}


void CMMSNamedList::ParseFile(CLexer& Lexer)
{
	GCString Token, Value;

	while( Lexer.GetToken(Token, ":\t\r\n") )
	{
		if( Token == "}" )
			return;
		SetValue(-1, Token);
	}
}

void CMMSReferList::ParseFile(CLexer& Lexer)
{
	GCString Token, Value;

	while( Lexer.GetToken(Token, "\t\r\n") )
	{
		Lexer.GetRest(Value);
		if( Token == "}" )
			return;
		SetValue(-1, Token);
	}
}

void CFormLayers::ParseFile(CLexer& Lexer)
{
	GCString Token, Value;
	char* flags;

	while( Lexer.GetToken(Token, ": \t\r\n") )
	{
		Lexer.GetRest(Value);
		if( Token == "}" )
			return;

		CFormLayer* Layer = new CFormLayer;
		Layer->sID = Token;
		flags = strtok((char*)(LPCSTR)Value, ", \t");
		while( NULL != flags )
		{
			if( _stricoll(flags, "Видимый") == 0 )
				Layer->IsVisible = true;
			if( _stricoll(flags, "Активный") == 0 )
				ActiveLayer = GetNChildren();
			flags = strtok(NULL, ", \t");
		}
		AddChild(Layer);
	}
}