/*
 * mfc-compat: free replacement for the non-GUI core of MFC 4.2 (ANSI/MBCS).
 * Copyright (C) 2026 Etersoft
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Implements CObject, CString (MFC 4.2 memory layout: one pointer to the
 * characters, preceded by CStringData), CTime, CTimeSpan and the basic macros.
 * Collections are in afxcoll.h and afxtempl.h. Written from the documented
 * MFC behaviour; no Microsoft code is used.
 */
#ifndef MFC_COMPAT_AFX_H
#define MFC_COMPAT_AFX_H

#ifdef _UNICODE
#error "mfc-compat supports only ANSI/MBCS builds"
#endif

#ifndef NOMINMAX
#define MFC_COMPAT_MINMAX   /* decided before libstdc++ defines NOMINMAX */
#endif

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <new>

/* FIXME: MSVC's <windef.h> defines the min/max macros for C++ as well, and
   MFC code relies on them. MinGW-w64 defines them only for C, and its
   libstdc++ (bits/os_defines.h) forces NOMINMAX. Provide them here unless the
   user defined NOMINMAX, so such code builds unchanged with MinGW GCC. */
#ifdef MFC_COMPAT_MINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#ifndef AFXAPI
#define AFXAPI
#endif
#define AFX_INLINE inline
#define AFX_CDECL __cdecl

#ifndef ASSERT
#define ASSERT(f) ((void)0)
#endif
#ifndef VERIFY
#define VERIFY(f) ((void)(f))
#endif
#ifndef ASSERT_VALID
#define ASSERT_VALID(p) ((void)0)
#endif
#ifndef TRACE
#define TRACE (void)
#endif
#define TRACE0(s) ((void)0)
#define TRACE1(s, a) ((void)0)
#define DEBUG_NEW new
#define DECLARE_DYNAMIC(c)
#define IMPLEMENT_DYNAMIC(c, b)
#define DECLARE_DYNCREATE(c)
#define IMPLEMENT_DYNCREATE(c, b)
#define DECLARE_SERIAL(c)
#define IMPLEMENT_SERIAL(c, b, s)

struct __POSITION {};
typedef __POSITION *POSITION;
#define BEFORE_START_POSITION ((POSITION)-1L)

class CObject
{
public:
    virtual ~CObject() {}
protected:
    CObject() {}
private:
    CObject(const CObject &);
    void operator=(const CObject &);
};

class CException : public CObject
{
public:
    virtual void Delete() { delete this; }
};

class CMemoryException : public CException {};

inline void AFXAPI AfxThrowMemoryException() { throw new CMemoryException; }

/* ------------------------------------------------------------------ */
/* CString                                                             */

struct CStringData
{
    long nRefs;        /* -1 for the empty string, else 1: copies are not shared */
    int nDataLength;   /* without the terminating zero */
    int nAllocLength;  /* without the terminating zero */
    TCHAR *data() { return (TCHAR *)(this + 1); }
};

class CString
{
public:
    CString() { Init(); }
    CString(const CString &s) { Init(); AssignCopy(s.GetData()->nDataLength, s.m_pchData); }
    CString(TCHAR ch, int nRepeat = 1)
    {
        Init();
        if (nRepeat > 0) { AllocBuffer(nRepeat); memset(m_pchData, ch, nRepeat); }
    }
    CString(LPCSTR psz) { Init(); int n = SafeStrlen(psz); if (n) { AllocBuffer(n); memcpy(m_pchData, psz, n); } }
    CString(LPCSTR pch, int nLength)
    {
        Init();
        if (nLength > 0) { AllocBuffer(nLength); memcpy(m_pchData, pch, nLength); }
    }
    CString(const unsigned char *psz) { Init(); *this = (LPCSTR)psz; }
    ~CString() { Release(); }

    int GetLength() const { return GetData()->nDataLength; }
    BOOL IsEmpty() const { return GetData()->nDataLength == 0; }
    void Empty() { Release(); Init(); }
    TCHAR GetAt(int i) const { return m_pchData[i]; }
    TCHAR operator[](int i) const { return m_pchData[i]; }
    void SetAt(int i, TCHAR ch) { m_pchData[i] = ch; }
    operator LPCTSTR() const { return m_pchData; }

