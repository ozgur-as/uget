#!/bin/bash
#
# Assemble the Windows distribution: a portable zip and an NSIS installer.
#
# Expects an already-configured cross build directory (see BUILDING.md) and
# the MinGW sysroot the build linked against. Everything runs on Linux --
# makensis is packaged for it, so no Windows machine is involved.
#
#   packaging/windows/make-dist.sh --build-dir /build --output-dir dist
#
# Produces, in the output directory:
#   uget-<version>-win64-portable.zip   self-contained, keeps config alongside
#   uget-<version>-win64-setup.exe      installs to Program Files
#
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR=""
OUTPUT_DIR="$SOURCE_DIR/dist"
SYSROOT="/usr/x86_64-w64-mingw32/sys-root/mingw"
OBJDUMP="x86_64-w64-mingw32-objdump"

usage() {
	sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \?//'
	exit "${1:-0}"
}

while [ $# -gt 0 ]; do
	case "$1" in
	--build-dir)  BUILD_DIR="$2"; shift 2 ;;
	--output-dir) OUTPUT_DIR="$2"; shift 2 ;;
	--sysroot)    SYSROOT="$2"; shift 2 ;;
	-h|--help)    usage 0 ;;
	*) echo "unknown argument: $1" >&2; usage 1 ;;
	esac
done

[ -n "$BUILD_DIR" ] || { echo "error: --build-dir is required" >&2; usage 1; }

EXE="$BUILD_DIR/ui-gtk/uget-gtk.exe"
[ -f "$EXE" ] || { echo "error: $EXE not found -- build the cross target first" >&2; exit 1; }
[ -d "$SYSROOT" ] || { echo "error: sysroot $SYSROOT not found" >&2; exit 1; }

VERSION="$(sed -n "s/^  version: '\([^']*\)',.*/\1/p" "$SOURCE_DIR/meson.build")"
[ -n "$VERSION" ] || { echo "error: could not read version from meson.build" >&2; exit 1; }

STAGE="$OUTPUT_DIR/stage"
PORTABLE_NAME="uget-$VERSION-win64"
PORTABLE="$OUTPUT_DIR/$PORTABLE_NAME"

echo ":: uGet $VERSION -- assembling Windows distribution"
rm -rf "$STAGE" "$PORTABLE"
mkdir -p "$STAGE/bin" "$OUTPUT_DIR"

cp "$EXE" "$STAGE/bin/"

# --- DLL closure ------------------------------------------------------------
# Walk the import table; anything present in the sysroot ships with us, and
# anything absent is a Windows system DLL that must not be redistributed.
resolve_dlls() {
	local file="$1" dep
	for dep in $($OBJDUMP -p "$file" 2>/dev/null | awk '/DLL Name:/{print $3}'); do
		if [ -f "$SYSROOT/bin/$dep" ] && [ ! -f "$STAGE/bin/$dep" ]; then
			cp "$SYSROOT/bin/$dep" "$STAGE/bin/"
			resolve_dlls "$SYSROOT/bin/$dep"
		fi
	done
}
resolve_dlls "$STAGE/bin/uget-gtk.exe"
echo ":: bundled $(find "$STAGE/bin" -name '*.dll' | wc -l) DLLs"

# --- GTK runtime data -------------------------------------------------------
# GSettings schemas are not optional: GTK aborts at startup without them.
mkdir -p "$STAGE/share/glib-2.0/schemas"
cp -r "$SYSROOT/share/glib-2.0/schemas/." "$STAGE/share/glib-2.0/schemas/"
glib-compile-schemas "$STAGE/share/glib-2.0/schemas" >/dev/null

mkdir -p "$STAGE/share/icons"
for theme in Adwaita hicolor; do
	[ -d "$SYSROOT/share/icons/$theme" ] && cp -r "$SYSROOT/share/icons/$theme" "$STAGE/share/icons/"
done

# gdk-pixbuf loaders, if this GTK build uses loadable ones
[ -d "$SYSROOT/lib/gdk-pixbuf-2.0" ] && {
	mkdir -p "$STAGE/lib"
	cp -r "$SYSROOT/lib/gdk-pixbuf-2.0" "$STAGE/lib/"
}

