# Building uGet from Source

> **Note:** Build instructions have only been tested on Ubuntu so far. Other distributions and platforms may need adjustments.

## Linux

### Dependencies

**Required:**
- GLib >= 2.86.0
- GTK4 >= 4.20.0
- libcurl >= 8.11.0
- gettext
- Meson >= 1.1.0
- Ninja

**Optional:**
- libnotify (desktop notifications)
- GStreamer 1.0 (notification sounds)
- libdbusmenu-glib (system tray icon)
- OpenSSL or GnuTLS (crypto support)

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
| `appindicator` | feature | auto | System tray icon (Linux only) |
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
