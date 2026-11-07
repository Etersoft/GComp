#include <afx.h>
#include <afxtempl.h>

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main()
{
    CString s("Hello"), t;
    CHECK(sizeof(CString) == sizeof(char *));
    t.Format("%s, %d", (LPCTSTR)s, 42);
    CHECK(t == "Hello, 42");
    char buf[32];
    sprintf(buf, "%s", s);   /* MFC programs pass CString through varargs */
    CHECK(!strcmp(buf, "Hello"));
    CHECK(t.Find("42") == 7 && t.Find(',') == 5 && t.ReverseFind('l') == 3);
    CHECK(t.Mid(7) == "42" && t.Left(2) == "He" && t.Right(2) == "42");
    CString w("  x y  "); w.TrimLeft(); w.TrimRight(); CHECK(w == "x y");
    CHECK(t.SpanIncluding("Hel") == "Hell" && t.SpanExcluding(",") == "Hello");
    CString r("a.b.c"); CHECK(r.Replace(".", "::") == 2 && r == "a::b::c");
    r.Delete(1, 2); CHECK(r == "ab::c"); r.Insert(0, '['); CHECK(r == "[ab::c");
    CString g; char *p = g.GetBufferSetLength(3); memcpy(p, "xyz", 3); CHECK(g == "xyz" && g.GetLength() == 3);
    { CString a("abc"), b(CString('x', 200)), c(CString('y', 600)); CHECK(((CStringData *)(LPCTSTR)a)[-1].nAllocLength == 64 && ((CStringData *)(LPCTSTR)b)[-1].nAllocLength == 256 && ((CStringData *)(LPCTSTR)c)[-1].nAllocLength == 600); }
    CHECK(s + "!" == "Hello!" && "<" + s == "<Hello" && s + '?' == "Hello?");

    CArray<int, int> a;
    for (int i = 0; i < 100; i++) a.Add(i);
    a.InsertAt(0, -1); a.RemoveAt(50);
    CHECK(a.GetSize() == 100 && a[0] == -1 && a[50] == 50);

    CStringArray sa; sa.Add("one"); sa.Add(CString("two")); sa.InsertAt(1, "mid");
    CHECK(sa.GetSize() == 3 && sa[1] == "mid" && sa[2] == "two");

    CPtrList l; l.AddTail((void *)1); l.AddTail((void *)2); l.AddHead((void *)0);
    POSITION pos = l.GetHeadPosition(); int n = 0;
    while (pos) CHECK(l.GetNext(pos) == (void *)(INT_PTR)n++);
    CHECK(n == 3 && l.Find((void *)2) != NULL);

    CMapStringToPtr m;
    const char *keys[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    for (int i = 0; i < 5; i++) m.SetAt(keys[i], (void *)(INT_PTR)i);
    void *v; CHECK(m.Lookup("gamma", v) && v == (void *)2 && !m.Lookup("zeta", v));
    pos = m.GetStartPosition(); CString k; n = 0;
    while (pos) { m.GetNextAssoc(pos, k, v); printf("assoc %s %d\n", (LPCTSTR)k, (int)(INT_PTR)v); n++; }
    CHECK(n == 5 && m.RemoveKey("beta") && m.GetCount() == 4);

    CTimeSpan sp(0, 1, 2, 3); CHECK(sp.Format("%H:%M:%S") == "01:02:03");
    CTime now = CTime::GetCurrentTime(); CHECK(now.GetYear() >= 2024);
    printf("%s\n", fails ? "FAILED" : "OK");
    return fails != 0;
}
