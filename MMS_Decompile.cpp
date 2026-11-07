
#include "DLL/GComp.h"
#include "Storage.h"
#include "CommonFunctions.h"
#include "MMS.h"
#include "Container.h"
#include "DialogForm.h"
#include "UI.H"
#include "Rights.h"
#include "NameTable.h"
#include "Ordering.h"
#include "MMS_Filter.h"

///////////////////////////////////////////////////////////////////////
///////////  class CMMSObject methods  ////////////////////////////////
///////////////////////////////////////////////////////////////////////
void CMMSObject::ExtractString(CStorage& Storage, GCString& DestDir, GCString& Str, MMSObjectType MMS_Type)
{
	CNameTableRecord* nt_rec = NameTable.Find(MMS_Type);
	{
		GCString FileName = nt_rec->PrepareFileName(DestDir);
		Storage.CopyStringToFile(Str, FileName);
	}
}

bool CMMSObject::ExtractMetadata(GCString& DestDir, MMSObjectType MMS_Type, CStorage& TypedTextStorage)
{
	CNameTableRecord* nt_rec = NameTable.Find(MMS_Type);

	MMS_Filter.Down();
	bool ToExtract = MMS_Filter.Test(this, nt_rec);
	if( ToExtract )
	{
		CBeautyString Content;
		BeautyString(Content, 0);

		if( !nt_rec->Dir.IsEmpty() )
		{
			GCString Dir = nt_rec->PrepareDirName(DestDir, this);
			CreateDirectory(Dir);
			ExtractDescriptions(Dir+"\\", TypedTextStorage, true);
		}
		
		GCString FName = nt_rec->PrepareFileName(DestDir, this);
		CreateDirectory(DestDir);
		TypedTextStorage.CopyStringToFile(Content, FName);
	}
	MMS_Filter.Up();

	return ToExtract;
}

void CMMSObject::ExtractMetadata(GCString& DestDir, CStorage& TypedTextStorage)
{
	ExtractMetadata(DestDir, NT_MetadataPart, TypedTextStorage);
}

void CMMSObject::ExtractDescriptions(const GCString& FNamePrefix, CStorage& TypedTextStor, bool StdName)
{
	if( MMS_Type == MMS_Property ) return; //Вытаскиваем описания только для объектов

	CNameTableRecord* nt_rec_obj = NameTable.Find(MMS_Type);
	CNameTableRecord* nt_rec = NameTable.Find(NT_UserHelpStor);
	CNameTableRecord* nt_rec_file = NameTable.Find(NT_UserHelp);

	MMS_Filter.Down();
	if( MMS_Filter.Test(this, nt_rec_obj) || MMS_Filter.Test(NULL, nt_rec_file) )
	{
		GCString StorName = nt_rec->PrepareStorageName(ID);
		GCString NewPrefix = FNamePrefix;
		GCString Suffix;
		
		if( ID_Type != IDT_NoID && 
			(nt_rec_obj == NULL || nt_rec_obj->Type1C == "sID") )
		{
			if( ID_Type == IDT_OnlyTextual )
				Suffix.Format("%s.", (LPCSTR)sID);
			else if( ID_Type == IDT_OnlyNumeric )
				Suffix.Format("%s.%i.", (LPCSTR)nt_rec_file->FileName, ID);
			else if( sID.IsEmpty() || !IsValidFileName(sID) )
				Suffix.Format("%s.%i.", (LPCSTR)nt_rec_file->FileName, ID);
			else
				Suffix.Format("%s.", (LPCSTR)sID);
		}

		InplaceTranslit(Suffix);
		NewPrefix += Suffix;

		if( ID_Type == IDT_BothIDs && 
			!TypedTextStor.ObjectExtracted(this) && 
			TypedTextStor.StorageExist(StorName) )
		{
			GCString FName;

			if( StdName || Suffix.IsEmpty() )
				FName.Format("%s%s.%s", (LPCSTR)FNamePrefix, (LPCSTR)nt_rec_file->FileName, (LPCSTR)nt_rec_file->Ext);
			else
				FName.Format("%s%s", (LPCSTR)NewPrefix, (LPCSTR)nt_rec_file->Ext);

			TypedTextStor.Open(StorName);
			{
				GCString StreamName = nt_rec_file->PrepareStorageName(ID);
				TypedTextStor.CopyToFile(StreamName, FName);
			}
			TypedTextStor.AddToExtractedList(FName, this);
			TypedTextStor.Close();
		}

		if( MMS_Type != MMS_Plan)
		{
			for( int i = 0; i < GetNChildren(); i++ )
			{
				GetChild(i)->ExtractDescriptions(NewPrefix, TypedTextStor, false);
			}
		}
	}
	MMS_Filter.Up();
}

