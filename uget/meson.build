# uget - Download manager core library
# Static library containing plugins, task management, download logic

uget_sources = files(
  'UgetSequence.c',
  'UgetRss.c',
  'UgetRpc.c',
  'UgetOption.c',
  'UgetData.c',
  'UgetFiles.c',
  'UgetNode.c',
  'UgetNode-compare.c',
  'UgetNode-filter.c',
  'UgetTask.c',
  'UgetHash.c',
  'UgetSite.c',
  'UgetApp.c',
  'UgetEvent.c',
  'UgetPlugin.c',
  'UgetA2cf.c',
  'UgetCurl.c',
  'UgetAria2.c',
  'UgetMedia.c',
  'UgetMedia-youtube.c',
  'UgetPluginCurl.c',
  'UgetPluginAria2.c',
  'UgetPluginMedia.c',
  'UgetPluginAgent.c',
  'UgetPluginMega.c',
  'UgetPluginEmpty.c',
)

# Add pwmd support if enabled
if have_libpwmd
  uget_sources += files('pwmd.c')
endif

uget_inc = include_directories('.')

uget_deps = [
  uglib_dep,
  glib_dep,
  threads_dep,
]

if have_libpwmd
  uget_deps += libpwmd_dep
endif

uget_lib = static_library('uget',
  uget_sources,
  include_directories: [uget_inc, config_inc],
  dependencies: uget_deps,
  c_args: ['-DNDEBUG'],
)

uget_dep = declare_dependency(
  link_with: uget_lib,
  include_directories: uget_inc,
)
