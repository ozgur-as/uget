# uglib - Core utility library for uGet
# Static library containing JSON, threading, networking utilities

uglib_sources = files(
  'UgStdio.c',
  'UgString.c',
  'UgThread.c',
  'UgSocket.c',
  'UgUtil.c',
  'UgFileUtil.c',
  'UgArray.c',
  'UgList.c',
  'UgSLink.c',
  'UgOption.c',
  'UgUri.c',
  'UgNode.c',
  'UgData.c',
  'UgInfo.c',
  'UgRegistry.c',
  'UgValue.c',
  'UgEntry.c',
  'UgBuffer.c',
  'UgJson.c',
  'UgJson-custom.c',
  'UgJsonFile.c',
  'UgJsonrpc.c',
  'UgJsonrpcSocket.c',
  'UgJsonrpcCurl.c',
  'UgHtml.c',
  'UgHtmlEntry.c',
  'UgHtmlFilter.c',
)

uglib_inc = include_directories('.')

# uglib dependencies
uglib_deps = [
  glib_dep,
  curl_dep,
  threads_dep,
]

# Windows socket library for UgSocket, UgJsonrpcSocket
if host_system == 'windows'
  ws2_32_dep = meson.get_compiler('c').find_library('ws2_32')
  uglib_deps += ws2_32_dep
endif

uglib_lib = static_library('uglib',
  uglib_sources,
  include_directories: [uglib_inc, config_inc],
  dependencies: uglib_deps,
  c_args: ['-DNDEBUG'],
)

uglib_dep = declare_dependency(
  link_with: uglib_lib,
  include_directories: uglib_inc,
  dependencies: uglib_deps,  # Propagate dependencies to consumers
)
