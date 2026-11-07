/*
 * mfc-compat: CArray, CList, CMap with MFC 4.2 semantics.
 * Copyright (C) 2026 Etersoft
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Behaviour kept from MFC 4.2 where programs can observe it: array growth
 * policy and bitwise relocation of elements, zero-filled construction,
 * hash functions, insertion at the head of a bucket chain and the
 * bucket-order iteration of GetNextAssoc.
 */
#ifndef MFC_COMPAT_COLLIMPL_H
#define MFC_COMPAT_COLLIMPL_H

template <class TYPE>
inline void AFXAPI ConstructElements(TYPE *p, int n)
{
    memset((void *)p, 0, n * sizeof(TYPE));
    for (; n--; p++) ::new ((void *)p) TYPE;
}

template <class TYPE>
inline void AFXAPI DestructElements(TYPE *p, int n)
{
    for (; n--; p++) p->~TYPE();
}

template <class TYPE>
inline void AFXAPI CopyElements(TYPE *dst, const TYPE *src, int n)
{
    while (n--) *dst++ = *src++;
}

template <class TYPE, class ARG_TYPE>
inline BOOL AFXAPI CompareElements(const TYPE *a, const ARG_TYPE *b)
{
    return *a == *b;
}

template <class ARG_KEY>
inline UINT AFXAPI HashKey(ARG_KEY key)
{
    return ((UINT)(UINT_PTR)(void *)(DWORD_PTR)key) >> 4;
}

inline UINT AFXAPI MfcHashString(LPCSTR key)
{
    UINT nHash = 0;
    while (*key) nHash = (nHash << 5) + nHash + (UINT)(int)*key++;
    return nHash;
}

template <> inline UINT AFXAPI HashKey<LPCSTR>(LPCSTR key) { return MfcHashString(key); }
template <> inline UINT AFXAPI HashKey<const CString &>(const CString &key) { return MfcHashString(key); }

/* ------------------------------------------------------------------ */

template <class TYPE, class ARG_TYPE = const TYPE &>
class CArray : public CObject
{
public:
    CArray() : m_pData(NULL), m_nSize(0), m_nMaxSize(0), m_nGrowBy(0) {}
    ~CArray() { RemoveAll(); }

