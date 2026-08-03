# Building uGet from Source

## Linux

### Dependencies

> **Distro note:** the version floors below are derived from the newest API the
> code actually calls, so anything from **Ubuntu 24.04 LTS onward** can build
> uGet. A clean build of 24.04 (GTK 4.14.5, GLib 2.80.0, libcurl 8.5.0) is
> verified: all targets compile with no warnings and the full test suite passes.
> Debian 13 "Trixie" (GTK 4.18), Fedora 43+, Arch Linux, and openSUSE
> Tumbleweed are all newer than the floor and should build unchanged.
>
> Too old: Ubuntu 22.04 (GTK 4.6) and Debian 12 (GTK 4.8) — both predate the
> `GtkAlertDialog`/`GtkFileDialog` APIs the UI is built on.

**Required:**
- A C17-capable compiler (GCC 8+ or Clang 6+)
- GLib >= 2.76.0
- GTK4 >= 4.14.0
- libcurl >= 7.55.0
- OpenSSL (crypto for the MEGA plugin; required unless disabled with `-Dopenssl=false`)
- gettext
- Meson >= 1.1.0
- Ninja

**Optional:**
- libnotify (desktop notifications)
- GStreamer 1.0 (notification sounds)
- libdbusmenu-glib (system tray icon)
- GnuTLS / libgcrypt (alternative crypto backend; build with `-Dgnutls=true -Dopenssl=false`)

**Runtime only (not needed to build):**
- yt-dlp — external extractor for media-site downloads (used by `UgetResolver` and the ytdlp plugin)
- aria2 — optional alternative download backend (aria2 plugin)

#### Debian / Ubuntu

```bash
sudo apt install build-essential pkg-config meson ninja-build gettext \
  libglib2.0-dev libgtk-4-dev libcurl4-openssl-dev libnotify-dev \
  libgstreamer1.0-dev libdbusmenu-glib-dev libssl-dev
```

This exact command is what the verified 24.04 build uses. `build-essential` and
`pkg-config` are already present on most desktop installs, but a minimal or
container image needs them named explicitly.

#### Fedora

```bash
sudo dnf install meson ninja-build gettext-devel glib2-devel gtk4-devel \
  libcurl-devel libnotify-devel gstreamer1-devel \
  libdbusmenu-devel openssl-devel
```

#### Arch Linux

```bash
sudo pacman -S meson ninja gettext glib2 gtk4 curl libnotify \
  gstreamer libdbusmenu-glib openssl
```

### Build

```bash
meson setup build
ninja -C build
```

### Run Tests

```bash
meson test -C build
```

### Install

```bash
ninja -C build install
```

By default this installs to `/usr/local`. To change the prefix:

```bash
meson setup build --prefix=/usr
```

### Build Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `notify` | feature | auto | Desktop notifications |
| `gstreamer` | feature | auto | Notification sounds |
| `appindicator` | feature | auto | System tray icon (Unix only) |
| `pwmd` | feature | disabled | Password manager daemon |
| `openssl` | boolean | true | Use OpenSSL for crypto |
| `gnutls` | boolean | false | Use GnuTLS instead of OpenSSL |
| `unix_socket` | boolean | false | Unix domain socket for JSON-RPC |
| `rss_notify` | boolean | true | RSS notifications |

Example:

```bash
meson setup build -Dgnutls=true -Dopenssl=false
```

## Windows

The Windows target is cross-compiled from Linux with MinGW-w64, using Fedora's
`mingw64-*` packages for the dependency set. `mingw64-gtk4` is 4.14.4, which is
why the GTK floor above matters — uGet cannot cross-compile against a higher
one. No Windows machine is involved at any stage.

> **Status:** `uget-gtk.exe` compiles and links cleanly (no errors, no
> warnings) and **starts and runs under Wine** with no criticals. It has **not
> been run on real Windows**, and the tray icon in particular cannot be
> exercised under Wine's headless shell — treat the Windows build as untested
> at runtime. Tray, balloon notifications, and sound are all implemented.

### Cross-compiling

Fedora 43 provides both the toolchain and a Meson cross file, so a container is
enough:

```bash
podman run --rm -v "$PWD":/src:ro fedora:43 bash -c '
  dnf install -y -q mingw64-gcc mingw64-gtk4 mingw64-glib2 mingw64-curl \
    mingw64-openssl mingw64-winpthreads meson ninja-build gettext
  meson setup /build /src \
    --cross-file /usr/share/mingw/toolchain-mingw64.meson \
    -Dnotify=disabled -Dgstreamer=disabled -Dappindicator=disabled
  ninja -C /build
'
```

Those three options are Linux-only mechanisms (D-Bus and freedesktop
protocols); Windows uses native equivalents instead, so disabling them loses
nothing:

| Option | Windows equivalent |
|--------|--------------------|
| `appindicator` (StatusNotifierItem tray) | `Shell_NotifyIcon` — `ui-gtk/UgtkTrayIcon-win32.c` |
| `notify` (libnotify) | tray balloon (`NIF_INFO`), same file |
| `gstreamer` (sounds) | `PlaySoundW` from **winmm** |

### Packaging

`packaging/windows/make-dist.sh` turns a cross build into both distributables:

```bash
packaging/windows/make-dist.sh --build-dir build-mingw --output-dir dist
```

| Artifact | Contents |
|----------|----------|
| `uget-<version>-win64-portable.zip` | Self-contained; keeps its config in `config/` beside the executable |
| `uget-<version>-win64-setup.exe` | NSIS installer: Program Files, Start Menu and desktop shortcuts, Add/Remove Programs entry, uninstaller |

The script resolves the DLL closure from the import tables (bundling only
libraries present in the MinGW sysroot, never Windows system DLLs) and stages
the GTK runtime data: GSettings schemas, the Adwaita and hicolor icon themes,
gdk-pixbuf loaders, and the compiled translations.

It needs `makensis` for the installer and skips it with a message if absent
(`dnf install mingw64-nsis`). Output goes to `dist/`, which is gitignored.

> **Note:** Fedora's `mingw64-nsis` ships only amd64 stubs, while `makensis`
> loads an x86 stub at startup — before the script's `Target amd64-unicode`
> applies — and will not start without one. `make-dist.sh` detects this and
> hands it a private `NSISDIR` where the amd64 stubs also answer to the x86
> names. The installer produced is a genuine 64-bit binary either way.
