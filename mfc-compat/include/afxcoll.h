/*
 * mfc-compat: non-template MFC 4.2 collections.
 * Copyright (C) 2026 Etersoft
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef MFC_COMPAT_AFXCOLL_H
#define MFC_COMPAT_AFXCOLL_H

#include <afx.h>
#include <afx_collimpl.h>

class CByteArray : public CArray<BYTE, BYTE> {};
class CWordArray : public CArray<WORD, WORD> {};
class CDWordArray : public CArray<DWORD, DWORD> {};
class CUIntArray : public CArray<UINT, UINT> {};
class CPtrArray : public CArray<void *, void *> {};
class CObArray : public CArray<CObject *, CObject *> {};
class CStringArray : public CArray<CString, LPCTSTR>
{
public:
    int Add(LPCTSTR s) { return CArray<CString, LPCTSTR>::Add(s); }
    int Add(const CString &s) { int i = m_nSize; SetSize(i + 1, -1); m_pData[i] = s; return i; }
    void SetAt(int i, LPCTSTR s) { m_pData[i] = s; }
    void SetAtGrow(int i, LPCTSTR s) { CArray<CString, LPCTSTR>::SetAtGrow(i, s); }
};

class CPtrList : public CList<void *, void *>
{
public:
    CPtrList(int nBlockSize = 10) : CList<void *, void *>(nBlockSize) {}
};
class CObList : public CList<CObject *, CObject *>
{
public:
    CObList(int nBlockSize = 10) : CList<CObject *, CObject *>(nBlockSize) {}
};
class CStringList : public CList<CString, LPCTSTR>
{
public:
    CStringList(int nBlockSize = 10) : CList<CString, LPCTSTR>(nBlockSize) {}
};

template <> inline BOOL AFXAPI CompareElements<CString, LPCTSTR>(const CString *a, const LPCTSTR *b)
{
    return a->Compare(*b) == 0;
}

class CMapWordToPtr : public CMap<WORD, WORD, void *, void *> {};
class CMapPtrToWord : public CMap<void *, void *, WORD, WORD> {};
class CMapPtrToPtr : public CMap<void *, void *, void *, void *> {};
class CMapWordToOb : public CMap<WORD, WORD, CObject *, CObject *> {};
class CMapStringToPtr : public CMap<CString, LPCTSTR, void *, void *> {};
class CMapStringToOb : public CMap<CString, LPCTSTR, CObject *, CObject *> {};
class CMapStringToString : public CMap<CString, LPCTSTR, CString, LPCTSTR> {};

#endif