void CMMSObject::Decompile(GCString& Dir, CStorage& Storage, CStorage& TypedTextStorage)
{
	if( sID.IsEmpty() ) return;

	CNameTableRecord* nt_rec = NameTable.Find(MMS_Type);
	if( nt_rec == NULL ) return;

	MMS_Filter.Down();
	if( MMS_Filter.Test(this, nt_rec) )
	{
		GCString ObjDir;
		if( !nt_rec->Dir.IsEmpty() )
		{
			ObjDir = nt_rec->PrepareDirName(Dir, this);
			CreateDirectory(ObjDir);
		}
		else
			ObjDir = Dir;

		Msg(2, "%s", (LPCSTR)sID);

		//Метаданные
		ExtractMetadata(Dir, MMS_Type, TypedTextStorage);
		//Описания
		ExtractDescriptions(ObjDir+"\\", TypedTextStorage, true);
	}
	MMS_Filter.Up();

	SendInfoMessage(InfoMsg_Progress, ++TotalObjectsProcessed);
}

void CMMSObject::ExtractMoxelPages(GCString& DestDir, CStorage& WBStorage)
{
	CNameTableRecord* nt_rec_mxl = NameTable.Find(NT_MoxelSheet);
	CContainer WBContent;

	WBStorage.ParseStream(&WBContent, GCString("Container.Contents"), false);

	//Analyze Container.Contents and extract moxel pages if any.
	GCString std_tab_name("Moxel WorkPlace");
	GCString ID_mxl("Moxcel.Worksheet");
	GCString mxlFName;
	CObjectOrdering Order(3);
	GCString tab_name;
	
	for( int i = 0; i < WBContent.GetNChildren(); i++ )
	{
		CContainerRecord* rec = (CContainerRecord*)WBContent.GetChild(i);
		if( rec->Type != ID_mxl ) continue;

		tab_name = (LPCSTR)(rec->ObjectName); //преобразование нужно, чтобы строка скопировалась, а не просто сделался общий буфер
		GCString page_suffix = Suffix(rec->StreamName);
		if( tab_name == std_tab_name )
			tab_name = "Таблица";
		else
		{
			if( MangleFileName(tab_name) > 0 )
				Msg(2, "MXL %s replaced by %s", (LPCSTR)rec->ObjectName, (LPCSTR)tab_name);
		}

		mxlFName.Format("%s.%s", (LPCSTR)tab_name, (LPCSTR)nt_rec_mxl->Ext);

		if( TaskParameters.NoEmptyMxl ) //Поскипаем пустые шаблоны
		{
			long size = WBStorage.GetStreamSize(rec->StreamName);
			if( size == 147 || size == 139 ) continue;
		}

		if( !DirectoryExist(DestDir) ) CreateDirectory(DestDir);
		{
			GCString StreamName = rec->StreamName;
			GCString FileName = DestDir+"\\"+Translit(mxlFName);
			WBStorage.CopyToFile(StreamName, FileName);
		}
		
		Order.Add(page_suffix, (LPCSTR)mxlFName, (LPCSTR)Translit(mxlFName), (LPCSTR)rec->ObjectName);
	}

	if( Order.Order.GetLength() > 0 ) Order.WriteFile(WBStorage, DestDir);
}

