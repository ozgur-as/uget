# ui-gtk - GTK3 User Interface for uGet
# Main executable

ui_gtk_sources = files(
  'UgtkUtil.c',
  'UgtkConfig.c',
  'UgtkSetting.c',
  'UgtkNodeList.c',
  'UgtkNodeTree.c',
  'UgtkNodeView.c',
  'UgtkTraveler.c',
  'UgtkSummary.c',
  'UgtkTrayIcon.c',
  'UgtkBanner.c',
  'UgtkSequence.c',
  'UgtkSelector.c',
  'UgtkProxyForm.c',
  'UgtkDownloadForm.c',
  'UgtkCategoryForm.c',
  'UgtkNodeDialog.c',
  'UgtkBatchDialog.c',
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

ui_gtk_inc = include_directories('.')

# Windows resource file (icon + version info)
if host_system == 'windows'
  windows = import('windows')
  ui_gtk_res = windows.compile_resources(
    '../Windows/resources/uget.rc',
    include_directories: include_directories('../Windows/resources'),
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
  ws2_32_dep = meson.get_compiler('c').find_library('ws2_32')
  ui_gtk_deps += ws2_32_dep
endif

uget_gtk = executable('uget-gtk',
  ui_gtk_sources,
  ui_gtk_res,
  include_directories: [ui_gtk_inc, config_inc],
  dependencies: ui_gtk_deps,
  win_subsystem: 'windows',  # GUI app, no console window
  install: true,
)
