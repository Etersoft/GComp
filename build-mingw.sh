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

# The .cpp files a Visual C++ 6 project lists, as paths from here
dsp_sources() {
    tr -d '\r' < "$1" | sed -n 's/^SOURCE=\(.*\.[cC][pP][pP]\)$/\1/p' |
        sed "s|\\\\|/|g; s|^|$(dirname "$1")/|"
}

mkdir -p "$BUILD/obj/zlib" "$BUILD/obj/dll" "$BUILD/obj/exe"

# zlib, statically linked into GComp.dll
Z="$BUILD/zlib-$ZLIB_VER"
if [ ! -f "$Z/zlib.h" ]; then
    T="$BUILD/zlib-$ZLIB_VER.tar.gz"
    [ -f "$T" ] || { curl -fsSL -o "$T" "$ZLIB_URL" || wget -q -O "$T" "$ZLIB_URL"; }
    echo "$ZLIB_SHA256  $T" | sha256sum -c -
    tar -xzf "$T" -C "$BUILD"
fi
ZOBJ=
for f in adler32 compress crc32 deflate infback inffast inflate inftrees trees uncompr zutil; do
    $CC -O2 -c "$Z/$f.c" -o "$BUILD/obj/zlib/$f.o"
    ZOBJ="$ZOBJ $BUILD/obj/zlib/$f.o"
done

S=$SRC/src
INCS=
for d in "" common storage metadata forms ui DLL; do INCS="$INCS -I$S/$d"; done

CXXFLAGS="-O2 ${EXTRA_CXXFLAGS:-} -I$MFC_COMPAT -I$Z $INCS -DWIN32 -DNDEBUG -D_MBCS -Werror=conditionally-supported"

# GComp.dll: sources as listed in src/DLL/GCompDLL.dsp
DLLSRC=$(dsp_sources "$S/DLL/GCompDLL.dsp")
DOBJ=
for f in $DLLSRC; do
    o="$BUILD/obj/dll/$(basename "${f%.*}").o"
    $CXX $CXXFLAGS -D_WINDOWS -D_USRDLL -DGCOMPDLL_EXPORTS -D_WINDLL -c "$f" -o "$o"
    DOBJ="$DOBJ $o"
done
$WINDRES -I"$S/DLL" -l0x419 -DNDEBUG -c 1251 -O coff -o "$BUILD/obj/dll/GComp.res" "$S/DLL/GComp.rc"
$CXX -shared -o "$BUILD/GComp.dll" $DOBJ $ZOBJ "$BUILD/obj/dll/GComp.res" \
    -Wl,--out-implib,"$BUILD/libGComp.a" -static -lole32 -luuid -lversion

# gcomp.exe: sources as listed in src/cli/gcomp.dsp
EXESRC=$(dsp_sources "$S/cli/gcomp.dsp")
EOBJ=
for f in $EXESRC; do
    o="$BUILD/obj/exe/$(basename "${f%.*}").o"
    $CXX $CXXFLAGS -D_CONSOLE -c "$f" -o "$o"
    EOBJ="$EOBJ $o"
done
$CXX -mconsole -o "$BUILD/gcomp.exe" $EOBJ -L"$BUILD" -lGComp -lole32 -static

echo "built: $BUILD/GComp.dll $BUILD/gcomp.exe"
