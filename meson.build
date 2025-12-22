# uGet Download Manager - Meson Build System
# Migrated from Autotools (configure.ac)

project('uget', 'c',
  version: '2.2.3',
  license: 'LGPL-2.1-or-later',
  meson_version: '>= 0.56.0',
  default_options: [
    'c_std=c11',
    'warning_level=1',
    'buildtype=debugoptimized',
  ]
)

cc = meson.get_compiler('c')
host_system = host_machine.system()

# ============================================================================
# Required Dependencies
# ============================================================================

glib_dep = dependency('glib-2.0', required: true)
gtk_dep = dependency('gtk+-3.0', version: '>= 3.4', required: true)
curl_dep = dependency('libcurl', version: '>= 7.19.1', required: true)
threads_dep = dependency('threads', required: true)

# ============================================================================
# Optional Dependencies
# ============================================================================

# libnotify - Desktop notifications (auto-detect)
notify_opt = get_option('notify')
libnotify_dep = dependency('libnotify', required: notify_opt)
have_libnotify = libnotify_dep.found()

# GStreamer - Audio support (auto-detect)
gstreamer_opt = get_option('gstreamer')
gstreamer_dep = dependency('gstreamer-1.0', required: gstreamer_opt)
have_gstreamer = gstreamer_dep.found()

# AppIndicator - System tray (auto on Unix, disabled on Windows)
appindicator_opt = get_option('appindicator')
if host_system == 'windows'
  # Disabled on Windows by default
  if appindicator_opt.enabled()
    appindicator_dep = dependency('appindicator3-0.1', required: true)
    have_appindicator = true
  else
    appindicator_dep = dependency('', required: false)
    have_appindicator = false
  endif
else
  appindicator_dep = dependency('appindicator3-0.1', required: appindicator_opt)
  have_appindicator = appindicator_dep.found()
endif

# pwmd - Password manager daemon (opt-in, fails if enabled but not found)
pwmd_opt = get_option('pwmd')
libpwmd_dep = dependency('libpwmd-8.0', version: '>= 8.3.0', required: pwmd_opt)
have_libpwmd = libpwmd_dep.found()

# ============================================================================
# Crypto Backend Selection
# ============================================================================

use_openssl = get_option('openssl')
use_gnutls = get_option('gnutls')

if use_gnutls
  libgcrypt_dep = dependency('libgcrypt', required: true)
  crypto_dep = libgcrypt_dep
  use_openssl = false
elif use_openssl
  libcrypto_dep = dependency('libcrypto', required: true)
  crypto_dep = libcrypto_dep
else
  crypto_dep = dependency('', required: false)
endif

# ============================================================================
# System Checks
# ============================================================================

# Check for posix_fallocate and ftruncate
have_posix_fallocate = cc.has_function('posix_fallocate')
have_ftruncate = cc.has_function('ftruncate')

# ============================================================================
# Configuration Header
# ============================================================================

conf_data = configuration_data()

conf_data.set_quoted('PACKAGE', meson.project_name())
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('VERSION', meson.project_version())

# Feature flags - only define when enabled (code uses #ifdef, not #if)
conf_data.set('HAVE_GLIB', 1)
if have_libnotify
  conf_data.set('HAVE_LIBNOTIFY', 1)
endif
if have_gstreamer
  conf_data.set('HAVE_GSTREAMER', 1)
endif
if have_appindicator
  conf_data.set('HAVE_APP_INDICATOR', 1)
endif
if have_libpwmd
  conf_data.set('HAVE_LIBPWMD', 1)
endif

# Crypto backend - only define if enabled (code uses #ifdef, not #if)
if use_openssl and not use_gnutls
  conf_data.set('USE_OPENSSL', 1)
endif
if use_gnutls
  conf_data.set('USE_GNUTLS', 1)
endif

if get_option('rss_notify')
  conf_data.set('HAVE_RSS_NOTIFY', 1)
endif
if get_option('unix_socket')
  conf_data.set('USE_UNIX_DOMAIN_SOCKET', 1)
endif

# System function checks
conf_data.set10('HAVE_POSIX_FALLOCATE', have_posix_fallocate)
conf_data.set10('HAVE_FTRUNCATE', have_ftruncate)

# Data directory for resources
conf_data.set_quoted('UG_DATADIR', get_option('prefix') / get_option('datadir'))

# Locale directory
conf_data.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))

configure_file(
  output: 'config.h',
  configuration: conf_data
)

# Add build directory to include path for config.h
config_inc = include_directories('.')

# Add HAVE_CONFIG_H so source files include config.h
add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

# ============================================================================
# Subdirectories
# ============================================================================

subdir('uglib')
subdir('uget')
subdir('ui-gtk')
subdir('tests')

# ============================================================================
# Summary
# ============================================================================

summary({
  'prefix': get_option('prefix'),
  'libdir': get_option('libdir'),
  'datadir': get_option('datadir'),
}, section: 'Directories')

summary({
  'libnotify': have_libnotify,
  'gstreamer': have_gstreamer,
  'appindicator': have_appindicator,
  'pwmd': have_libpwmd,
}, section: 'Optional Features')

summary({
  'OpenSSL': use_openssl and not use_gnutls,
  'GnuTLS': use_gnutls,
}, section: 'Crypto Backend')
