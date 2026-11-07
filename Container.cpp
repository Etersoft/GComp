
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


/* Storages in the order 1C 7.7 lists them in Container.Contents. */
static const struct { LPCSTR Name; LPCSTR Type; } DirInfo[] = {
	{"Metadata", "MetaDataContainer"},
	{"Subconto", "SubcontoContainer"},
	{"SubList", "SublistContainer"},
	{"SubFolder", "SubcontoGroupFolder"},
	{"Document", "DocumentContainer"},
	{"Journal", "JournalContainer"},
	{"Report", "ReportContainer"},
	{"TypedText", "TypedTextContainer"},
	{"UserDef", "UserDefContainer"},
	{"Picture", "PictureContainer"},
	{"CalcJournal", "CalcJournalContainer"},
	{"CalcVar", "CalcVarContainer"},
	{"AccountChart", "AccountChartContainer"},
	{"AccountChartList", "AccountChartListContainer"},
	{"OperationList", "OperationListContainer"},
	{"Operation", "OperationContainer"},
	{"GlobalData", "GlobalDataContainer"},
	{"ProvList", "ProvListContainer"},
	{"WorkBook", "WorkBookContainer"},
	{"ModuleText_Number", "ModuleContainer"},
	{"Transact_Number", "ModuleContainer"},
	{"UserHelp_Number", "ModuleContainer"},
	{NULL, NULL}
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

	int i;
	for( i = 0; DirInfo[i].Name != NULL; i++ )
		DirNameMap.SetAt(DirInfo[i].Name, (void*)DirInfo[i].Type);

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


/* Position of an element in Container.Contents: 1C writes the known
   storages and streams in a fixed order (that of DirInfo, FileInfo and
   UIInfo); elements with the same rank, such as numbered ones, keep the
   order of the storage enumeration. */
static int ContentsRank(LPCSTR Name, bool IsDir, CContainerType Type)
{
	char* CleanName = RemoveLastDigits((LPSTR)Name);
	int rank = 1000;
	int i;
	if( IsDir )
	{
		for( i = 0; DirInfo[i].Name != NULL; i++ )
			if( strcmp(CleanName, DirInfo[i].Name) == 0 ) { rank = i; break; }
	}
	else if( Type == CT_UserInterfaces )
	{
		for( i = 0; UIInfo[i].FileName != NULL; i++ )
			if( strcmp(Name, UIInfo[i].FileName) == 0 ) { rank = 100 + i; break; }
	}
	else
	{
		for( i = 0; FileInfo[i].FileName != NULL; i++ )
			if( strcmp(CleanName, FileInfo[i].FileName) == 0 ) { rank = 100 + i; break; }
	}
	delete[] CleanName;
	return rank;
}

GCString& CContainer::Create(CStorage& Storage, bool Recursive, CContainerType Type)
{
	CStringArray Items;
	CArray<int, int> Ranks;
	int ItemStart;
	char* ElemName;
	IEnumSTATSTG *ppenum;
	STATSTG stat;
	unsigned long uCount;
	GCString StorageName;

	Content = "{\"Container.Contents\"";

	Storage.GetStorage()->EnumElements(0, NULL, 0, &ppenum);
	while ( S_OK == ppenum->Next(1, &stat, &uCount) )
	{
		ItemStart = Content.GetLength();
		ElemName = new char[wcslen(stat.pwcsName)+1];
		sprintf(ElemName, "%S", stat.pwcsName);
		if( (stat.type & STGTY_STORAGE ) != 0 )
		{
			StorageName.Format("%S", stat.pwcsName);
			AddDir(stat.pwcsName);
			Ranks.Add(ContentsRank(ElemName, true, Type));
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
			Ranks.Add(ContentsRank(ElemName, false, Type));
		}
		delete[] ElemName;
		CoTaskMemFree(stat.pwcsName);
		Items.Add(Content.Mid(ItemStart));
		Content = Content.Left(ItemStart);
	}
	ppenum->Release();

	//stable insertion sort by rank
	int i, j;
	for( i = 1; i < Items.GetSize(); i++ )
	{
		CString item = Items[i];
		int rank = Ranks[i];
		for( j = i; j > 0 && Ranks[j-1] > rank; j-- )
		{
			Items[j] = Items[j-1];
			Ranks[j] = Ranks[j-1];
		}
		Items[j] = item;
		Ranks[j] = rank;
	}
	for( i = 0; i < Items.GetSize(); i++ )
		Content += Items[i];

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

