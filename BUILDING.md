# Building uGet from Source

> **Note:** Build instructions have only been tested on Ubuntu so far. Other distributions and platforms may need adjustments.

## Linux

### Dependencies

> **Distro note:** GTK4 >= 4.20 and GLib >= 2.86 ship with GNOME 49
> (September 2025). Older stable releases are too old to build — notably
> Ubuntu 24.04 LTS (GTK 4.14, libcurl 8.5) and Debian 13 "Trixie"
> (GTK 4.18). Use Fedora 43+, Ubuntu 25.10+, Arch Linux, or openSUSE
> Tumbleweed.

**Required:**
- A C17-capable compiler (GCC 8+ or Clang 6+)
- GLib >= 2.86.0
- GTK4 >= 4.20.0
- libcurl >= 8.11.0
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
sudo apt install meson ninja-build gettext libglib2.0-dev libgtk-4-dev \
  libcurl4-openssl-dev libnotify-dev libgstreamer1.0-dev \
  libdbusmenu-glib-dev libssl-dev
```

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

Windows builds use MSYS2 with MinGW. Instructions coming soon.