    int GetSize() const { return m_nSize; }
    int GetCount() const { return m_nSize; }
    BOOL IsEmpty() const { return m_nSize == 0; }
    int GetUpperBound() const { return m_nSize - 1; }
    void SetSize(int nNewSize, int nGrowBy = -1)
    {
        if (nGrowBy != -1) m_nGrowBy = nGrowBy;
        if (nNewSize == 0) {
            if (m_pData) {
                DestructElements<TYPE>(m_pData, m_nSize);
                delete[] (BYTE *)m_pData;
                m_pData = NULL;
            }
            m_nSize = m_nMaxSize = 0;
        } else if (!m_pData) {
            m_pData = (TYPE *)new BYTE[nNewSize * sizeof(TYPE)];
            ConstructElements<TYPE>(m_pData, nNewSize);
            m_nSize = m_nMaxSize = nNewSize;
        } else if (nNewSize <= m_nMaxSize) {
            if (nNewSize > m_nSize) ConstructElements<TYPE>(&m_pData[m_nSize], nNewSize - m_nSize);
            else if (m_nSize > nNewSize) DestructElements<TYPE>(&m_pData[nNewSize], m_nSize - nNewSize);
            m_nSize = nNewSize;
        } else {
            int grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                grow = grow < 4 ? 4 : (grow > 1024 ? 1024 : grow);
            }
            int nNewMax = nNewSize < m_nMaxSize + grow ? m_nMaxSize + grow : nNewSize;
            TYPE *p = (TYPE *)new BYTE[nNewMax * sizeof(TYPE)];
            memcpy((void *)p, (void *)m_pData, m_nSize * sizeof(TYPE));
            ConstructElements<TYPE>(&p[m_nSize], nNewSize - m_nSize);
            delete[] (BYTE *)m_pData;
            m_pData = p;
            m_nSize = nNewSize;
            m_nMaxSize = nNewMax;
        }
    }
    void FreeExtra()
    {
        if (m_nSize == m_nMaxSize) return;
        TYPE *p = NULL;
        if (m_nSize) {
            p = (TYPE *)new BYTE[m_nSize * sizeof(TYPE)];
            memcpy((void *)p, (void *)m_pData, m_nSize * sizeof(TYPE));
        }
        delete[] (BYTE *)m_pData;
        m_pData = p;
        m_nMaxSize = m_nSize;
    }
    void RemoveAll() { SetSize(0, -1); }
    TYPE GetAt(int i) const { return m_pData[i]; }
    void SetAt(int i, ARG_TYPE v) { m_pData[i] = v; }
    TYPE &ElementAt(int i) { return m_pData[i]; }
    const TYPE *GetData() const { return m_pData; }
    TYPE *GetData() { return m_pData; }
    void SetAtGrow(int i, ARG_TYPE v)
    {
        if (i >= m_nSize) SetSize(i + 1, -1);
        m_pData[i] = v;
    }
    int Add(ARG_TYPE v) { int i = m_nSize; SetAtGrow(i, v); return i; }
    int Append(const CArray &src)
    {
        int nOld = m_nSize;
        SetSize(m_nSize + src.m_nSize);
        CopyElements<TYPE>(m_pData + nOld, src.m_pData, src.m_nSize);
        return nOld;
    }
    void Copy(const CArray &src)
    {
        if (this == &src) return;
        SetSize(src.m_nSize);
        CopyElements<TYPE>(m_pData, src.m_pData, src.m_nSize);
    }
    TYPE operator[](int i) const { return m_pData[i]; }
    TYPE &operator[](int i) { return m_pData[i]; }
    void InsertAt(int nIndex, ARG_TYPE v, int nCount = 1)
    {
        if (nIndex >= m_nSize) {
            SetSize(nIndex + nCount, -1);
        } else {
            int nOld = m_nSize;
            SetSize(m_nSize + nCount, -1);
            DestructElements<TYPE>(&m_pData[nOld], nCount);
            memmove((void *)&m_pData[nIndex + nCount], (void *)&m_pData[nIndex], (nOld - nIndex) * sizeof(TYPE));
            ConstructElements<TYPE>(&m_pData[nIndex], nCount);
        }
        while (nCount--) m_pData[nIndex++] = v;
    }
    void RemoveAt(int nIndex, int nCount = 1)
    {
        int nMove = m_nSize - (nIndex + nCount);
        DestructElements<TYPE>(&m_pData[nIndex], nCount);
        if (nMove) memmove((void *)&m_pData[nIndex], (void *)&m_pData[nIndex + nCount], nMove * sizeof(TYPE));
        m_nSize -= nCount;
    }
    void InsertAt(int nStart, CArray *p)
    {
        if (p->GetSize() > 0) {
            InsertAt(nStart, p->GetAt(0), p->GetSize());
            for (int i = 0; i < p->GetSize(); i++) SetAt(nStart + i, p->GetAt(i));
        }
    }

protected:
    TYPE *m_pData;
    int m_nSize;
    int m_nMaxSize;
    int m_nGrowBy;
};

/* ------------------------------------------------------------------ */

template <class TYPE, class ARG_TYPE = const TYPE &>
class CList : public CObject
{
protected:
    struct CNode
    {
        CNode *pNext;
        CNode *pPrev;
        TYPE data;
    };
public:
    CList(int nBlockSize = 10) : m_pNodeHead(NULL), m_pNodeTail(NULL), m_nCount(0) { (void)nBlockSize; }
    ~CList() { RemoveAll(); }

