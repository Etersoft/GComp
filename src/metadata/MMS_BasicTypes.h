
#ifndef MMS_BasicTypes
#define MMS_BasicTypes

#include "Declarations.h"
#include "Strings.h"

#define VAR_OFFSET(OBJ, VAR) ((int)(&(OBJ->VAR)) - (int)OBJ)

class CObjectProperty;
class CPropertySet;


//Check values set from description files (see SetFileValue)
extern bool StrictPropertyValues;

class CStoredProperty
{
public:
	virtual ~CStoredProperty() {}
	virtual void SetValue(GCString& Value) = 0;
	virtual GCString BeautyString() = 0;
	virtual GCString MMSString() = 0;
};

//При множественном наследовании этот класс должен быть первым!
class CStoredObject
{
public:
	/* Number of fields in the record read from the Main Metadata Stream, or
	   -1. Older records have fewer fields than the property tables; ToString
	   writes no more than that so that compiling reproduces the record. */
	int FieldCount;

	CStoredObject() : FieldCount(-1) {}
	virtual ~CStoredObject() {}
	//"ЧислоПолей: N" in a description when the record is shorter than the table
	void BeautyFieldCount(CBeautyString& String, int level);

	virtual CPropertySet* GetPropSet() {return NULL;};
	virtual bool SetValue(GCString& Name, GCString& Value);
	virtual bool SetValue(int MMS_Index, GCString& Value);
	virtual void BeautyString(CBeautyString& String, int level);
	virtual void ToString(CMMSString& String);
};

class CObjectProperty
{
public:
	const char* Name;
	int MMS_Index;
	enum {
		t_object,    //объект, который в ММС начинается со '{'
		t_subobject, //объект внутри gcomp (например тип реквизита)
		t_prop,      //свойство типа CStoredProperty, или производный класс
		t_string, t_quotstring, 
		t_bool, t_int, t_uint
	} Type;
	int Offset; //offset from beginning of object, in bytes
};


class CPropertySet
{
private:
	CObjectProperty* Propertys;
	CStoredObject* DefaultValues;
public:
	int nProps;
	int MaxMMS_Index;

private:
	void add_flags(CBeautyString& String, int level, const char* id, unsigned int Flags, char* FlagNames[]);
public:
	CPropertySet(CObjectProperty* Props, CStoredObject* Defaults);
	~CPropertySet();

	CObjectProperty& operator[] (int i) { return Propertys[i]; };
	void* GetObjProp(void* Object, int i);
	
	int FindByName(const char* Name);
	int FindByMMSIndex(int Index);
	void SetValue(void* Object, int i, GCString& Value, bool Unqoute = true);
	bool SetByMMSIndex(void* Object, int MMS_Index, GCString& Value, bool Unqoute = true);
	
	//Defaults - an object of the same class to compare with instead of the
	//table's defaults (e.g. defaults that depend on the element's class)
	void BeautyString(CBeautyString& String, int level, const void* Object, int i,
		const void* Defaults = NULL);
	void ToString(CMMSString& String, const void* Object, int i, bool AddComma);
};

//Строка
class CStoredString : public CStoredProperty
{
private:
	GCString Value;

public:
	CStoredString() {}
	GCString& operator=(GCString& NewVal) { Value = NewVal; return Value; }
	GCString& operator()() { return Value; }
	operator GCString() {return Value;}
	
	virtual void SetValue(GCString& NewVal) {Value = NewVal;}
	
	virtual GCString BeautyString() {return Value;}
	virtual GCString MMSString() {return Value;}
};

// Логический тип
class CStoredBool : public CStoredProperty
{
private:
	bool Value;
	int Raw; //number as stored by 1C: may be other than 0/1 (e.g. 0xCDCDCDCD)

public:
	CStoredBool() { Value = false; Raw = 0; }
	bool operator=(bool NewVal) { Value = NewVal; Raw = NewVal ? 1 : 0; return Value; }
	bool operator()() { return Value; }
	operator bool() {return Value;}
	
	virtual void SetValue(GCString& Value);
	
	virtual GCString BeautyString();
	virtual GCString MMSString();
};

// Логический тип для флага "Полужирный" в параметрах шрифта
class CFontBold : public CStoredProperty
{
private:
	bool Value;
	bool Needed;
	int Raw; //lfWeight as stored by 1C when it is not 400 or 700 (e.g. 0), else -1

public:
	CFontBold() { Value = false; Needed = true; Raw = -1; };
	bool operator=(bool NewVal) { Value = NewVal; Raw = -1; return Value; };
	bool operator()() { return Value; };