void CMMSObject::ExtractWorkBook(GCString& DestDir, CStorage& Storage, CMMSObject* Parent)
{
	CNameTableRecord* nt_rec = NameTable.Find(NT_Workbook);

	if( TaskParameters.CompoundType != external_report ) //внутри внешнего отчета нет WorkBook, так что всё уже открыто
		if( !Storage.Open(nt_rec->StorageName) )
			return; //Контейнера нет, значит и делать нечего

	CreateDirectory(DestDir); // TODO проверить, а не лишний ли это вызов CreateDirectory()?

	//Модуль формы
	ExtractStreamByType(Storage, NT_FormModule, DestDir, true);

	//Сама форма
	CNameTableRecord* nt_rec_dlg = NameTable.Find(NT_DialogForm);
	MMS_Filter.Down();
	if( MMS_Filter.Test(this, nt_rec_dlg) )
	{
		GCString StorName = nt_rec_dlg->PrepareStorageName();
		if( Storage.StreamExist(StorName) )
		{
			Msg(2, "Form of: %i, %s", ID, (LPCSTR)sID);
			if( TaskParameters.NotParseForms )
			{
				ExtractStreamByType(Storage, NT_DialogForm, DestDir);
			}
			else
			{
				CDialogForm DialogForm(Parent);
				try
				{
					Storage.ParseStream(&DialogForm, StorName, true);
					DialogForm.DetectFormatting(Storage.LastParsedText);
					DialogForm.WriteToFile(Storage, nt_rec_dlg->PrepareFileName(DestDir));
				}
				catch(...) //чего-то там не распарсилось - тогда вытащим поток как есть
				{
					Msg(0, "WARN: error parsing form of %i: %s. Extracting stream as is.", ID, (LPCSTR)sID);
					ExtractStreamByType(Storage, NT_DialogForm, DestDir);
				}
			}
		}
	}
	MMS_Filter.Up();

	//Шаблоны таблиц
	MMS_Filter.Down();
	if( MMS_Filter.Test(GCString("ШаблоныТаблиц")) || MMS_Filter.Test(GCString("MoxelPages")) )
	{
		ExtractMoxelPages(DestDir, Storage);

		//Container.Profile выгружаем только в том случае, если он отличается от
		//стандартного, который создаётся при компиляции. Если потока нет,
		//пишем пустой файл: компиляция тогда его не создаёт.
		GCString ContProfName = "Container.Profile";
		GCString FileName = DestDir+"\\"+ContProfName;
		GCString Default = DefaultContainerProfile;
		long size = Storage.GetStreamSize(ContProfName);
		if( size < 0 )
		{
			GCString Empty;
			Storage.CopyStringToFile(Empty, FileName);
		}
		else if( Storage.OpenStream(ContProfName) )
		{
			if( size != Default.GetLength() || 0 != Storage.CompareWithString(Default) )
			{
				Storage.Rewind();
				Storage.CopyToFile(FileName);
			}
			Storage.CloseStream();
		}
	}
	MMS_Filter.Up();

	if( TaskParameters.CompoundType != external_report )
		Storage.Close();
}

void CMMSObject::ExtractListForms(CFormDescrList& Forms, GCString& Dir, CStorage& Storage, GCString Prefix)
{
	CNameTableRecord* nt_rec = NameTable.Find(NT_ListForm);
	GCString StorName;

	for( int i = 0; i < Forms.GetNChildren(); i++ )
	{
		CMMSObject* Form = Forms.GetChild(i);
		MMS_Filter.Down();
		if( MMS_Filter.Test(Form, nt_rec) || MMS_Filter.Test(GCString("ФормыСписков")) )
		{
			GCString FormDir;
			FormDir.Format("%s\\%s.%s", (LPCSTR)Dir, (LPCSTR)Translit(Form->sID), (LPCSTR)nt_rec->Ext);
			
			StorName.Format("%s%i", (LPCSTR)Prefix, Form->ID);
			if( Storage.Open(StorName) )
			{
				ExtractWorkBook(FormDir, Storage, this);
				Storage.Close();
			}
		}
		MMS_Filter.Up();
	}
}

