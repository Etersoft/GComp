
#ifndef STORAGE_H
#define STORAGE_H


#include "Declarations.h"
#include "StreamInfo.h"

#include "ZLibEngine.h"

void SetIOPre(LPCTSTR MsgPrefix, LPCTSTR FileName);

class CStorage
{
private:
	static IzlibEnginePtr iLibEngine;
public:
	static void InitZLibEngine();
	static void ReleaseZLibEngine();

protected:
	CList<IStorage*, IStorage*&> Storages;
	bool IsRootStorage;
	void Push(IStorage* pStorage);
	IStorage* Pop();

	//Хэш-таблица файлов и объектов, выгруженных в текущей сессии. 
	//Нужна для последующей очистки каталога от старых файлов
	CMapStringToPtr* ExtractedFiles;
	//Доп. хэш-таблица объектов. В основном используется чтобы избежать повторной выгрузки описаний
	CMapPtrToPtr*    ExtractedObjects;

public:
	IStream* pStream;
	GCString StreamName;
	CStreamInfo StreamInfo;

private:
	void SetParentStorage(CStorage& Storage);

public:
	CStorage();
	CStorage(CStorage& Storage);
	//В основном этот конструктор вызывается с обычной строкой, поэтому для SubStorage
	//не будем делать ссылочный тип - пусть конструктор каждый раз вызывается
	CStorage(CStorage& Storage, GCString SubStorage);
	~CStorage();

public:
	GCString FullPath();

	bool OpenCompound(const GCString& CompoundFileName);
	bool Open(CStringArray* StorageName);
	bool Open(const GCString& StorageName);
	bool CreateCompound(const GCString& CompoundFileName);
	bool Create(CStringArray* StorageName);
	bool Create(const GCString& StorageName);
	bool Delete(const GCString& StorageName);
	void Close(bool CloseAll = false);

	bool OpenStream(const GCString& StreamName);
	bool CreateStream(const GCString& StreamName);
	void CloseStream();

	bool StorageExist(GCString& StorageName);
	bool StreamExist(const GCString& StreamName);
	long GetStreamSize(GCString& StreamName);

	void Rewind();

	void GetCleanStream();
	bool CopyToFile(FILE* File, unsigned long bytes_to_copy);
	bool CopyToFile(GCString& FileName, bool OnlyRestOfSream = false);
	bool CopyToFile(GCString& StreamName, GCString& FileName);
	bool Extract(GCString& DestDir, bool WithContainerContents = true);
	bool CopyStringToFile(GCString& String, GCString& FileName);

	bool StreamFromFile(GCString& StreamName, GCString& FileName);
	bool StreamFromString(GCString StreamName, GCString& String);

	unsigned long AppendFile(IStream* pStream, FILE* File);
	unsigned long AppendFile(IStream* pStream, const GCString& FileName);
	void AddZeroByte();

	bool FromFile(GCString& FileName);
	bool FromFile(FILE* File, LPCSTR FileName = NULL);
	bool StorageFromDir(const GCString& StorageName, const GCString& SrcDir);

	//Сравнивает текущий поток с указанным файлом. Сравнение начинается с 
	//текущей позиции (как в потоке, так и в файле). Если указан bytes_to_compare > 0, 
	//то сравниваем только указанное количество байт. Если не указан - сравниваем до 
	//конца потока или файла.
	int Compare(FILE* File, unsigned long bytes_to_compare);
	int Compare(GCString& FileName, unsigned long bytes_to_compare);
	int CompareWithString(GCString& str2);
	//Операторы сравнения потока целиком (перед сравнением происходит перепозиционирование)
	int FullCompare(GCString& FileName);

	IStorage* GetStorage();

	void ParseStream(CMMSObject* Object, const GCString& StreamName, bool ParseWitnNoIDs);
	//raw text of the stream last given to ParseStream (formatting details)
	GCString LastParsedText;

	void AddToExtractedList(GCString& FullFName, void* object);
	bool ObjectExtracted(void* object);
	int PruneDirectory(GCString& Path, bool NoEmptyFolders);

private:
	void ReleaseMaps();
	void WriteSizeToStream(IStream* pStream);
	bool ExtractPictureGallery(GCString& FileName);
	
	bool GalleryImported; //флаг - чтобы не импортировать Gallery 3 раза
	bool ImportPictureGallery(GCString& StreamName, GCString& FileName);

	void RemoveDirectory(GCString& Path);
};



LPCSTR GetStgErrorString(HRESULT err);

ULARGE_INTEGER GetStreamSize(IStream* pStream);

IStorage* OpenStorage(IStorage* pParentStorage, wchar_t* wStorName);
IStorage* OpenStorage(IStorage* pParentStorage, const GCString& StorName);

IStream* OpenStream(IStorage* pParentStorage, wchar_t* wStreamName);
IStream* OpenStream(IStorage* pParentStorage, const GCString& StreamName);

bool StringDifferFromFile(GCString& String, GCString& FileName);
bool CopyStringToFile(GCString& String, GCString& FileName);

//Text files of the source tree (by extension): with "encoding: UTF-8" they
//are converted from/to CP1251, the rest (pictures, work books...) never
bool IsTextFileName(LPCSTR FileName);
bool IsUtf8File(LPCSTR FileName);
//fopen() for reading a file of the source tree: a UTF-8 text file is
//converted to CP1251 first; a character without CP1251 stops the work
FILE* OpenSourceFile(LPCSTR FileName, const char* Mode);

#endif