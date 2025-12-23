#!/bin/bash
set -e

# uGet Linux Portable Distribution Builder
# Run this script to create a self-contained 'dist' folder
# containing the executable and its dependencies.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/builddir-linux"
DIST_DIR="${PROJECT_ROOT}/dist"
EXE="${BUILD_DIR}/ui-gtk/uget-gtk"

# Build project
echo "Building project in $BUILD_DIR..."
# Clean build
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "Configuring build directory..."
meson setup "$BUILD_DIR" "$PROJECT_ROOT"
meson compile -C "$BUILD_DIR"

# Check for executable
if [ ! -f "$EXE" ]; then
    echo "Error: Executable not found at $EXE"
    echo "Please compile the project first (e.g., meson compile -C builddir)"
    exit 1
fi

echo "Creating distribution folder at $DIST_DIR..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/lib"
mkdir -p "$DIST_DIR/share"

# Copy main executable
echo "Copying executable..."
cp "$EXE" "$DIST_DIR/bin/"

# Function to copy shared libraries
copy_deps() {
    local bin="$1"
    echo "Resolving dependencies for $(basename "$bin")..."
    
    # Get list of dependencies using ldd
    # Format: libname.so => /path/to/libname.so (0x...)
    local deps=$(ldd "$bin" | awk '{print $3}' | grep "^/")
    
    for lib in $deps; do
        local libname=$(basename "$lib")
        
        # Exclude core system libraries to ensure portability
        case "$libname" in
            # Glibc / core
            libc.so*|libpthread.so*|libdl.so*|librt.so*|libm.so*|libresolv.so*|ld-linux*) continue ;;
            # C++ stdlib (sometimes better to bundle, but often causes conflicts if system is different)
            # libstdc++.so*|libgcc_s.so*) continue ;; 
            # X11 / Graphics (usually present on target)
            # libX11.so*|libxcb.so*|libXau.so*|libXdmcp.so*|libGL.so*|libdrm.so*) continue ;;
            # Font config (system specific config)
            libfontconfig.so*|libfreetype.so*) continue ;;
        esac
        
        if [ ! -f "$DIST_DIR/lib/$libname" ]; then
            echo "  Bundling $libname"
            cp -L "$lib" "$DIST_DIR/lib/"
        fi
    done
}

copy_deps "$EXE"

echo "Copying assets..."

# Copy uGet icons
if [ -d "${PROJECT_ROOT}/pixmaps/icons/hicolor" ]; then
    echo "  Copying icons..."
    mkdir -p "$DIST_DIR/share/icons"
    cp -r "${PROJECT_ROOT}/pixmaps/icons/hicolor" "$DIST_DIR/share/icons/"
fi

# Copy Logo
if [ -f "${PROJECT_ROOT}/pixmaps/logo.png" ]; then
    mkdir -p "$DIST_DIR/share/pixmaps/uget"
    cp "${PROJECT_ROOT}/pixmaps/logo.png" "$DIST_DIR/share/pixmaps/uget/"
fi

# Copy Sounds
if [ -f "${PROJECT_ROOT}/sounds/notification.wav" ]; then
    echo "  Copying sounds..."
    mkdir -p "$DIST_DIR/share/sounds/uget"
    cp "${PROJECT_ROOT}/sounds/notification.wav" "$DIST_DIR/share/sounds/uget/"
fi

# Copy GLib Schemas (if available locally or from system)
# Note: In a real portable app, you might want to compile source schemas.
# Here we try to find compiled schemas or source schemas from the project.
if [ -d "${PROJECT_ROOT}/ui-gtk/data/glib-2.0/schemas" ]; then
     echo "  Compiling schemas..."
     mkdir -p "$DIST_DIR/share/glib-2.0/schemas"
     glib-compile-schemas "${PROJECT_ROOT}/ui-gtk/data/glib-2.0/schemas" --targetdir="$DIST_DIR/share/glib-2.0/schemas"
fi

# Create Launcher Script
echo "Creating launcher script..."
cat > "$DIST_DIR/uget" << 'EOF'
#!/bin/bash
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="$HERE/lib:$LD_LIBRARY_PATH"
export XDG_DATA_DIRS="$HERE/share:$XDG_DATA_DIRS"
export GSETTINGS_SCHEMA_DIR="$HERE/share/glib-2.0/schemas"

exec "$HERE/bin/uget-gtk" "$@"
EOF
chmod +x "$DIST_DIR/uget"

# Create .gitignore for dist folder
echo "*" > "$DIST_DIR/.gitignore"

echo "Done! Run '$DIST_DIR/uget' to test."