    const CString &operator=(const CString &s)
    {
        if (m_pchData != s.m_pchData) AssignCopy(s.GetData()->nDataLength, s.m_pchData);
        return *this;
    }
    const CString &operator=(TCHAR ch) { AssignCopy(1, &ch); return *this; }
    const CString &operator=(LPCSTR psz) { AssignCopy(SafeStrlen(psz), psz); return *this; }
    const CString &operator=(const unsigned char *psz) { return *this = (LPCSTR)psz; }

    const CString &operator+=(const CString &s) { ConcatInPlace(s.GetData()->nDataLength, s.m_pchData); return *this; }
    const CString &operator+=(TCHAR ch) { ConcatInPlace(1, &ch); return *this; }
    const CString &operator+=(LPCTSTR psz) { ConcatInPlace(SafeStrlen(psz), psz); return *this; }

    friend CString AFXAPI operator+(const CString &a, const CString &b) { CString r; r.ConcatCopy(a.GetLength(), a.m_pchData, b.GetLength(), b.m_pchData); return r; }
    friend CString AFXAPI operator+(const CString &a, TCHAR ch) { CString r; r.ConcatCopy(a.GetLength(), a.m_pchData, 1, &ch); return r; }
    friend CString AFXAPI operator+(TCHAR ch, const CString &a) { CString r; r.ConcatCopy(1, &ch, a.GetLength(), a.m_pchData); return r; }
    friend CString AFXAPI operator+(const CString &a, LPCTSTR b) { CString r; r.ConcatCopy(a.GetLength(), a.m_pchData, SafeStrlen(b), b); return r; }
    friend CString AFXAPI operator+(LPCTSTR a, const CString &b) { CString r; r.ConcatCopy(SafeStrlen(a), a, b.GetLength(), b.m_pchData); return r; }

    int Compare(LPCTSTR psz) const { return strcmp(m_pchData, psz); }
    int CompareNoCase(LPCTSTR psz) const { return _stricmp(m_pchData, psz); }
    int Collate(LPCTSTR psz) const { return strcoll(m_pchData, psz); }
    int CollateNoCase(LPCTSTR psz) const { return _stricoll(m_pchData, psz); }

    CString Mid(int nFirst) const { return Mid(nFirst, GetLength() - nFirst); }
    CString Mid(int nFirst, int nCount) const
    {
        if (nFirst < 0) nFirst = 0;
        if (nCount < 0) nCount = 0;
        if (nFirst + nCount > GetLength()) nCount = GetLength() - nFirst;
        if (nFirst > GetLength()) nCount = 0;
        if (nFirst == 0 && nCount == GetLength()) return *this;
        CString r; r.AllocCopy(nCount, nFirst, m_pchData); return r;
    }
    CString Left(int nCount) const
    {
        if (nCount < 0) nCount = 0;
        if (nCount >= GetLength()) return *this;
        CString r; r.AllocCopy(nCount, 0, m_pchData); return r;
    }
    CString Right(int nCount) const
    {
        if (nCount < 0) nCount = 0;
        if (nCount >= GetLength()) return *this;
        CString r; r.AllocCopy(nCount, GetLength() - nCount, m_pchData); return r;
    }
    CString SpanIncluding(LPCTSTR set) const { return Left((int)strspn(m_pchData, set)); }
    CString SpanExcluding(LPCTSTR set) const { return Left((int)strcspn(m_pchData, set)); }

    void MakeUpper() { _strupr(m_pchData); }
    void MakeLower() { _strlwr(m_pchData); }
    void MakeReverse() { _strrev(m_pchData); }

    void TrimRight()
    {
        TCHAR *p = m_pchData, *last = NULL;
        for (; *p; p++) {
            if (_istspace((_TUCHAR)*p)) { if (!last) last = p; }
            else last = NULL;
        }
        if (last) { *last = 0; GetData()->nDataLength = (int)(last - m_pchData); }
    }
    void TrimRight(TCHAR ch)
    {
        TCHAR *p = m_pchData, *last = NULL;
        for (; *p; p++) {
            if (*p == ch) { if (!last) last = p; }
            else last = NULL;
        }
        if (last) { *last = 0; GetData()->nDataLength = (int)(last - m_pchData); }
    }
    void TrimRight(LPCTSTR set)
    {
        TCHAR *p = m_pchData, *last = NULL;
        for (; *p; p++) {
            if (strchr(set, *p)) { if (!last) last = p; }
            else last = NULL;
        }
        if (last) { *last = 0; GetData()->nDataLength = (int)(last - m_pchData); }
    }
    void TrimLeft()
    {
        const TCHAR *p = m_pchData;
        while (_istspace((_TUCHAR)*p)) p++;
        CutLeft(p);
    }
    void TrimLeft(TCHAR ch)
    {
        const TCHAR *p = m_pchData;
        while (ch == *p) p++;
        CutLeft(p);
    }
    void TrimLeft(LPCTSTR set)
    {
        const TCHAR *p = m_pchData;
        while (*p && strchr(set, *p)) p++;
        CutLeft(p);
    }

