#ifndef YAML_H
#define YAML_H

#include <afxtempl.h>
#include "Strings.h"

//std::vector does not get along with the min/max macros of MFC headers
template <class T> class CYamlArray : public CArray<T, const T&>
{
public:
	size_t size() const { return (size_t)this->GetSize(); }
	bool empty() const { return this->GetSize() == 0; }
	void push_back(const T& v) { this->Add(v); }
	T& back() { return this->ElementAt(this->GetSize() - 1); }
	void pop_back() { this->RemoveAt(this->GetSize() - 1); }
	void clear() { this->RemoveAll(); }
};

/* Description files (.yaml) are YAML. GComp keeps writing and parsing its
   brace descriptions ("Name: value", blocks in { }); these functions convert
   between the two at file input and output, see README "Source tree format".

   Values: a scalar is the text of the value whatever its style; whether
   it is in quotes in the brace text (doubled quotes, \n and \\ escapes)
   is decided by the type of the property when it is read. !u "..." is a
   value of the brace text as it is. A block whose header has a
   value is  Name: {_: value, ...}  in a mapping and  - Name: value  with the
   block's lines as further keys (or under _:) in a sequence. */

//Brace text (CP1251, CRLF) -> YAML text (CP1251, LF)
#define YAML_TOP_SEQUENCE 1 //the top level is always a sequence (rights, panels)
void BraceToYaml(const GCString& Brace, GCString& Yaml, int Flags);

//YAML text (CP1251) -> brace text (CRLF); errors stop with FileName and line.
//LineMap gets the YAML line of every brace line.
void YamlToBrace(const GCString& Yaml, GCString& Brace, LPCSTR FileName,
	CYamlArray<int>* LineMap = NULL);

#endif