void CMMSObject::ExtractStreamByType(CStorage& Storage, MMSObjectType MMS_Type, GCString& Dir, bool NotExtractEmpty)
{
	CNameTableRecord* nt_rec = NameTable.Find(MMS_Type);

	MMS_Filter.Down();
	if( MMS_Filter.Test(this, nt_rec) )
	{
		CStorage SubStorage(Storage);
		if( SubStorage.OpenStream(nt_rec->PrepareStorageName(ID)) )
		{
			SubStorage.GetCleanStream();
			if( !NotExtractEmpty || SubStorage.StreamInfo.Size > 0 )
				{
					GCString FileName = nt_rec->PrepareFileName(Dir);
					SubStorage.CopyToFile(FileName);
				}
		}
	}
	MMS_Filter.Up();
}

void CMMSObject::ExtractOrdering(CStorage& Storage, GCString& DestDir)
{
	if( TaskParameters.NoOrdering ) return;

	int nProps = GetNChildren();
	if( nProps == 0 ) return;

	CObjectOrdering Order(0);

	for( int i = 0; i < nProps; i++ )
	{
		CMMSObject* obj = GetChild(i);
		Order.Add(obj->sID);
	}

	Order.WriteFile(Storage, DestDir);
}

void CMMSAttributes_SF::Decompile(GCString& Dir, CStorage& Storage, CStorage& TypedTextStorage)
{
	if( sID.IsEmpty() ) return;

	CNameTableRecord* nt_rec = NameTable.Find(MMS_Type);
	if( nt_rec == NULL ) return;

	MMS_Filter.Down();
	if( MMS_Filter.Test(this, nt_rec) )
	{
		GCString ObjDir;
		if( !nt_rec->Dir.IsEmpty() )
			ObjDir = nt_rec->PrepareDirName(Dir, this);
		else
			ObjDir = Dir;

		Msg(2, "%s", (LPCSTR)sID);

		if( TaskParameters.MetadataBySubfolders )
		{
			for( int i = 0; i < GetNChildren(); i++ )
			{
				CMMSObject* Child = GetChild(i);
				GCString ChildDir = Child->GetObjectDir(ObjDir);
				Child->ExtractMetadata(ChildDir, TypedTextStorage);
				Child->ExtractDescriptions(ChildDir+"\\", TypedTextStorage, true);
			}
			ExtractOrdering(Storage, ObjDir);
		}
		else
		{
			//Метаданные
			ExtractMetadata(Dir, MMS_Type, TypedTextStorage);
			//Описания
			ExtractDescriptions(ObjDir+"\\", TypedTextStorage, true);
		}
	}
	MMS_Filter.Up();

	SendInfoMessage(InfoMsg_Progress, ++TotalObjectsProcessed);
}


///////////////////////////////////////////////////////////////////////
///////////  class CMMS methods  //////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void CMMS::ExtractStorage(GCString& Dir, CStorage& Storage, MMSObjectType MMS_Type, bool WithContainerCOntents)
{
	CNameTableRecord* nt_rec = NameTable.Find(MMS_Type);

	MMS_Filter.Down();
	if( MMS_Filter.Test(NULL, nt_rec) )
	{
		GCString DestDir = nt_rec->PrepareDirName(Dir);
		CStorage SrcStorage(Storage);

		if( SrcStorage.Open(nt_rec->PrepareStorageName(ID)) )
		{
			CreateDirectory(DestDir);
			if( !SrcStorage.Extract(DestDir, WithContainerCOntents) ) remove(DestDir);
		}
	}
	MMS_Filter.Up();
}

