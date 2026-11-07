#!/bin/sh
# Build GComp.dll and gcomp.exe as 32-bit Windows PE with winegcc (no MSVC,
# no Microsoft MFC). Needs: wine development tools with i386-windows import
# libraries, mfc-compat headers, network access or a cached zlib tarball.
#
#   ./build-winegcc.sh   (MFC_COMPAT overrides the bundled mfc-compat/include)
#
# Output: $BUILD/GComp.dll, $BUILD/gcomp.exe (default BUILD=./build-pe).
set -eu

SRC=$(cd "$(dirname "$0")" && pwd)
BUILD=${BUILD:-$SRC/build-pe}
MFC_COMPAT=${MFC_COMPAT:-$SRC/mfc-compat/include}
WINEGCC=${WINEGCC:-winegcc}
WINEGXX=${WINEGXX:-wineg++}
WRC=${WRC:-wrc}
ZLIB_VER=1.3.1
ZLIB_SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
ZLIB_URL=${ZLIB_URL:-https://zlib.net/fossils/zlib-$ZLIB_VER.tar.gz}

TARGET="-b i686-windows"
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
    $WINEGCC $TARGET -O2 -c "$Z/$f.c" -o "$BUILD/obj/zlib/$f.o"
    ZOBJ="$ZOBJ $BUILD/obj/zlib/$f.o"
done

CXXFLAGS="$TARGET -O2 ${EXTRA_CXXFLAGS:-} -I$MFC_COMPAT -I$Z -I$SRC -I$SRC/DLL -DWIN32 -DNDEBUG -D_MBCS -Wno-non-pod-varargs"

# GComp.dll: sources as listed in DLL/GCompDLL.dsp
DLLSRC=$(tr -d '\r' < "$SRC/DLL/GCompDLL.dsp" | sed -n 's/^SOURCE=\(.*\.[cC][pP][pP]\)$/\1/p' |
         sed 's|\\|/|g; s|^\./|DLL/|; s|^\.\./||')
DOBJ=
for f in $DLLSRC; do
    o="$BUILD/obj/dll/$(basename "${f%.*}").o"
    $WINEGXX $CXXFLAGS -D_WINDOWS -D_USRDLL -DGCOMPDLL_EXPORTS -D_WINDLL -c "$SRC/$f" -o "$o"
    DOBJ="$DOBJ $o"
done
$WRC --nostdinc -I"$MFC_COMPAT" -I"$SRC/DLL" -I/usr/include/wine/windows \
    -l0x419 -DNDEBUG -o "$BUILD/obj/dll/GComp.res" "$SRC/DLL/GComp.rc"
$WINEGXX $TARGET -shared -o "$BUILD/GComp.dll" $DOBJ $ZOBJ "$BUILD/obj/dll/GComp.res" \
    -Wl,--out-implib,"$BUILD/libGComp.a" -lole32 -luuid -lversion

# gcomp.exe: sources as listed in gcomp.dsp
EXESRC=$(tr -d '\r' < "$SRC/gcomp.dsp" | sed -n 's/^SOURCE=\.\\\(.*\.[cC][pP][pP]\)$/\1/p')
EOBJ=
for f in $EXESRC; do
    o="$BUILD/obj/exe/$(basename "${f%.*}").o"
    $WINEGXX $CXXFLAGS -D_CONSOLE -c "$SRC/$f" -o "$o"
    EOBJ="$EOBJ $o"
done
$WINEGXX $TARGET -mconsole -o "$BUILD/gcomp.exe" $EOBJ -L"$BUILD" -lGComp -lole32

echo "built: $BUILD/GComp.dll $BUILD/gcomp.exe"
