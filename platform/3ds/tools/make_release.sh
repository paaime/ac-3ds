#!/bin/sh
# Usage: make_release.sh [--source-url URL] [--allow-dirty] (platform/3ds/README.md, "Release build").
set -eu

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
P3DS=$ROOT/platform/3ds
RES=$P3DS/res
DKP=${DEVKITPRO:-/opt/devkitpro}
BUILD=$ROOT/build/3ds-release
OUT=$ROOT/build/release
SOURCE_URL=""
ALLOW_DIRTY=0

while [ $# -gt 0 ]; do
    case $1 in
        --source-url) SOURCE_URL=$2; shift 2 ;;
        --allow-dirty) ALLOW_DIRTY=1; shift ;;
        *) echo "usage: $0 [--source-url URL] [--allow-dirty]" >&2; exit 2 ;;
    esac
done

fail() { echo "make_release: $*" >&2; exit 1; }

find_tool() { # name, variable's value
    if [ -n "$2" ]; then echo "$2"
    elif command -v "$1" >/dev/null 2>&1; then command -v "$1"
    elif [ -x "$ROOT/ref/bin/ctrtools/$1" ]; then echo "$ROOT/ref/bin/ctrtools/$1"
    else fail "$1 not found: set ${1}'s path in $(echo "$1" | tr a-z A-Z) or put it on PATH"
    fi
}
MAKEROM=$(find_tool makerom "${MAKEROM:-}")
if [ -f "$RES/banner.png" ]; then BANNERTOOL=$(find_tool bannertool "${BANNERTOOL:-}"); else BANNERTOOL=""; fi