    int GetCount() const { return m_nCount; }
    int GetSize() const { return m_nCount; }
    BOOL IsEmpty() const { return m_nCount == 0; }
    TYPE &GetHead() { return m_pNodeHead->data; }
    TYPE GetHead() const { return m_pNodeHead->data; }
    TYPE &GetTail() { return m_pNodeTail->data; }
    TYPE GetTail() const { return m_pNodeTail->data; }
    TYPE RemoveHead()
    {
        CNode *n = m_pNodeHead;
        TYPE r = n->data;
        m_pNodeHead = n->pNext;
        if (m_pNodeHead) m_pNodeHead->pPrev = NULL; else m_pNodeTail = NULL;
        FreeNode(n);
        return r;
    }
    TYPE RemoveTail()
    {
        CNode *n = m_pNodeTail;
        TYPE r = n->data;
        m_pNodeTail = n->pPrev;
        if (m_pNodeTail) m_pNodeTail->pNext = NULL; else m_pNodeHead = NULL;
        FreeNode(n);
        return r;
    }
    POSITION AddHead(ARG_TYPE v)
    {
        CNode *n = NewNode(NULL, m_pNodeHead);
        n->data = v;
        if (m_pNodeHead) m_pNodeHead->pPrev = n; else m_pNodeTail = n;
        m_pNodeHead = n;
        return (POSITION)n;
    }
    POSITION AddTail(ARG_TYPE v)
    {
        CNode *n = NewNode(m_pNodeTail, NULL);
        n->data = v;
        if (m_pNodeTail) m_pNodeTail->pNext = n; else m_pNodeHead = n;
        m_pNodeTail = n;
        return (POSITION)n;
    }
    void AddHead(CList *p) { for (POSITION pos = p->GetTailPosition(); pos;) AddHead(p->GetPrev(pos)); }
    void AddTail(CList *p) { for (POSITION pos = p->GetHeadPosition(); pos;) AddTail(p->GetNext(pos)); }
    void RemoveAll()
    {
        for (CNode *n = m_pNodeHead; n;) { CNode *next = n->pNext; FreeNode(n); n = next; }
        m_pNodeHead = m_pNodeTail = NULL;
        m_nCount = 0;
    }
    POSITION GetHeadPosition() const { return (POSITION)m_pNodeHead; }
    POSITION GetTailPosition() const { return (POSITION)m_pNodeTail; }
    TYPE &GetNext(POSITION &pos) { CNode *n = (CNode *)pos; pos = (POSITION)n->pNext; return n->data; }
    TYPE GetNext(POSITION &pos) const { CNode *n = (CNode *)pos; pos = (POSITION)n->pNext; return n->data; }
    TYPE &GetPrev(POSITION &pos) { CNode *n = (CNode *)pos; pos = (POSITION)n->pPrev; return n->data; }
    TYPE GetPrev(POSITION &pos) const { CNode *n = (CNode *)pos; pos = (POSITION)n->pPrev; return n->data; }
    TYPE &GetAt(POSITION pos) { return ((CNode *)pos)->data; }
    TYPE GetAt(POSITION pos) const { return ((CNode *)pos)->data; }
    void SetAt(POSITION pos, ARG_TYPE v) { ((CNode *)pos)->data = v; }
    void RemoveAt(POSITION pos)
    {
        CNode *n = (CNode *)pos;
        if (n == m_pNodeHead) m_pNodeHead = n->pNext; else n->pPrev->pNext = n->pNext;
        if (n == m_pNodeTail) m_pNodeTail = n->pPrev; else n->pNext->pPrev = n->pPrev;
        FreeNode(n);
    }
    POSITION InsertBefore(POSITION pos, ARG_TYPE v)
    {
        if (!pos) return AddHead(v);
        CNode *old = (CNode *)pos;
        CNode *n = NewNode(old->pPrev, old);
        n->data = v;
        if (old->pPrev) old->pPrev->pNext = n; else m_pNodeHead = n;
        old->pPrev = n;
        return (POSITION)n;
    }
    POSITION InsertAfter(POSITION pos, ARG_TYPE v)
    {
        if (!pos) return AddTail(v);
        CNode *old = (CNode *)pos;
        CNode *n = NewNode(old, old->pNext);
        n->data = v;
        if (old->pNext) old->pNext->pPrev = n; else m_pNodeTail = n;
        old->pNext = n;
        return (POSITION)n;
    }
    POSITION Find(ARG_TYPE v, POSITION startAfter = NULL) const
    {
        CNode *n = startAfter ? ((CNode *)startAfter)->pNext : m_pNodeHead;
        for (; n; n = n->pNext)
            if (CompareElements<TYPE>(&n->data, &v)) return (POSITION)n;
        return NULL;
    }
    POSITION FindIndex(int nIndex) const
    {
        if (nIndex < 0 || nIndex >= m_nCount) return NULL;
        CNode *n = m_pNodeHead;
        while (nIndex--) n = n->pNext;
        return (POSITION)n;
    }

protected:
    CNode *m_pNodeHead;
    CNode *m_pNodeTail;
    int m_nCount;

    CNode *NewNode(CNode *prev, CNode *next)
    {
        CNode *n = (CNode *)new BYTE[sizeof(CNode)];
        n->pPrev = prev;
        n->pNext = next;
        ConstructElements<TYPE>(&n->data, 1);
        m_nCount++;
        return n;
    }
    void FreeNode(CNode *n)
    {
        DestructElements<TYPE>(&n->data, 1);
        delete[] (BYTE *)n;
        m_nCount--;
    }
};

/* ------------------------------------------------------------------ */

