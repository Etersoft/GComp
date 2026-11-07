# mfc-compat

Free replacement for the non-GUI core of MFC 4.2 (ANSI/MBCS) for building old
console MFC programs as Windows PE with winegcc or MinGW, without Microsoft
MFC headers or libraries.

Provided headers: `afx.h`, `afxtempl.h`, `afxcoll.h`.

- `CObject`, `CException`, `POSITION`, `ASSERT`/`VERIFY`/`TRACE` (no-ops),
  `DEBUG_NEW`, empty `DECLARE_*`/`IMPLEMENT_*` macros.
- `CString` with the MFC 4.2 memory layout: a single pointer to the characters
  preceded by `CStringData {nRefs, nDataLength, nAllocLength}`. Programs that
  pass `CString` through `...` (`printf("%s", str)`) or read `GetData()` work.
  Copies are not shared (`nRefs` is always 1).
- `CArray`, `CList`, `CMap` templates and `CPtrArray`, `CStringArray`,
  `CObArray`, `CByteArray`, `CWordArray`, `CDWordArray`, `CUIntArray`,
  `CPtrList`, `CObList`, `CStringList`, `CMapStringToPtr`, `CMapStringToOb`,
  `CMapStringToString`, `CMapWordToPtr`, `CMapPtrToPtr`, `CMapPtrToWord`,
  `CMapWordToOb`.
  Kept from MFC 4.2: array growth and bitwise relocation of elements,
  zero-filled construction, hash functions, default hash size 17, insertion
  at the head of a bucket and bucket-order `GetNextAssoc`.
- `CTime`, `CTimeSpan`.

Not provided: windows, dialogs, documents/views, `CFile`, `CArchive`,
serialization, OLE, UNICODE builds.

Written from the documented MFC behaviour; contains no Microsoft code.
There is no complete free MFC implementation (2026-11): Wine does not ship
one, OpenMFC (Kreijstal/openmfc) targets the MSVC ABI of mfc140u.dll and has
no license; ReactOS ATL and Win32++ provide CString-like classes but not the
MFC collections.

## Build notes

clang (winegcc PE target `i686-windows`) uses the MSVC C++ ABI. Passing a
`CString` through `...` is rejected by default; build with
`-Wno-non-pod-varargs`, then the object is copied bitwise like MSVC does.

    make check SDK=/path/to/wine/i386-windows WINE=wine

License: LGPL-2.1-or-later (see COPYING).
