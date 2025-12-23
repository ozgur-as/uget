@echo off
REM uGet Windows Distribution Builder
REM Runs via MSYS2 MinGW64 (auto-detected)

set MSYS2_PATH=C:\msys64

if not exist "%MSYS2_PATH%\usr\bin\bash.exe" (
    echo Error: MSYS2 not found at %MSYS2_PATH%
    echo Please install MSYS2 or edit this script to set MSYS2_PATH.
    pause
    exit /b 1
)

echo Running distribution build in MSYS2 MinGW64...
echo.

set MSYSTEM=MINGW64
set CHERE_INVOKING=1
cd /d "%~dp0.."

REM Pass project root path in case bash pipe loses directory context
REM Use sed to extract script from line 30 to end
"%MSYS2_PATH%\usr\bin\bash.exe" -l -c "sed -n '35,$p' \"$1\" | tr -d '\r' | bash" -- "%~f0"

echo.
if %ERRORLEVEL% EQU 0 (
    echo Build complete!
) else (
    echo Build failed with error %ERRORLEVEL%
)
exit /b

REM --- END OF BATCH, START OF BASH SCRIPT ---
#!/bin/bash
set -e

# We are running from pipe, so SCRIPT_DIR cannot be derived from $0
# We assume CWD is project root (set by batch wrapper)
PROJECT_ROOT="$(pwd)"
BUILD_DIR="${PROJECT_ROOT}/builddir"
DIST_DIR="${PROJECT_ROOT}/dist"
EXE="${BUILD_DIR}/ui-gtk/uget-gtk.exe"
MINGW_BIN="/mingw64/bin"
MINGW_PREFIX="/mingw64"

# Check if exe exists
if [ ! -f "$EXE" ]; then
    echo "Error: $EXE not found. Run 'meson compile -C builddir' first."
    exit 1
fi

# Create dist folder with proper structure
echo "Creating distribution folder..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/bin"

# Create .gitignore
echo "*" > "$DIST_DIR/.gitignore"

# Copy main executable to bin subfolder
echo "Copying uget-gtk.exe..."
cp "$EXE" "$DIST_DIR/bin/"

# Function to copy a DLL and its dependencies recursively
copy_dll() {
    local dll="$1"
    local basename=$(basename "$dll")
    
    if [ -f "$DIST_DIR/bin/$basename" ]; then
        return
    fi
    
    # Skip Windows system DLLs
    case "$basename" in
        KERNEL32.dll|ntdll.dll|msvcrt.dll|USER32.dll|GDI32.dll|SHELL32.dll|\
        ADVAPI32.dll|WS2_32.dll|ole32.dll|OLEAUT32.dll|COMDLG32.dll|\
        IMM32.dll|WINMM.dll|WINSPOOL.DRV|dwmapi.dll|UxTheme.dll|\
        SHLWAPI.dll|VERSION.dll|CRYPT32.dll|bcrypt.dll|SETUPAPI.dll|\
        COMCTL32.dll|api-ms-*|ext-ms-*|RPCRT4.dll|secur32.dll|DNSAPI.dll|\
        IPHLPAPI.DLL|WLDAP32.dll|USERENV.dll|cfgmgr32.dll|powrprof.dll|\
        kernel32.dll|user32.dll|gdi32.dll|shell32.dll|advapi32.dll|\
        ws2_32.dll|ws2_32.DLL|ole32.dll|oleaut32.dll|comdlg32.dll|\
        imm32.dll|winmm.dll|winspool.drv|shlwapi.dll|version.dll|\
        crypt32.dll|setupapi.dll|comctl32.dll|rpcrt4.dll|dnsapi.dll|\
        iphlpapi.dll|wldap32.dll|userenv.dll|netapi32.dll|NETAPI32.dll|\
        mswsock.dll|MSWSOCK.dll|sechost.dll|SECHOST.dll|dbghelp.dll|DBGHELP.dll)
            return
            ;;
    esac
    
    local found=""
    if [ -f "$MINGW_BIN/$basename" ]; then
        found="$MINGW_BIN/$basename"
    elif [ -f "$dll" ]; then
        found="$dll"
    fi
    
    if [ -n "$found" ]; then
        echo "  Copying $basename"
        cp "$found" "$DIST_DIR/bin/"
        for dep in $(ldd "$found" 2>/dev/null | grep -i mingw64 | awk '{print $3}'); do
            if [ -f "$dep" ]; then
                copy_dll "$dep"
            fi
        done
    fi
}