    int Replace(TCHAR chOld, TCHAR chNew)
    {
        int n = 0;
        if (chOld != chNew)
            for (TCHAR *p = m_pchData; *p; p++)
                if (*p == chOld) { *p = chNew; n++; }
        return n;
    }
    int Replace(LPCTSTR pszOld, LPCTSTR pszNew)
    {
        int nOld = SafeStrlen(pszOld);
        if (!nOld) return 0;
        int nNew = SafeStrlen(pszNew), nCount = 0;
        for (const TCHAR *p = m_pchData; (p = strstr(p, pszOld)) != NULL; p += nOld) nCount++;
        if (!nCount) return 0;
        CString r;
        int nLen = GetLength() + (nNew - nOld) * nCount;
        r.AllocBuffer(nLen);
        TCHAR *dst = r.m_pchData;
        const TCHAR *src = m_pchData, *hit;
        while ((hit = strstr(src, pszOld)) != NULL) {
            memcpy(dst, src, hit - src); dst += hit - src;
            memcpy(dst, pszNew, nNew); dst += nNew;
            src = hit + nOld;
        }
        strcpy(dst, src);
        *this = r;
        return nCount;
    }
    int Remove(TCHAR ch)
    {
        TCHAR *src = m_pchData, *dst = m_pchData;
        for (; *src; src++) if (*src != ch) *dst++ = *src;
        *dst = 0;
        int n = (int)(src - dst);
        GetData()->nDataLength -= n;
        return n;
    }
    int Insert(int nIndex, TCHAR ch) { TCHAR s[2] = {ch, 0}; return Insert(nIndex, s); }
    int Insert(int nIndex, LPCTSTR psz)
    {
        int nIns = SafeStrlen(psz), nLen = GetLength();
        if (nIndex < 0) nIndex = 0;
        if (nIndex > nLen) nIndex = nLen;
        if (nIns) {
            CString r; r.AllocBuffer(nLen + nIns);
            memcpy(r.m_pchData, m_pchData, nIndex);
            memcpy(r.m_pchData + nIndex, psz, nIns);
            memcpy(r.m_pchData + nIndex + nIns, m_pchData + nIndex, nLen - nIndex + 1);
            *this = r;
        }
        return GetLength();
    }
    int Delete(int nIndex, int nCount = 1)
    {
        int nLen = GetLength();
        if (nIndex < 0) nIndex = 0;
        if (nCount > 0 && nIndex < nLen) {
            if (nIndex + nCount > nLen) nCount = nLen - nIndex;
            memmove(m_pchData + nIndex, m_pchData + nIndex + nCount, nLen - nIndex - nCount + 1);
            GetData()->nDataLength = nLen - nCount;
        }
        return GetLength();
    }

    int Find(TCHAR ch) const { return Find(ch, 0); }
    int Find(TCHAR ch, int nStart) const
    {
        if (nStart < 0) nStart = 0;
        if (nStart >= GetLength()) return -1;
        const TCHAR *p = strchr(m_pchData + nStart, (_TUCHAR)ch);
        return p ? (int)(p - m_pchData) : -1;
    }
    int Find(LPCTSTR psz) const { return Find(psz, 0); }
    int Find(LPCTSTR psz, int nStart) const
    {
        if (nStart < 0) nStart = 0;
        if (nStart > GetLength()) return -1;
        const TCHAR *p = strstr(m_pchData + nStart, psz);
        return p ? (int)(p - m_pchData) : -1;
    }
    int ReverseFind(TCHAR ch) const
    {
        const TCHAR *p = strrchr(m_pchData, (_TUCHAR)ch);
        return p ? (int)(p - m_pchData) : -1;
    }
    int FindOneOf(LPCTSTR set) const
    {
        const TCHAR *p = strpbrk(m_pchData, set);
        return p ? (int)(p - m_pchData) : -1;
    }

