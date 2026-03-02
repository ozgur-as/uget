# uGet Download Manager

A lightweight download manager with a GTK4 interface. Runs on Linux and Windows.

Supports HTTP, HTTPS, FTP, and SFTP downloads with multi-connection downloading, queue management, and aria2 integration for BitTorrent and metalink support.

Originally created by C.H. Huang. This fork revives the project with a modern GTK4 UI, Meson build system, and ongoing maintenance.

## Features

- Multi-connection downloading for faster speeds
- Download queue management with categories
- Pause, resume, and schedule downloads
- aria2 integration for BitTorrent and metalink support
- System tray integration (StatusNotifierItem)
- 36 languages supported
- Desktop notifications and notification sounds

## Building from Source

```bash
meson setup build
ninja -C build
meson test -C build       # run tests
ninja -C build install    # install
```

See [BUILDING.md](BUILDING.md) for detailed instructions, dependencies, and build options.

## Status

This project is under active development. The GTK4 migration is complete but some features are still being worked on. See [CHANGELOG.md](CHANGELOG.md) for details.

## License

LGPL-2.1-or-later (with OpenSSL exception). See [COPYING](COPYING) for details.
