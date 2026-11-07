
#include <ctype.h>
#include "Declarations.h"
#include "Container.h"
#include "CommonFunctions.h"
#include "Storage.h"

CContainerRecord* CContainerRecord::DefVal;
CObjectProperty CContainerRecord::Props[] = {
	{"Type",		0, CObjectProperty::t_string, VAR_OFFSET(DefVal, Type)},
	{"StreamName",	1, CObjectProperty::t_string, VAR_OFFSET(DefVal, StreamName)},
	{"ObjectName",	2, CObjectProperty::t_string, VAR_OFFSET(DefVal, ObjectName)},
	{"Param4",		3, CObjectProperty::t_string, VAR_OFFSET(DefVal, Param4)},
	{NULL, 0, }
};
CPropertySet CContainerRecord::PropSet(Props, DefVal);

typedef struct
{
	LPCSTR FileName;
	CContainerInfo Info;
} CFileInfo;

static CFileInfo FileInfo[] = {
	{"Dialog Stream", {"DialogEditor", "Dialog Form"}},
	{"MD Programm text", {"TextDocument", "Module text"}},
	{"Page.", {"Moxcel.Worksheet", "Moxel WorkPlace"}},
	{"Gallery", {"PictureGalleryContainer", "Gallery"}},
	{"Main MetaData Stream", {"MetaDataStream", "Main MetaData Stream"}},
	{"GUIDData", {"GuidHistoryContainer", "GUIDData"}},
	{"TagStream", {"TagStream", "TagStream"}},
	{NULL, {NULL, NULL}}
};

static CFileInfo UIInfo[] = {
	{"Page.1", {"MenuEditorType", "Редактор меню"}},
	{"Page.2", {"ToolbarEditorType", "Редактор панелей инструментов"}},
	{NULL, {NULL, NULL}}
};


static CMapStringToPtr DirNameMap;
static CMapStringToPtr FileNameMap;
static CMapStringToPtr UINameMap;
static bool MapsInitialized = false;

CContainer::CContainer()
{
	sID = "Container.Contents";
	Content = "{\"Container.Contents\"";

	if( MapsInitialized ) return;

	DirNameMap.SetAt("Metadata",         (void*)"MetaDataContainer");
	DirNameMap.SetAt("Subconto",         (void*)"SubcontoContainer");
	DirNameMap.SetAt("SubList",          (void*)"SublistContainer");
	DirNameMap.SetAt("SubFolder",        (void*)"SubcontoGroupFolder");
	DirNameMap.SetAt("Document",         (void*)"DocumentContainer");
	DirNameMap.SetAt("Journal",          (void*)"JournalContainer");
	DirNameMap.SetAt("Report",           (void*)"ReportContainer");
	DirNameMap.SetAt("TypedText",        (void*)"TypedTextContainer");
	DirNameMap.SetAt("UserDef",          (void*)"UserDefContainer");
	DirNameMap.SetAt("Picture",          (void*)"PictureContainer");
	DirNameMap.SetAt("CalcJournal",      (void*)"CalcJournalContainer");
	DirNameMap.SetAt("CalcVar",          (void*)"CalcVarContainer");
	DirNameMap.SetAt("AccountChart",     (void*)"AccountChartContainer");
	DirNameMap.SetAt("AccountChartList", (void*)"AccountChartListContainer");
	DirNameMap.SetAt("OperationList",    (void*)"OperationListContainer");
	DirNameMap.SetAt("Operation",        (void*)"OperationContainer");
	DirNameMap.SetAt("GlobalData",       (void*)"GlobalDataContainer");
	DirNameMap.SetAt("ProvList",         (void*)"ProvListContainer");
	DirNameMap.SetAt("WorkBook",         (void*)"WorkBookContainer");

	DirNameMap.SetAt("ModuleText_Number", (void*)"ModuleContainer");
	DirNameMap.SetAt("Transact_Number",    (void*)"ModuleContainer");
	DirNameMap.SetAt("UserHelp_Number",    (void*)"ModuleContainer");

	int i;
	for( i = 0; FileInfo[i].FileName != NULL; i++ )
	{
		FileNameMap.SetAt(FileInfo[i].FileName, (CObject*) &(FileInfo[i].Info));
	}

	for( i = 0; UIInfo[i].FileName != NULL; i++ )
	{
		UINameMap.SetAt(UIInfo[i].FileName, (CObject*) &(UIInfo[i].Info));
	}

	MapsInitialized = true;
}