# --- uGet's own data --------------------------------------------------------
cp -r "$SOURCE_DIR/pixmaps/icons/hicolor" "$STAGE/share/icons/" 2>/dev/null || true
mkdir -p "$STAGE/share/uget"
cp "$SOURCE_DIR/sounds/notification.wav" "$STAGE/share/uget/"

# Compiled translations, mirroring the Unix install layout.
if [ -d "$BUILD_DIR/po" ]; then
	while IFS= read -r mo; do
		lang="$(basename "$(dirname "$(dirname "$mo")")")"
		mkdir -p "$STAGE/share/locale/$lang/LC_MESSAGES"
		cp "$mo" "$STAGE/share/locale/$lang/LC_MESSAGES/uget.mo"
	done < <(find "$BUILD_DIR/po" -name '*.mo')
	echo ":: bundled $(find "$STAGE/share/locale" -name '*.mo' | wc -l) translations"
fi

echo ":: staging tree $(du -sh "$STAGE" | cut -f1)"

# --- portable zip -----------------------------------------------------------
# The marker makes uGet keep its config next to the executable
# (see ugtk_is_portable_mode() in ui-gtk/UgtkConfig.c).
cp -r "$STAGE" "$PORTABLE"
touch "$PORTABLE/bin/uget-portable-mode"
mkdir -p "$PORTABLE/config"

ZIP="$OUTPUT_DIR/$PORTABLE_NAME-portable.zip"
rm -f "$ZIP"
python3 - "$OUTPUT_DIR" "$PORTABLE_NAME" "$ZIP" <<'PY'
import os, sys, zipfile
out_dir, base, zip_path = sys.argv[1], sys.argv[2], sys.argv[3]
root = os.path.join(out_dir, base)
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            full = os.path.join(dirpath, name)
            zf.write(full, os.path.join(base, os.path.relpath(full, root)))
        if not filenames and not _dirnames:      # keep empty config/
            rel = os.path.relpath(dirpath, root)
            zf.writestr(os.path.join(base, rel) + "/", "")
PY
rm -rf "$PORTABLE"
echo ":: portable zip  $ZIP ($(du -h "$ZIP" | cut -f1))"

# --- installer --------------------------------------------------------------
# makensis loads a default x86 stub while starting up -- before the script's
# "Target amd64-unicode" can apply -- and refuses to run if it is absent.
# Fedora's mingw64-nsis ships amd64 stubs only. Where that is the case, hand
# makensis a private NSISDIR in which the amd64 stubs also answer to the x86
# names; Target still selects the real amd64 stub for the installer itself,
# which stays a genuine 64-bit binary.
prepare_nsisdir() {
	local sysdir="${NSISDIR:-/usr/share/nsis}" priv c v
	[ -d "$sysdir/Stubs" ] || return 0
	[ -f "$sysdir/Stubs/zlib-x86-unicode" ] && return 0   # complete install

	priv="$OUTPUT_DIR/.nsisdir"
	rm -rf "$priv"
	cp -r "$sysdir" "$priv"
	for c in zlib bzip2 lzma; do
		for v in "" _solid; do
			[ -f "$priv/Stubs/${c}${v}-amd64-unicode" ] && \
				cp "$priv/Stubs/${c}${v}-amd64-unicode" "$priv/Stubs/${c}${v}-x86-unicode"
		done
	done
	export NSISDIR="$priv"
	echo ":: system NSIS has no x86 stubs; using private NSISDIR"
}

SETUP="$OUTPUT_DIR/$PORTABLE_NAME-setup.exe"
if command -v makensis >/dev/null 2>&1; then
	rm -f "$SETUP"
	prepare_nsisdir
	makensis -V2 \
		-DVERSION="$VERSION" \
		-DSTAGEDIR="$STAGE" \
		-DOUTFILE="$SETUP" \
		-DLICENSE="$SOURCE_DIR/COPYING" \
		-DICON="$SOURCE_DIR/packaging/windows/uget-icon.ico" \
		"$SOURCE_DIR/packaging/windows/uget.nsi"
	rm -rf "$OUTPUT_DIR/.nsisdir"
	echo ":: installer     $SETUP ($(du -h "$SETUP" | cut -f1))"
else
	echo ":: makensis not found -- skipping installer (Fedora: dnf install mingw64-nsis)"
fi

echo ":: done"