    void AFX_CDECL Format(LPCTSTR fmt, ...) { va_list a; va_start(a, fmt); FormatV(fmt, a); va_end(a); }
    void FormatV(LPCTSTR fmt, va_list args)
    {
        int cap = 256;
        for (;;) {
            TCHAR *buf = (TCHAR *)malloc(cap + 1);
            if (!buf) AfxThrowMemoryException();
            va_list a;
            va_copy(a, args);
            int n = _vsnprintf(buf, cap, fmt, a);
            va_end(a);
            if (n >= 0 && n < cap) { buf[n] = 0; AssignCopy(n, buf); free(buf); return; }
            free(buf);
            cap *= 2;
        }
    }

    void AnsiToOem() { ::CharToOemA(m_pchData, m_pchData); }
    void OemToAnsi() { ::OemToCharA(m_pchData, m_pchData); }

    LPTSTR GetBuffer(int nMinBufLength)
    {
        if (nMinBufLength > GetData()->nAllocLength) {
            CStringData *old = GetData();
            int nOld = old->nDataLength;
            AllocBuffer(nMinBufLength);
            memcpy(m_pchData, old->data(), nOld + 1);
            GetData()->nDataLength = nOld;
            FreeData(old);
        }
        return m_pchData;
    }
    void ReleaseBuffer(int nNewLength = -1)
    {
        if (nNewLength == -1) nNewLength = (int)strlen(m_pchData);
        GetData()->nDataLength = nNewLength;
        m_pchData[nNewLength] = 0;
    }
    LPTSTR GetBufferSetLength(int nNewLength)
    {
        GetBuffer(nNewLength);
        GetData()->nDataLength = nNewLength;
        m_pchData[nNewLength] = 0;
        return m_pchData;
    }
    void FreeExtra()
    {
        if (GetData()->nDataLength != GetData()->nAllocLength) {
            CString r(m_pchData, GetLength());
            *this = r;
        }
    }
    LPTSTR LockBuffer() { return m_pchData; }
    void UnlockBuffer() {}
    BOOL LoadString(UINT nID)
    {
        TCHAR buf[1024];
        int n = ::LoadStringA(::GetModuleHandleA(NULL), nID, buf, sizeof buf);
        AssignCopy(n, buf);
        return n > 0;
    }

protected:
    LPTSTR m_pchData;

    CStringData *GetData() const { return ((CStringData *)m_pchData) - 1; }
    void Init() { m_pchData = EmptyData()->data(); }
    static CStringData *EmptyData()
    {
        static struct { CStringData d; TCHAR z; } nil = {{-1, 0, 0}, 0};
        return &nil.d;
    }
    static int SafeStrlen(LPCTSTR p) { return p ? (int)strlen(p) : 0; }
    /* The empty string is a per-module static with nRefs == -1 (as in MFC):
       recognise it by nRefs, not only by address, so that strings created in
       a DLL can be destroyed in the EXE and vice versa. The extra address
       checks let GCC see that the module's own empty string is never written
       or freed (-Wstringop-overflow, -Wfree-nonheap-object). */
    static bool IsEmptyData(const CStringData *d) { return d == EmptyData() || d->nRefs < 0; }
    static void FreeData(CStringData *d) { if (!IsEmptyData(d)) free(d); }
    void Release() { FreeData(GetData()); }
    void AllocBuffer(int nLen)
    {
        if (nLen == 0) { Init(); return; }
        /* MFC 4.2 release builds allocate strings up to 512 characters from
           fixed 64/128/256/512 blocks and report that size in nAllocLength.
           Programs may silently rely on the slack, so keep the same sizes. */
        int nAlloc = nLen <= 64 ? 64 : nLen <= 128 ? 128 : nLen <= 256 ? 256 : nLen <= 512 ? 512 : nLen;
        CStringData *d = (CStringData *)malloc(sizeof(CStringData) + nAlloc + 1);
        if (!d) AfxThrowMemoryException();
        d->nRefs = 1;
        d->nDataLength = nLen;
        d->nAllocLength = nAlloc;
        d->data()[nLen] = 0;
        m_pchData = d->data();
    }
    void AllocCopy(int nCount, int nIndex, LPCTSTR src = NULL)
    {
        if (!src) src = m_pchData;
        AllocBuffer(nCount);
        if (nCount) memcpy(m_pchData, src + nIndex, nCount);
    }
    void AssignCopy(int nLen, LPCTSTR src)
    {
        if (IsEmptyData(GetData()) || nLen > GetData()->nAllocLength) {
            CStringData *old = GetData();
            AllocBuffer(nLen);
            if (nLen) memmove(m_pchData, src, nLen);
            FreeData(old);
        } else {
            memmove(m_pchData, src, nLen);
            GetData()->nDataLength = nLen;
            m_pchData[nLen] = 0;
        }
    }
    void ConcatCopy(int n1, LPCTSTR s1, int n2, LPCTSTR s2)
    {
        int n = n1 + n2;
        if (n) {
            AllocBuffer(n);
            memcpy(m_pchData, s1, n1);
            memcpy(m_pchData + n1, s2, n2);
        }
    }
    void ConcatInPlace(int n, LPCTSTR src)
    {
        if (!n) return;
        CStringData *d = GetData();
        if (IsEmptyData(d) || d->nDataLength + n > d->nAllocLength) {
            CStringData *old = d;
            CString tmp;
            tmp.ConcatCopy(old->nDataLength, m_pchData, n, src);
            LPTSTR swap = m_pchData; m_pchData = tmp.m_pchData; tmp.m_pchData = swap;
        } else {
            memcpy(m_pchData + d->nDataLength, src, n);
            d->nDataLength += n;
            m_pchData[d->nDataLength] = 0;
        }
    }
    void CutLeft(const TCHAR *p)
    {
        if (p != m_pchData) {
            int n = GetLength() - (int)(p - m_pchData);
            memmove(m_pchData, p, n + 1);
            GetData()->nDataLength = n;
        }
    }
};