char* RemoveLastDigits(LPSTR Str)
{
	int len = strlen(Str);
	char* NewStr = new char[len+1];
	memcpy(NewStr, Str, len+1);

	LPSTR pos;
	pos = NewStr + len - 1;
	while( isdigit(*(unsigned char*)pos) ) *pos-- = '\0';
	return NewStr;
}


void CContainer::Add(LPCSTR Type, LPCSTR Name, LPCSTR Descr)
{
	Content += ",{\"";
	Content += Type;
	Content += "\",\"";
	Content += Name;
	Content += "\",\"";
	Content += Descr;
	Content += "\",\"\"}";
}

void CContainer::AddDir(LPSTR DirName)
{
	LPSTR Type;
	char* CleanName = RemoveLastDigits(DirName);

	if( !DirNameMap.Lookup(CleanName, (void*&)Type) )
		Type = "TypedObjectContainer";

	Add(Type, DirName, DirName);

	delete[] CleanName;
}

void CContainer::AddDir(const wchar_t* wDirName)
{
	char* DirName = new char[wcslen(wDirName)+1];
	sprintf(DirName, "%S", wDirName);
	AddDir(DirName);
	delete[] DirName;
}

void CContainer::AddFile(LPSTR FileName, CContainerType ContainerType)
{
	LPSTR CleanName = FileName;
	CContainerInfo *Info;

	switch( ContainerType )
	{
	case CT_UserInterfaces:
		if( UINameMap.Lookup(CleanName, (void*&)Info) )
			Add(Info->Type, FileName, Info->Descr);
		break;
	default:
		CleanName = RemoveLastDigits(FileName);
		if( FileNameMap.Lookup(CleanName, (void*&)Info) )
			Add(Info->Type, FileName, Info->Descr);
		delete[] CleanName;
		break;
	}
}

void CContainer::AddFile(const wchar_t* wFileName, CContainerType ContainerType)
{
	char* FileName = new char[wcslen(wFileName)+1];
	sprintf(FileName, "%S", wFileName);
	AddFile(FileName, ContainerType);
	delete[] FileName;
}


GCString& CContainer::Create(CStorage& Storage, bool Recursive, CContainerType Type)
{
	IEnumSTATSTG *ppenum;
	STATSTG stat;
	unsigned long uCount;
	GCString StorageName;

	Content = "{\"Container.Contents\"";

	Storage.GetStorage()->EnumElements(0, NULL, 0, &ppenum);
	while ( S_OK == ppenum->Next(1, &stat, &uCount) )
	{
		if( (stat.type & STGTY_STORAGE ) != 0 )
		{
			StorageName.Format("%S", stat.pwcsName);
			AddDir(stat.pwcsName);
			if( Recursive )
			{
				if( Storage.Open(StorageName) )
				{
					CContainer SubContainer;
					SubContainer.Create(Storage, Recursive);
					Storage.Close();
				}
			}
		}
		else
		{
			AddFile(stat.pwcsName, Type);
		}
	}
	ppenum->Release();

	Content += "}\n";

	GCString ContentsName("Container.Contents");
	if( !Storage.StreamExist(ContentsName) )
	{
		Storage.StreamFromString("Container.Contents", Content);
	}

	return Content;
}

GCString& CContainer::CreateInTypedText(CStorage& TypedTextStorage)
{
	Content = "{\"Container.Contents\",{\"MetaDataDescription\",\"MD Programm text\",\"MD Programm text\",\"\"}}\n";
	TypedTextStorage.StreamFromString("Container.Contents", Content);
	return Content;
}