template <class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap : public CObject
{
protected:
    struct CAssoc
    {
        CAssoc *pNext;
        UINT nHashValue;
        KEY key;
        VALUE value;
    };
public:
    CMap(int nBlockSize = 10) : m_pHashTable(NULL), m_nHashTableSize(17), m_nCount(0) { (void)nBlockSize; }
    ~CMap() { RemoveAll(); }

    int GetCount() const { return m_nCount; }
    int GetSize() const { return m_nCount; }
    BOOL IsEmpty() const { return m_nCount == 0; }
    UINT GetHashTableSize() const { return m_nHashTableSize; }
    void InitHashTable(UINT nHashSize, BOOL bAllocNow = TRUE)
    {
        delete[] m_pHashTable;
        m_pHashTable = NULL;
        if (bAllocNow) {
            m_pHashTable = new CAssoc *[nHashSize];
            memset(m_pHashTable, 0, sizeof(CAssoc *) * nHashSize);
        }
        m_nHashTableSize = nHashSize;
    }
    BOOL Lookup(ARG_KEY key, VALUE &rValue) const
    {
        UINT h;
        CAssoc *a = GetAssocAt(key, h);
        if (!a) return FALSE;
        rValue = a->value;
        return TRUE;
    }
    VALUE &operator[](ARG_KEY key)
    {
        UINT h;
        CAssoc *a = GetAssocAt(key, h);
        if (!a) {
            if (!m_pHashTable) InitHashTable(m_nHashTableSize);
            a = (CAssoc *)new BYTE[sizeof(CAssoc)];
            ConstructElements<KEY>(&a->key, 1);
            ConstructElements<VALUE>(&a->value, 1);
            a->nHashValue = h;
            a->key = key;
            a->pNext = m_pHashTable[h];
            m_pHashTable[h] = a;
            m_nCount++;
        }
        return a->value;
    }
    void SetAt(ARG_KEY key, ARG_VALUE v) { (*this)[key] = v; }
    BOOL RemoveKey(ARG_KEY key)
    {
        if (!m_pHashTable) return FALSE;
        CAssoc **pp = &m_pHashTable[HashKey<ARG_KEY>(key) % m_nHashTableSize];
        for (CAssoc *a = *pp; a; pp = &a->pNext, a = a->pNext)
            if (CompareElements<KEY, ARG_KEY>(&a->key, &key)) {
                *pp = a->pNext;
                FreeAssoc(a);
                return TRUE;
            }
        return FALSE;
    }
    void RemoveAll()
    {
        if (m_pHashTable) {
            for (UINT i = 0; i < m_nHashTableSize; i++)
                for (CAssoc *a = m_pHashTable[i]; a;) { CAssoc *n = a->pNext; FreeAssoc(a); a = n; }
            delete[] m_pHashTable;
            m_pHashTable = NULL;
        }
        m_nCount = 0;
    }
    POSITION GetStartPosition() const { return m_nCount == 0 ? NULL : BEFORE_START_POSITION; }
    void GetNextAssoc(POSITION &rNext, KEY &rKey, VALUE &rValue) const
    {
        CAssoc *ret = (CAssoc *)rNext;
        if (ret == (CAssoc *)BEFORE_START_POSITION)
            for (UINT b = 0; b < m_nHashTableSize; b++)
                if ((ret = m_pHashTable[b]) != NULL) break;
        CAssoc *next = ret->pNext;
        if (!next)
            for (UINT b = ret->nHashValue + 1; b < m_nHashTableSize; b++)
                if ((next = m_pHashTable[b]) != NULL) break;
        rNext = (POSITION)next;
        rKey = ret->key;
        rValue = ret->value;
    }

protected:
    CAssoc **m_pHashTable;
    UINT m_nHashTableSize;
    int m_nCount;

    CAssoc *GetAssocAt(ARG_KEY key, UINT &h) const
    {
        h = HashKey<ARG_KEY>(key) % m_nHashTableSize;
        if (!m_pHashTable) return NULL;
        for (CAssoc *a = m_pHashTable[h]; a; a = a->pNext)
            if (CompareElements<KEY, ARG_KEY>(&a->key, &key)) return a;
        return NULL;
    }
    void FreeAssoc(CAssoc *a)
    {
        DestructElements<VALUE>(&a->value, 1);
        DestructElements<KEY>(&a->key, 1);
        delete[] (BYTE *)a;
        m_nCount--;
    }
};

#endif