void CMMS::ExtractCommonMXL(GCString& Dir, CStorage& Storage)
{
	CNameTableRecord* nt_rec_gd = NameTable.Find(NT_GlobalData);
	CNameTableRecord* nt_rec = NameTable.Find(NT_CommonMXL);

	MMS_Filter.Down();
	if( MMS_Filter.Test(NULL, nt_rec) )
	{
		GCString GDDir = nt_rec->PrepareDirName(Dir);
		
		GCString StorName = nt_rec_gd->StorageName;
		StorName += "\\";
		StorName += nt_rec->StorageName;
		
		//the whole work book: GComp 2.2.15 took only the moxel pages and lost
		//the form, its module and Container.Profile
		CStorage GDStorage(Storage);
		if( GDStorage.Open(StorName) )
		{
			ExtractWorkBook(GDDir, GDStorage, NULL);
		}
	}
	MMS_Filter.Up();
}


/*
void CMMS::DeleteUnusedObjects(CStorage& Storage)
{
	int i;

	for( i = 0; i < GetNChildren(); i++ )
		GetChild(i)->Used = true;


	CMMSObject* frm = Storage.ParseStream(NameTable.Find(NT_DialogForm)->PrepareStorageName(0), false);
	if( frm == NULL )
		return;

	CMMSObject* Controls = frm->FindChild(GCString("Controls"));
	for( i = 0; i < Controls->GetNChildren(); i++ )
	{
		int nProps = Controls->GetChild(i)->GetNChildren();
		GCString& sWndClass = Controls->GetChild(i)->GetChild(0)->sID;
		GCString sTypeLetter, sTypeID;
		if( isdigit(sWndClass[0]) ) //Не врубаюсь, как 1С читает свой дебильный формат?!
		{
			sTypeLetter = Controls->GetChild(i)->GetChild(12)->sID;
			sTypeID = Controls->GetChild(i)->GetChild(15)->sID;
		}
		else
		{
			sTypeLetter = Controls->GetChild(i)->GetChild(13)->sID;
			sTypeID = Controls->GetChild(i)->GetChild(16)->sID;
		}

		int ID = atoi(sTypeID);
		if( ID != 0 ) 
			MarkUsed(ID);
	}

	for( i = 0; i < GetNChildren(); i++ )
	{
		if( GetChild(i)->MMS_Type == MMS_Buh ) continue;
		if( GetChild(i)->MMS_Type == MMS_TaskItem ) continue;
		GetChild(i)->DeleteUnused();
	}

	delete frm;
}
*/

void CMMS::ExtractPictureGallery(GCString& Dir, CStorage& Storage)
{
	CNameTableRecord* nt_rec = NameTable.Find(NT_PictureGallery);
	GCString DestDir = nt_rec->PrepareDirName(Dir);
	CStorage SrcStorage(Storage);

	if( SrcStorage.Open(nt_rec->PrepareStorageName(ID)) )
	{
		CreateDirectory(DestDir);
		if( !SrcStorage.Extract(DestDir, false) ) remove(DestDir);
	}
}

void CMMS::BuildObjNameCache()
{
	//Build names cache 
	CMMSObject::BuildObjNameCache(ObjNameCache, GCString());
	//sorting by ID
	qsort(ObjNameCache.GetData(), ObjNameCache.GetSize(), 
		sizeof(TObjNameCacheRecord), 
		ObjNameCache_compare_id);
}

