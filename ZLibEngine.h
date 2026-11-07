/*
 * Replacement for the V77.zlibEngine COM object of 1C:Enterprise 7.7
 * (zlibeng.dll), which GComp used through `#import "zlibeng.dll"`.
 *
 * 1C 7.7 stores "MD Programm text", "Inplace description" and pictures as
 * raw deflate (no zlib header, windowBits -15), written by zlib 1.0.4 with
 * level 4 and memLevel 9. Compression uses the deflate of zlib 1.0.4 linked
 * under zlib104_* names (zlib104-rename.h): newer zlib gives other bytes on
 * larger inputs. Decompression uses the current zlib.
 *
 * pkDecompress: inflate `src` from its current position to the end and write
 *               to `dst`; `dst` is rewound to the start afterwards.
 * pkCompress:   deflate `src` from its current position to the end into
 *               `dst` at its current position.
 *
 * This file is part of GComp.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef ZLIBENGINE_H
#define ZLIBENGINE_H

#include <windows.h>
#include <ole2.h>
#include <zlib.h>

/* deflate of zlib 1.0.4; its z_stream has the same layout as the current one */
extern "C" {
int zlib104_deflateInit2_(z_streamp strm, int level, int method, int windowBits,
                          int memLevel, int strategy, const char *version, int stream_size);
int zlib104_deflate(z_streamp strm, int flush);
int zlib104_deflateEnd(z_streamp strm);
}

class CZLibEngine
{
public:
    HRESULT CreateInstance(LPCWSTR) { return S_OK; }
    void Release() {}
    CZLibEngine *operator->() { return this; }

    HRESULT pkDecompress(IStream *src, IStream *dst)
    {
        z_stream z = {};
        if (inflateInit2(&z, -15) != Z_OK) return E_FAIL;
        HRESULT hr = Pump(src, dst, z, false);
        inflateEnd(&z);
        LARGE_INTEGER zero = {};
        dst->Seek(zero, STREAM_SEEK_SET, NULL);
        return hr;
    }

    HRESULT pkCompress(IStream *src, IStream *dst)
    {
        z_stream z = {};
        if (zlib104_deflateInit2_(&z, 4, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY,
                                  "1.0.4", (int)sizeof(z_stream)) != Z_OK) return E_FAIL;
        HRESULT hr = Pump(src, dst, z, true);
        zlib104_deflateEnd(&z);
        return hr;
    }

private:
    static HRESULT Pump(IStream *src, IStream *dst, z_stream &z, bool compress)
    {
        unsigned char in[16384], out[16384];
        int ret = Z_OK;
        for (;;) {
            ULONG got = 0;
            HRESULT hr = src->Read(in, sizeof in, &got);
            if (FAILED(hr)) return hr;
            int flush = (got == 0) ? Z_FINISH : Z_NO_FLUSH;
            z.next_in = in;
            z.avail_in = got;
            do {
                z.next_out = out;
                z.avail_out = sizeof out;
                ret = compress ? zlib104_deflate(&z, flush) : inflate(&z, Z_NO_FLUSH);
                if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR || ret == Z_NEED_DICT)
                    return E_FAIL;
                ULONG n = (ULONG)(sizeof out - z.avail_out);
                if (n) {
                    ULONG written = 0;
                    hr = dst->Write(out, n, &written);
                    if (FAILED(hr) || written != n) return FAILED(hr) ? hr : E_FAIL;
                }
            } while (z.avail_out == 0);
            if (ret == Z_STREAM_END) return S_OK;
            if (got == 0) return compress ? S_OK : E_FAIL;
        }
    }
};

typedef CZLibEngine IzlibEnginePtr;

#endif
