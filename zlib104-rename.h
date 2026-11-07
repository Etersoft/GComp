/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Force-included when compiling the deflate part of zlib 1.0.4 (adler32.c,
 * deflate.c, trees.c, zutil.c): renames its external symbols so that it
 * can be linked next to a current zlib.
 *
 * 1C:Enterprise 7.7 compresses module texts and pictures with zlib 1.0.4
 * (zlibeng.dll). Newer zlib versions produce different raw deflate data
 * for the same parameters on larger inputs, so byte-exact compilation needs
 * the old deflate. Decompression uses the current zlib.
 */
#define _tr_align            zlib104__tr_align
#define _tr_flush_block      zlib104__tr_flush_block
#define _tr_init             zlib104__tr_init
#define _tr_stored_block     zlib104__tr_stored_block
#define _tr_tally            zlib104__tr_tally
#define adler32              zlib104_adler32
#define deflate              zlib104_deflate
#define deflateCopy          zlib104_deflateCopy
#define deflateEnd           zlib104_deflateEnd
#define deflateInit2_        zlib104_deflateInit2_
#define deflateInit_         zlib104_deflateInit_
#define deflateParams        zlib104_deflateParams
#define deflateReset         zlib104_deflateReset
#define deflateSetDictionary zlib104_deflateSetDictionary
#define deflate_copyright    zlib104_deflate_copyright
#define z_errmsg             zlib104_z_errmsg
#define zcalloc              zlib104_zcalloc
#define zcfree               zlib104_zcfree
#define zlibVersion          zlib104_zlibVersion
