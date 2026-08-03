# Changelog

## 2.3.0 (WIP)

- Restored the Windows build: uGet again compiles and links for Windows,
  cross-compiled from Linux with MinGW-w64 (see `BUILDING.md`)
- Added Windows packaging: an NSIS installer and a portable zip, both built
  on Linux by `packaging/windows/make-dist.sh`
- Translations are now built for Windows as well; the Windows build was
  English-only because the build skipped the message catalogs entirely
- Fixed the Windows build not finding its bundled GSettings schemas, which
  made a relocatable install (portable zip, or any non-default path) fall
  back to defaults
- Windows tray icon reimplemented on `Shell_NotifyIcon`, replacing the
  `GtkStatusIcon` that GTK4 removed: context menu mirroring the Unix tray,
  three icon states embedded as resources, tooltip showing transfer rates,
  and recovery when Explorer restarts
- Windows notifications restored as tray balloons; the previous
  implementation was dead code that searched for a GTK3 window class
- Fixed a crash on Windows startup: uGet requested the GTK3 GSettings schema
  `org.gtk.Settings.FileChooser`, and a missing schema aborts the process.
  Uses the GTK4 id and degrades gracefully when schemas are absent
- Fixed GTK4 theme parser errors from `!important` in the Windows menu CSS
  (GTK4 dropped support for it)
- Fixed 64-bit JSON serialization on Windows: `%I64d`/`%I64u` are an MSVC-only
  extension that MinGW's ANSI stdio does not understand, so every int64 value
  written to JSON was serialized incorrectly. Now uses `PRId64`/`PRIu64`
- Fixed `UgJsonrpcSocket` storing a `SOCKET` in an `int`, which truncated
  socket handles on 64-bit Windows
- Fixed `CURLOPT_POSTFIELDSIZE` being passed `size_t`/`int` instead of `long`,
  and `ShellExecute` results being truncated to `int` from a 64-bit `HINSTANCE`
- Lowered dependency floors to match the newest API actually used: GTK4 4.14
  (was 4.20), GLib 2.76 (was 2.86), libcurl 7.55 (was 8.11). Ubuntu 24.04 LTS
  and Debian 13 can now build uGet
- Migrated entire UI from GTK3 to GTK4
- Migrated build system from Autotools to Meson
- Media: replaced the ~2500-line in-tree URL parser with an external
  extractor integration — async resolution via `UgetResolver`, a
  format picker, and a `UgetPluginYtdlp` downloader for streams needing
  video+audio muxing
- System tray: replaced AppIndicator3 with StatusNotifierItem (GDBus + libdbusmenu-glib)
- Crypto: migrated from deprecated AES_* to EVP_* (modern OpenSSL API)
- Replaced deprecated GLib slice allocator (g_slice_*) with g_malloc/g_free
- Bumped C standard from C11 to C17; adopted designated initializers for
  plugin/data descriptor structs and static_assert guards for binary-format
  type-punning unions
- libcurl: replaced deprecated CURLOPT/CURLINFO with modern variants
- Full Wayland support (proper app-id, icon theme, no X11 forcing)
- GtkTreeView/GtkListStore replaced with GtkColumnView/GListModel
- GtkDialog/GtkMessageDialog replaced with GtkWindow/GtkAlertDialog
- GtkFileChooserDialog replaced with async GtkFileDialog API
- GtkMenu/GtkMenuItem replaced with GtkPopoverMenu/GMenuModel + GActions
- Removed all legacy Autotools files, GTK version conditionals, and dead code
- Added i18n locale compilation (36 languages)
- Added install targets for desktop file, icons, sounds, and translations
- Fixed all compiler warnings
- Fixed all GTK4 runtime warnings

## 2.2.3 (2020-01-01)

- Add parser for YouTube recently changed field

## 2.2.2 (2019-05-20)

- Use quicksort to sort downloads
- Backup torrent and metalink files
- curl plug-in: handle duplicate files with double extensions

## 2.2.1 (2018-03-08)

- Reduce memory usage
- mega plug-in: fix completed size display for files > 2G on 32-bit
- Adjust speed limit independently without enabling global speed limit
- Fix: Can't get 1080p video from YouTube

## 2.2.0 (2018-01-06)

- mega plug-in: create new plug-in for MEGA site
- all plug-in: avoid crash if plug-in failed to start
- Fix: some category/status doesn't refresh its download list

## 2.1.6 (2017-08-24)

- User can use sorting in any category and status
- curl plug-in: can use ftruncate() to create large file
- Fix: uGet doesn't close file descriptor when saving config file
- Fix: category functions can't work correctly

## 2.1.5 (2017-02-18)

- URL Sequence Batch can setup 3 wildcard range
- Avoid configure file corrupted on sudden shutdown
- curl plug-in: crashes when download file > 4GB
- Fix: Segmentation fault after pressing delete key
- Fix: Wayland hidden tray

## 2.1.4 (2016-05-16)

- In speed limit mode, adjust existing task speed when adding new task
- Add new setting "Display large icon"
- Add Unix domain socket option for JSON-RPC
- Use msys2 + mingw to build uGet for Windows

## 2.1.3 (2016-04-10)

- Fix: UI freeze if user activate download in sorted list

## 2.1.2 (2016-04-01)

- Revert URI decoder to 2.0.4

## 2.1.1 (2016-03-20)

- curl plug-in: fix a bug that downloaded file may be incomplete
- curl plug-in: improve downloaded segment handler
- curl plug-in: set min split size to 10 MiB
- media plug-in: report error if YouTube video has been removed

## 2.1.0 (2016-02-20)

- Add new media plug-in to get link from media website
- Portable mode support on Windows
- curl plug-in: avoid showing "99:99:99" in elapsed when starting