	void UnNeeded() { Needed = false; }
	bool IsNeeded() { return Needed; }
	
	virtual void SetValue(GCString& Value);
	
	virtual GCString BeautyString();
	virtual GCString MMSString();
};

//Ссылка на объект метаданных
class CMMSObjRef : public CStoredProperty
{
public:
	int iTypeObjectID;
	GCString sTypeObjectID;

public:
	CMMSObjRef() { iTypeObjectID = 0; };
	
	virtual void SetValue(GCString& Value);
	
	virtual GCString BeautyString();
	virtual GCString MMSString();
};

//Ссылка на объект метаданных, у которой "0" и пустое значение различаются:
//значение, не являющееся ID, записывается как есть
class CMMSObjRefRaw : public CMMSObjRef
{
public:
	virtual GCString BeautyString()
	{
		if( iTypeObjectID == 0 ) return sTypeObjectID;
		return CMMSObjRef::BeautyString();
	}
	virtual GCString MMSString()
	{
		if( sTypeObjectID.Find('.') < 0 ) return sTypeObjectID;
		return CMMSObjRef::MMSString();
	}
};

//Слово с битовыми флагами
class CStoredBitSet : public CStoredProperty
{
private:
	char** BitNames;
	unsigned int Value;
	bool EmptyRaw; //stored as an empty string, not "0"; described as "Пусто"

public:
	CStoredBitSet() {BitNames = NULL; EmptyRaw = false;}
	void SetTable(char** Names) {BitNames = Names;}

	int BitByName(const char* BitName);
	bool BitValue(int i);

	unsigned int operator=(unsigned int NewVal) { Value = NewVal; EmptyRaw = false; return Value; };
	virtual void SetValue(GCString& Value);

	virtual GCString BeautyString();
	virtual GCString MMSString();
};

//Набор вариантов значений
class CStoredSet : public CStoredProperty
{
public:
	typedef struct {
		int value;
		char* representation;
	} TSetPair; //тип для записи таблицы соответствий

private:
	TSetPair* ValTable;
	int Value;

public:
	CStoredSet() {ValTable = NULL; Value = 0;}
	CStoredSet(int NewValue) {Value = NewValue;}
	void SetTable(TSetPair* aValTable) {ValTable = aValTable;}

	virtual void SetValue(GCString& NewValue);

	virtual GCString BeautyString();
	virtual GCString MMSString();
};

//Буква, обозначающая тип
class CMMSTypeLetter : public CStoredProperty
{
public:
	GCString TypeLetter;
	bool Numeric;

private:
	typedef struct {
		char letter;
		char* type;
	} TTypeTable;
	static TTypeTable type_table[];

public:
	CMMSTypeLetter() {TypeLetter = 'U'; Numeric = false;}

	virtual void SetValue(GCString& Value);

	virtual GCString BeautyString();
	virtual GCString MMSString();
	GCString AsNumber();
};

//Тип атрибута
class CMMSAttributeType : public CStoredObject
{
public:
	CMMSTypeLetter TypeLetter;
	int Length;
	int Precision; //точность (для чисел)
	CMMSObjRef TypeObjectID;

	bool NeedType; //для элементов диалоговых форм, где тип не нужен

private:
	static CObjectProperty Props[];
	static CPropertySet PropSet;

public:
	CMMSAttributeType() { Length = Precision = 0; NeedType = true; };
	void SetNumeric() {TypeLetter.Numeric = true;}
	//equals the type a compilation writes when the description has none
	bool IsDefault();
	CPropertySet* GetPropSet() {return &PropSet;};
	virtual void BeautyString(CBeautyString& String, int level);
};

//Буква, обозначающая область распространения
class CMigrationArea : public CStoredProperty
{
public:
	GCString Letter;

	CMigrationArea() { Letter = 'W'; };
	virtual void SetValue(GCString& Value);
	virtual GCString BeautyString();
	virtual GCString MMSString();
};

//Буква, обозначающая периодичность регистра
class CPeriod : public CStoredProperty
{
public:
	GCString PeriodLetter;

	CPeriod() { PeriodLetter = ' '; };
	virtual void SetValue(GCString& Value);
	virtual GCString BeautyString();
	virtual GCString MMSString();
};

#endif