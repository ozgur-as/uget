# Changelog

## 2.3.0 (WIP)

- Migrated entire UI from GTK3 to GTK4
- Migrated build system from Autotools to Meson
- System tray: replaced AppIndicator3 with StatusNotifierItem (GDBus + libdbusmenu-glib)
- Crypto: migrated from deprecated AES_* to EVP_* (modern OpenSSL API)
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