inline bool AFXAPI operator==(const CString &a, const CString &b) { return a.Compare(b) == 0; }
inline bool AFXAPI operator==(const CString &a, LPCTSTR b) { return a.Compare(b) == 0; }
inline bool AFXAPI operator==(LPCTSTR a, const CString &b) { return b.Compare(a) == 0; }
inline bool AFXAPI operator!=(const CString &a, const CString &b) { return a.Compare(b) != 0; }
inline bool AFXAPI operator!=(const CString &a, LPCTSTR b) { return a.Compare(b) != 0; }
inline bool AFXAPI operator!=(LPCTSTR a, const CString &b) { return b.Compare(a) != 0; }
inline bool AFXAPI operator<(const CString &a, const CString &b) { return a.Compare(b) < 0; }
inline bool AFXAPI operator<(const CString &a, LPCTSTR b) { return a.Compare(b) < 0; }
inline bool AFXAPI operator<(LPCTSTR a, const CString &b) { return b.Compare(a) > 0; }
inline bool AFXAPI operator>(const CString &a, const CString &b) { return a.Compare(b) > 0; }
inline bool AFXAPI operator>(const CString &a, LPCTSTR b) { return a.Compare(b) > 0; }
inline bool AFXAPI operator>(LPCTSTR a, const CString &b) { return b.Compare(a) < 0; }
inline bool AFXAPI operator<=(const CString &a, const CString &b) { return a.Compare(b) <= 0; }
inline bool AFXAPI operator<=(const CString &a, LPCTSTR b) { return a.Compare(b) <= 0; }
inline bool AFXAPI operator<=(LPCTSTR a, const CString &b) { return b.Compare(a) >= 0; }
inline bool AFXAPI operator>=(const CString &a, const CString &b) { return a.Compare(b) >= 0; }
inline bool AFXAPI operator>=(const CString &a, LPCTSTR b) { return a.Compare(b) >= 0; }
inline bool AFXAPI operator>=(LPCTSTR a, const CString &b) { return b.Compare(a) <= 0; }

/* ------------------------------------------------------------------ */
/* CTimeSpan, CTime                                                    */