void CMMS::BuildObjNameCache(TCompareFunction sort_method, const GCString& Dir, CStorage& Storage)
{
	//Build names cache 
	CMMSObject::BuildObjNameCache(ObjNameCache, GCString());

	// and save it
	if( TaskType == TaskType_Decompile )
	{
		GCString NamesStr, str;
		for( int i = 0; i < ObjNameCache.GetSize(); i++ )
		{
			str.Format("%10i\t%s\r\n", ObjNameCache[i].ID, (LPCSTR)ObjNameCache[i].Name);
			NamesStr += str;
		}
		CNameTableRecord* nt_rec = NameTable.Find(NT_ObjNameTable);
		GCString FileName = nt_rec->PrepareFileName(Dir);
		if( !(TaskParameters.CompoundType == external_report && TaskParameters.TruncateMMS) )
			Storage.CopyStringToFile(NamesStr, FileName);
	}

	//Проверка уникальности ID
	CheckObjNameCache();

	//sorting by ID only after saving
	qsort(ObjNameCache.GetData(), ObjNameCache.GetSize(), 
		sizeof(TObjNameCacheRecord), 
		sort_method);
}

void CMMS::CheckObjNameCache()
{
	int i, sz = ObjNameCache.GetSize();
	bool error = false;
	qsort(ObjNameCache.GetData(), ObjNameCache.GetSize(), sizeof(TObjNameCacheRecord), ObjNameCache_compare_id);
	for( i = 1; i < sz; i++ )
	{
		if( ObjNameCache[i-1].ID == ObjNameCache[i].ID )
		{
			//1C itself allows it (e.g. a calendar's holidays and an attribute
			//share an ID); compilation maps names to IDs, so it is harmless
			Msg(0, "WARN: Неуникальный числовой идентификатор - %i: %s, %s", ObjNameCache[i].ID,
				(LPCSTR)ObjNameCache[i-1].Name, (LPCSTR)ObjNameCache[i].Name);
		}
	}
	qsort(ObjNameCache.GetData(), ObjNameCache.GetSize(), sizeof(TObjNameCacheRecord), ObjNameCache_compare_str);
	for( i = 1; i < sz; i++ )
	{
		if( ObjNameCache[i-1].Name == ObjNameCache[i].Name )
		{
			Msg(0, "ERR: Неуникальный текстовый идентификатор - %i: %s", ObjNameCache[i].ID, (LPCSTR)ObjNameCache[i].Name);
			error = true;
		}
	}
	for( i = 0; i < sz; i++ )
	{
		if( ObjNameCache[i].ID == 0 )
		{
			Msg(0, "ERR: Нулевой числовой идентификатор для %s", (LPCSTR)ObjNameCache[i].Name);
			error = true;
		}
	}
	if( error && TaskType == TaskType_Compile )
	{
		Msg(0, "Компиляция прервана");
		throw NULL;
	}
}

bool CMMS::ReadObjNameCache(GCString& FileName)
{
	if( !FileExist(FileName) )
		return false;
	
	CLexer Lexer(FileName);
	GCString Token, Value;
	TObjNameCacheRecord new_entry;
	while( Lexer.GetToken(Token, " \t\r\n") )
	{
		Lexer.GetRest(Value);
		new_entry.ID = atoi(Token);
		new_entry.Name = Value;
		ObjNameCache.Add(new_entry);
	}

	qsort(ObjNameCache.GetData(), ObjNameCache.GetSize(), 
		sizeof(TObjNameCacheRecord), 
		ObjNameCache_compare_str);

	return true;
}

void CMMS::ReadObjNameCacheFromDir(GCString& Dir)
{
	if( TaskType != TaskType_Compile )
		return; //читаем кэш только при компиляции

	CNameTableRecord* nt_rec = NameTable.Find(NT_ObjNameTable);
	GCString FileName = nt_rec->PrepareFileName(Dir);
	ReadObjNameCache(FileName);
}