echo "Copying DLL dependencies..."
for dll in $(ldd "$EXE" 2>/dev/null | grep -i mingw64 | awk '{print $3}'); do
    copy_dll "$dll"
done

echo ""
echo "Copying uGet project files..."

# GLib schemas
echo "  Copying GLib schemas..."
mkdir -p "$DIST_DIR/share/glib-2.0/schemas"
cp "$MINGW_PREFIX/share/glib-2.0/schemas/gschemas.compiled" "$DIST_DIR/share/glib-2.0/schemas/" 2>/dev/null || \
    glib-compile-schemas "$MINGW_PREFIX/share/glib-2.0/schemas" --targetdir="$DIST_DIR/share/glib-2.0/schemas"

# System hicolor index.theme
echo "  Copying system hicolor theme base..."
mkdir -p "$DIST_DIR/share/icons/hicolor"
cp "$MINGW_PREFIX/share/icons/hicolor/index.theme" "$DIST_DIR/share/icons/hicolor/" 2>/dev/null || true

# Fallback icons (image-missing)
echo "  Copying required fallback icons..."
for size in 16x16 22x22 24x24 32x32 48x48; do
    src="$MINGW_PREFIX/share/icons/AdwaitaLegacy/$size/status/image-missing.png"
    if [ -f "$src" ]; then
        mkdir -p "$DIST_DIR/share/icons/hicolor/$size/status"
        cp "$src" "$DIST_DIR/share/icons/hicolor/$size/status/"
    fi
done

# uGet icons
echo "  Copying uGet icons..."
if [ -d "${PROJECT_ROOT}/pixmaps/icons/hicolor" ]; then
    for dir in "${PROJECT_ROOT}/pixmaps/icons/hicolor"/*/; do
        cp -r "$dir" "$DIST_DIR/share/icons/hicolor/"
    done
fi

# uGet logo
echo "  Copying uGet logo..."
mkdir -p "$DIST_DIR/share/pixmaps/uget"
if [ -f "${PROJECT_ROOT}/pixmaps/logo.png" ]; then
    cp "${PROJECT_ROOT}/pixmaps/logo.png" "$DIST_DIR/share/pixmaps/uget/"
fi

# uGet sounds
echo "  Copying uGet sounds..."
mkdir -p "$DIST_DIR/share/sounds/uget"
if [ -f "${PROJECT_ROOT}/sounds/notification.wav" ]; then
    cp "${PROJECT_ROOT}/sounds/notification.wav" "$DIST_DIR/share/sounds/uget/"
fi

# GTK settings
echo "  Creating GTK settings..."
mkdir -p "$DIST_DIR/etc/gtk-3.0"
cat > "$DIST_DIR/etc/gtk-3.0/settings.ini" << 'EOF'
[Settings]
gtk-theme-name=Windows10
gtk-icon-theme-name=hicolor
gtk-font-name=Segoe UI 9
gtk-button-images=false
gtk-menu-images=false
EOF

# Launcher
echo "  Creating launcher..."
cat > "$DIST_DIR/uget.bat" << 'EOF'
@echo off
start "" "%~dp0bin\uget-gtk.exe"
EOF

# Count files
DLL_COUNT=$(ls -1 "$DIST_DIR/bin"/*.dll 2>/dev/null | wc -l)
TOTAL_SIZE=$(du -sh "$DIST_DIR" | cut -f1)

echo ""
echo "Distribution created in: $DIST_DIR"
echo "  - bin/uget-gtk.exe"
echo "  - $DLL_COUNT DLL files"
echo "  - uGet icons, logo, sounds in share/"
echo "  - Total size: $TOTAL_SIZE"
echo ""
echo "Run uget.bat or bin/uget-gtk.exe"