class CTimeSpan
{
public:
    CTimeSpan() : m_timeSpan(0) {}
    CTimeSpan(time_t t) : m_timeSpan(t) {}
    CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs)
        : m_timeSpan(nSecs + 60 * (nMins + 60 * (nHours + 24 * (time_t)lDays))) {}
    LONG GetDays() const { return (LONG)(m_timeSpan / (24 * 3600)); }
    LONG GetTotalHours() const { return (LONG)(m_timeSpan / 3600); }
    int GetHours() const { return (int)(GetTotalHours() - GetDays() * 24); }
    LONG GetTotalMinutes() const { return (LONG)(m_timeSpan / 60); }
    int GetMinutes() const { return (int)(GetTotalMinutes() - GetTotalHours() * 60); }
    LONG GetTotalSeconds() const { return (LONG)m_timeSpan; }
    int GetSeconds() const { return (int)(GetTotalSeconds() - GetTotalMinutes() * 60); }
    CTimeSpan operator-(CTimeSpan o) const { return CTimeSpan(m_timeSpan - o.m_timeSpan); }
    CTimeSpan operator+(CTimeSpan o) const { return CTimeSpan(m_timeSpan + o.m_timeSpan); }
    const CTimeSpan &operator+=(CTimeSpan o) { m_timeSpan += o.m_timeSpan; return *this; }
    const CTimeSpan &operator-=(CTimeSpan o) { m_timeSpan -= o.m_timeSpan; return *this; }
    BOOL operator==(CTimeSpan o) const { return m_timeSpan == o.m_timeSpan; }
    BOOL operator!=(CTimeSpan o) const { return m_timeSpan != o.m_timeSpan; }
    BOOL operator<(CTimeSpan o) const { return m_timeSpan < o.m_timeSpan; }
    BOOL operator>(CTimeSpan o) const { return m_timeSpan > o.m_timeSpan; }
    /* %D days, %H hours (0-23), %M minutes, %S seconds, %% percent */
    CString Format(LPCTSTR fmt) const
    {
        CString r;
        TCHAR buf[32];
        for (; *fmt; fmt++) {
            if (*fmt != '%' || !fmt[1]) { r += *fmt; continue; }
            switch (*++fmt) {
            case 'D': sprintf(buf, "%ld", GetDays()); r += buf; break;
            case 'H': sprintf(buf, "%02d", GetHours()); r += buf; break;
            case 'M': sprintf(buf, "%02d", GetMinutes()); r += buf; break;
            case 'S': sprintf(buf, "%02d", GetSeconds()); r += buf; break;
            default: r += *fmt; break;
            }
        }
        return r;
    }
private:
    time_t m_timeSpan;
};

class CTime
{
public:
    static CTime PASCAL GetCurrentTime() { return CTime(::time(NULL)); }
    CTime() : m_time(0) {}
    CTime(time_t t) : m_time(t) {}
    CTime(int y, int mo, int d, int h, int mi, int s, int nDST = -1)
    {
        struct tm t = {};
        t.tm_sec = s; t.tm_min = mi; t.tm_hour = h;
        t.tm_mday = d; t.tm_mon = mo - 1; t.tm_year = y - 1900; t.tm_isdst = nDST;
        m_time = mktime(&t);
    }
    time_t GetTime() const { return m_time; }
    int GetYear() const { return GetLocalTm()->tm_year + 1900; }
    int GetMonth() const { return GetLocalTm()->tm_mon + 1; }
    int GetDay() const { return GetLocalTm()->tm_mday; }
    int GetHour() const { return GetLocalTm()->tm_hour; }
    int GetMinute() const { return GetLocalTm()->tm_min; }
    int GetSecond() const { return GetLocalTm()->tm_sec; }
    int GetDayOfWeek() const { return GetLocalTm()->tm_wday + 1; }
    struct tm *GetLocalTm(struct tm *ptm = NULL) const
    {
        struct tm *p = localtime(&m_time);
        if (ptm && p) { *ptm = *p; return ptm; }
        return p;
    }
    CTimeSpan operator-(CTime o) const { return CTimeSpan(m_time - o.m_time); }
    CTime operator-(CTimeSpan s) const { return CTime(m_time - s.GetTotalSeconds()); }
    CTime operator+(CTimeSpan s) const { return CTime(m_time + s.GetTotalSeconds()); }
    BOOL operator==(CTime o) const { return m_time == o.m_time; }
    BOOL operator!=(CTime o) const { return m_time != o.m_time; }
    BOOL operator<(CTime o) const { return m_time < o.m_time; }
    BOOL operator>(CTime o) const { return m_time > o.m_time; }
    CString Format(LPCTSTR fmt) const
    {
        TCHAR buf[256];
        struct tm *p = localtime(&m_time);
        if (!p || !strftime(buf, sizeof buf, fmt, p)) buf[0] = 0;
        return CString(buf);
    }
    CString FormatGmt(LPCTSTR fmt) const
    {
        TCHAR buf[256];
        struct tm *p = gmtime(&m_time);
        if (!p || !strftime(buf, sizeof buf, fmt, p)) buf[0] = 0;
        return CString(buf);
    }
private:
    time_t m_time;
};

#include <afxcoll.h>

#endif