void CMMS::Decompile(GCString& Dir, CStorage& Storage, CStorage& TypedTextStorage)
{
	//Build names cache and save it
	BuildObjNameCache(ObjNameCache_compare_id, Dir, Storage);

	//Если это внешний отчёт, то отработаем по упрощённой схеме
	if( TaskParameters.CompoundType == external_report	)
	{
		//DeleteUnusedObjects(Storage);
		if( !TaskParameters.TruncateMMS )
			ExtractMetadata(Dir, TypedTextStorage);
		ExtractWorkBook(Dir, Storage, NULL);
		ExtractStreamByType(Storage, NT_ErtUserHelp, Dir, true);
		ExtractStorage(Dir, Storage, NT_PictureGallery, false);
		return;
	}

	SendInfoMessage(InfoMsg_Total, CountObjects());
	TotalObjectsProcessed = 0;

	int nProps = GetNChildren();
	for( int i = 0; i < nProps; i++ )
	{
		CMMSObject *obj = GetChild(i);
		obj->Decompile(Dir, Storage, TypedTextStorage);
	}

	//Глобальный модуль
	ExtractStreamByType(TypedTextStorage, NT_GlobalModule, Dir);
	//Общие таблицы
	ExtractCommonMXL(Dir, Storage);
	
	//Картинки
	ExtractStorage(Dir, Storage, NT_PictureGallery, false);

	//Юзерские интерфейсы
	DecompileUI(Dir, Storage);
	//Юзерские права
	DecompileUserRights(Dir, Storage);

	//Служебные потоки
	ExtractStreamByType(Storage, NT_GUIDData, Dir);
	ExtractStreamByType(Storage, NT_TagStream, Dir);

	//Описания
	CNameTableRecord* nt_rec_descr = NameTable.Find(NT_UserHelpStor);
	GCString DescrDir = nt_rec_descr->PrepareDirName(Dir);
	ExtractDescriptions(DescrDir+"\\", TypedTextStorage, false);
}

///////////////////////////////////////////////////////////////////////
///////////  class CMMSReport methods  ////////////////////////////////
///////////////////////////////////////////////////////////////////////
void CMMSReport::Decompile(GCString& Dir, CStorage& Storage, GCString& Prefix, CStorage& TypedTextStorage)
{
	MMS_Filter.Down();
	if( MMS_Filter.Test(this, NULL) )
	{
		GCString StorName;

		Msg(2, "Отчет (обработка) %s", (LPCSTR)sID);

		GCString RptDir = GetObjectDir(Dir);
		CreateDirectory(RptDir);
		
		StorName.Format("%s%i", (LPCSTR)Prefix, ID);
		if( Storage.Open(StorName) )
		{
			ExtractWorkBook(RptDir, Storage, NULL);
			Storage.Close();
		}

		ExtractMetadata(RptDir, TypedTextStorage);
		ExtractDescriptions(RptDir+"\\", TypedTextStorage, true);
	}
	MMS_Filter.Up();

	SendInfoMessage(InfoMsg_Progress, ++TotalObjectsProcessed);
}

///////////////////////////////////////////////////////////////////////
///////////  class CMMSReportList methods  /////////////////////////////
///////////////////////////////////////////////////////////////////////
void CMMSReportList::Decompile(GCString& Dir, CStorage& Storage, CStorage& TypedTextStorage)
{
	int nChildren = GetNChildren();
	if( nChildren== 0 ) return;

	CNameTableRecord* nt_rec = NameTable.Find(sID);

	MMS_Filter.Down();
	if( MMS_Filter.Test(this, nt_rec) )
	{
		GCString ReportsDir = nt_rec->PrepareDirName(Dir);

		CreateDirectory(ReportsDir);

		if( Storage.Open(nt_rec->StorageName) )
		{
			GCString ReportStorName = nt_rec->StorageName + "_Number";
			for( int i = 0; i < nChildren; i++ )
			{
				CMMSReport* rpt = (CMMSReport*)GetChild(i);
				rpt->Decompile(ReportsDir, Storage, ReportStorName, TypedTextStorage);
			}
			Storage.Close();
		}

		ExtractOrdering(Storage, ReportsDir);
	}
	MMS_Filter.Up();
}

