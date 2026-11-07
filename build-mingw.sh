#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Build GComp.dll and gcomp.exe as 32-bit Windows PE with MinGW-w64 GCC (no MSVC,
# no Microsoft MFC). Needs: i686-w64-mingw32 g++/gcc/windres (Fedora:
# mingw32-gcc-c++), network access or a cached zlib tarball. Linked with
# -static so that libgcc, libstdc++ and libwinpthread need not be shipped.
#
#   ./build-mingw.sh   (MFC_COMPAT overrides the bundled mfc-compat/include)
#
# Output: $BUILD/GComp.dll, $BUILD/gcomp.exe (default BUILD=./build-mingw).
set -eu

SRC=$(cd "$(dirname "$0")" && pwd)
BUILD=${BUILD:-$SRC/build-mingw}
MFC_COMPAT=${MFC_COMPAT:-$SRC/mfc-compat/include}
CC=${MINGW_CC:-i686-w64-mingw32-gcc}
CXX=${MINGW_CXX:-i686-w64-mingw32-g++}
WINDRES=${MINGW_WINDRES:-i686-w64-mingw32-windres}
ZLIB_VER=1.3.1
ZLIB_SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
ZLIB_URL=${ZLIB_URL:-https://zlib.net/fossils/zlib-$ZLIB_VER.tar.gz}

mkdir -p "$BUILD/obj/zlib" "$BUILD/obj/dll" "$BUILD/obj/exe"

# zlib, statically linked into GComp.dll
Z="$BUILD/zlib-$ZLIB_VER"
if [ ! -f "$Z/zlib.h" ]; then
    T="$BUILD/zlib-$ZLIB_VER.tar.gz"
    [ -f "$T" ] || { curl -fsSL -o "$T" "$ZLIB_URL" || wget -q -O "$T" "$ZLIB_URL"; }
    echo "$ZLIB_SHA256  $T" | sha256sum -c -
    tar -xzf "$T" -C "$BUILD"
fi
# deflate of zlib 1.0.4, the version in 1C 7.7 zlibeng.dll: newer zlib
# compresses larger module texts differently (see zlib104-rename.h)
Z104_SHA256=e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24
Z104="$BUILD/zlib-1.0.4"
if [ ! -f "$Z104/deflate.c" ]; then
    T="$BUILD/zlib-1.0.4.tar.gz"
    U=https://zlib.net/fossils/zlib-1.0.4.tar.gz
    [ -f "$T" ] || { curl -fsSL -o "$T" "$U" || wget -q -O "$T" "$U"; }
    echo "$Z104_SHA256  $T" | sha256sum -c -
    tar -xzf "$T" -C "$BUILD"
fi
mkdir -p "$BUILD/obj/zlib104"
ZOBJ=
for f in adler32 deflate trees zutil; do
    $CC -O2 -w -include "$SRC/zlib104-rename.h" -I"$Z104" -c "$Z104/$f.c" -o "$BUILD/obj/zlib104/$f.o"
    ZOBJ="$ZOBJ $BUILD/obj/zlib104/$f.o"
done
for f in adler32 compress crc32 deflate infback inffast inflate inftrees trees uncompr zutil; do
    $CC -O2 -c "$Z/$f.c" -o "$BUILD/obj/zlib/$f.o"
    ZOBJ="$ZOBJ $BUILD/obj/zlib/$f.o"
done

CXXFLAGS="-O2 ${EXTRA_CXXFLAGS:-} -I$MFC_COMPAT -I$Z -I$SRC -I$SRC/DLL -DWIN32 -DNDEBUG -D_MBCS -Werror=conditionally-supported"

# GComp.dll: sources as listed in DLL/GCompDLL.dsp
DLLSRC=$(tr -d '\r' < "$SRC/DLL/GCompDLL.dsp" | sed -n 's/^SOURCE=\(.*\.[cC][pP][pP]\)$/\1/p' |
         sed 's|\\|/|g; s|^\./|DLL/|; s|^\.\./||')
DOBJ=
for f in $DLLSRC; do
    o="$BUILD/obj/dll/$(basename "${f%.*}").o"
    $CXX $CXXFLAGS -D_WINDOWS -D_USRDLL -DGCOMPDLL_EXPORTS -D_WINDLL -c "$SRC/$f" -o "$o"
    DOBJ="$DOBJ $o"
done
$WINDRES -I"$SRC/DLL" -l0x419 -DNDEBUG -c 1251 -O coff -o "$BUILD/obj/dll/GComp.res" "$SRC/DLL/GComp.rc"
$CXX -shared -o "$BUILD/GComp.dll" $DOBJ $ZOBJ "$BUILD/obj/dll/GComp.res" \
    -Wl,--out-implib,"$BUILD/libGComp.a" -static -lole32 -luuid -lversion

# gcomp.exe: sources as listed in gcomp.dsp
EXESRC=$(tr -d '\r' < "$SRC/gcomp.dsp" | sed -n 's/^SOURCE=\.\\\(.*\.[cC][pP][pP]\)$/\1/p')
EOBJ=
for f in $EXESRC; do
    o="$BUILD/obj/exe/$(basename "${f%.*}").o"
    $CXX $CXXFLAGS -D_CONSOLE -c "$SRC/$f" -o "$o"
    EOBJ="$EOBJ $o"
done
$CXX -mconsole -o "$BUILD/gcomp.exe" $EOBJ -L"$BUILD" -lGComp -lole32 -static

echo "built: $BUILD/GComp.dll $BUILD/gcomp.exe"
