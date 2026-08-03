# ui-gtk - GTK4 User Interface for uGet
# Main executable

ui_gtk_sources = files(
  'UgtkUtil.c',
  'UgtkConfig.c',
  'UgtkSetting.c',
  'UgtkNodeObject.c',
  'UgtkNodeList.c',
  'UgtkNodeTree.c',
  'UgtkNodeView.c',
  'UgtkTraveler.c',
  'UgtkSummary.c',
  'UgtkBanner.c',
  'UgtkSequence.c',
  'UgtkSelector.c',
  'UgtkProxyForm.c',
  'UgtkDownloadForm.c',
  'UgtkCategoryForm.c',
  'UgtkNodeDialog.c',
  'UgtkBatchDialog.c',
  'UgtkFormatPicker.c',
  'UgtkScheduleForm.c',
  'UgtkSettingForm.c',
  'UgtkSettingDialog.c',
  'UgtkConfirmDialog.c',
  'UgtkAboutDialog.c',
  'UgtkMenubar.c',
  'UgtkMenubar-ui.c',
  'UgtkApp.c',
  'UgtkApp-ui.c',
  'UgtkApp-callback.c',
  'UgtkApp-timeout.c',
  'UgtkApp-main.c',
)

# Tray icon: two backends providing the same API, exactly one is compiled.
# Windows uses Shell_NotifyIcon; everything else uses StatusNotifierItem.
if host_system == 'windows'
  ui_gtk_sources += files('UgtkTrayIcon-win32.c')
else
  ui_gtk_sources += files('UgtkTrayIcon.c')
endif

ui_gtk_inc = include_directories('.')

# Windows resource file (icon + version info)
if host_system == 'windows'
  windows = import('windows')

  # Derive the Windows version fields from meson.project_version() so they
  # never drift from the project version. e.g. "2.3.0" -> string "2.3.0" and
  # numeric "2,3,0,0". Any -rcN suffix is stripped for the numeric field.
  ver_padded = (meson.project_version().split('-')[0] + '.0.0.0').split('.')
  rc_conf = configuration_data()
  rc_conf.set('VERSION', meson.project_version())
  rc_conf.set('VER_COMMA', '@0@,@1@,@2@,@3@'.format(
    ver_padded[0], ver_padded[1], ver_padded[2], ver_padded[3]))
  uget_rc = configure_file(
    input: '../packaging/windows/uget.rc.in',
    output: 'uget.rc',
    configuration: rc_conf,
  )

  ui_gtk_res = windows.compile_resources(
    uget_rc,
    include_directories: include_directories('../packaging/windows'),
  )
else
  ui_gtk_res = []
endif

# Collect all dependencies
ui_gtk_deps = [
  uget_dep,
  uglib_dep,
  gtk_dep,
  glib_dep,
  curl_dep,
  threads_dep,
]

# Add crypto dependency
if use_openssl and not use_gnutls
  ui_gtk_deps += libcrypto_dep
elif use_gnutls
  ui_gtk_deps += libgcrypt_dep
endif

# Add optional dependencies
if have_libnotify
  ui_gtk_deps += libnotify_dep
endif

if have_gstreamer
  ui_gtk_deps += gstreamer_dep
endif

if have_appindicator
  ui_gtk_deps += appindicator_dep
endif

if have_libpwmd
  ui_gtk_deps += libpwmd_dep
endif

# Windows-specific libraries
if host_system == 'windows'
  win_cc = meson.get_compiler('c')
  ui_gtk_deps += win_cc.find_library('ws2_32')
  # winmm: PlaySoundW(), the Windows backend of ugtk_play_sound()
  ui_gtk_deps += win_cc.find_library('winmm')
endif

uget_gtk = executable('uget-gtk',
  ui_gtk_sources,
  ui_gtk_res,
  include_directories: [ui_gtk_inc, config_inc],
  dependencies: ui_gtk_deps,
  win_subsystem: 'windows',  # GUI app, no console window
  install: true,
)