VERSION=$(sed -n 's/^project(ac3ds VERSION \([0-9]*\.[0-9]*\.[0-9]*\) .*/\1/p' "$P3DS/CMakeLists.txt")
[ -n "$VERSION" ] || fail "no version in platform/3ds/CMakeLists.txt's project()"
MAJOR=${VERSION%%.*}; REST=${VERSION#*.}; MINOR=${REST%%.*}; MICRO=${REST#*.}

COMMIT=$(git -C "$ROOT" rev-parse --short HEAD)
DIRTY=$(git -C "$ROOT" status --porcelain -- platform/3ds src include pc)
NAME=ac3ds-$VERSION
if [ -n "$DIRTY" ]; then
    [ "$ALLOW_DIRTY" = 1 ] || fail "uncommitted changes in the sources (--allow-dirty for a test package):
$DIRTY"
    NAME=$NAME-dirty
fi
[ -n "$SOURCE_URL" ] || echo "make_release: no --source-url: README.txt says the address is still to come" >&2
URL_TEXT=${SOURCE_URL:-"(to be published)"}
[ -z "$SOURCE_URL" ] || URL_TEXT="$SOURCE_URL (commit $COMMIT)"

echo "== Animal Crossing 3DS $VERSION, commit $COMMIT"
cmake -S "$P3DS" -B "$BUILD" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$DKP/cmake/3DS.cmake" -DAC3DS_DEBUG=OFF >/dev/null
cmake --build "$BUILD"

STAGE=$OUT/$NAME
rm -rf "$STAGE" "$OUT/$NAME.zip"
mkdir -p "$STAGE/LICENSES"
cp "$BUILD/ac3ds.3dsx" "$STAGE/"
set -- -f cia -o "$STAGE/ac3ds.cia" -rsf "$RES/ac3ds.rsf" -target t -elf "$BUILD/ac3ds.elf" \
    -icon "$BUILD/ac3ds.smdh" -major "$MAJOR" -minor "$MINOR" -micro "$MICRO"
if [ -n "$BANNERTOOL" ]; then
    # bannertool needs a sound for the banner: a tenth of a second of silence.
    python3 -c 'import sys, wave; w = wave.open(sys.argv[1], "wb"); w.setnchannels(1); w.setsampwidth(2); \
w.setframerate(32000); w.writeframes(bytes(6400)); w.close()' "$BUILD/silence.wav"
    "$BANNERTOOL" makebanner -i "$RES/banner.png" -a "$BUILD/silence.wav" -o "$BUILD/banner.bnr" >/dev/null
    set -- "$@" -banner "$BUILD/banner.bnr"
else
    echo "make_release: no res/banner.png: the .cia has no banner (the HOME Menu shows the icon only)" >&2
fi
"$MAKEROM" "$@"
sed -e "s|@VERSION@|$VERSION|g" -e "s|@SOURCE_URL@|$URL_TEXT|g" "$RES/README.txt" > "$STAGE/README.txt"
cp "$ROOT/LICENSE" "$STAGE/LICENSES/ACGC-PC-Port.txt"
cp "$P3DS/LICENSE" "$STAGE/LICENSES/Animal-Crossing-3DS.txt"
cp "$P3DS/lib/quicknes/LICENSE" "$STAGE/LICENSES/QuickNES-LGPL-2.1.txt"
cp "$RES/THIRD-PARTY.txt" "$STAGE/LICENSES/THIRD-PARTY.txt"

# Exactly these files, so nothing else (a disc image, a save, a log) can slip in.
EXPECTED="ac3ds.3dsx ac3ds.cia README.txt LICENSES/ACGC-PC-Port.txt LICENSES/Animal-Crossing-3DS.txt
LICENSES/QuickNES-LGPL-2.1.txt LICENSES/THIRD-PARTY.txt"
FOUND=$(cd "$STAGE" && find . -type f | sed 's|^\./||' | sort | tr '\n' ' ')
WANTED=$(echo $EXPECTED | tr ' ' '\n' | sort | tr '\n' ' ')
[ "$FOUND" = "$WANTED" ] || fail "unexpected files in the package: $FOUND"

(cd "$OUT" && zip -q -r -X "$NAME.zip" "$NAME")
echo "== $OUT/$NAME.zip"
(cd "$STAGE" && shasum -a 256 ac3ds.3dsx ac3ds.cia) && shasum -a 256 "$OUT/$NAME.zip" | sed "s|$OUT/||"

QR=$OUT/$NAME-fbi-qr.png
QR_URL=""
RELEASE_URL=${SOURCE_URL%/}/releases/download/v$VERSION
rm -f "$QR"
if [ -z "$SOURCE_URL" ] || [ -n "$DIRTY" ]; then
    echo "make_release: no FBI QR code without --source-url, or for a test package" >&2
elif QRENCODE=$(command -v "${QRENCODE:-qrencode}"); then
    "$QRENCODE" -o "$QR" -l M -s 8 -m 2 "$RELEASE_URL/ac3ds.cia"
    QR_URL=$RELEASE_URL/$NAME-fbi-qr.png
    echo "== $QR: $RELEASE_URL/ac3ds.cia"
else
    echo "make_release: qrencode not found (QRENCODE or PATH): no FBI QR code" >&2
fi

NOTES=$OUT/$NAME-notes.md
if [ -n "$QR_URL" ]; then
    sed -e '/^@FBI@$/d' -e '/^@END_FBI@$/d' -e "s|@QR_URL@|$QR_URL|" -e "s|@VERSION@|$VERSION|g" \
        "$RES/release-notes.md" > "$NOTES"
else
    sed -e '/^@FBI@$/,/^@END_FBI@$/d' -e "s|@VERSION@|$VERSION|g" "$RES/release-notes.md" > "$NOTES"
fi
echo "== $NOTES: write what's new and the known issues"
if [ -n "$SOURCE_URL" ] && [ -z "$DIRTY" ]; then
    REPO=${SOURCE_URL%/}
    FILES="\"$OUT/$NAME.zip\" \"$STAGE/ac3ds.cia\""
    [ -z "$QR_URL" ] || FILES="$FILES \"$QR\""
    echo "== then, with v$VERSION tagged and pushed:"
    echo "gh release create v$VERSION --repo ${REPO#https://github.com/} --verify-tag \\"
    echo "  --title \"Animal Crossing 3DS $VERSION\" --notes-file \"$NOTES\" $FILES"
fi
