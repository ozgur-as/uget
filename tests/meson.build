# tests - Unit tests for uget and uglib

test_deps = [
  uglib_dep,
  uget_dep,
  glib_dep,
  curl_dep,
  threads_dep,
]

# Add crypto dependency for plugin tests
if use_openssl and not use_gnutls
  test_deps += libcrypto_dep
elif use_gnutls
  test_deps += libgcrypt_dep
endif

# test-json: JSON parser tests
test_json = executable('test-json',
  'test-json.c',
  include_directories: config_inc,
  dependencies: [uglib_dep, glib_dep, curl_dep, threads_dep],
)
test('json', test_json)

# test-jsonrpc: JSON-RPC tests
test_jsonrpc = executable('test-jsonrpc',
  'test-jsonrpc.c',
  include_directories: config_inc,
  dependencies: [uget_dep, uglib_dep, glib_dep, curl_dep, threads_dep],
)
test('jsonrpc', test_jsonrpc)

# test-info: Info structure tests
test_info = executable('test-info',
  'test-info.c',
  include_directories: config_inc,
  dependencies: [uglib_dep, glib_dep, curl_dep, threads_dep],
)
test('info', test_info)

# test-uglib: uglib unit tests
test_uglib = executable('test-uglib',
  'test-uglib.c',
  include_directories: config_inc,
  dependencies: [uglib_dep, glib_dep, curl_dep, threads_dep],
)
test('uglib', test_uglib)

# test-uget: uget unit tests
test_uget = executable('test-uget',
  'test-uget.c',
  include_directories: config_inc,
  dependencies: [uget_dep, uglib_dep, glib_dep, curl_dep, threads_dep],
)
test('uget', test_uget)

# test-plugin+app: Plugin integration tests
test_plugin_app = executable('test-plugin+app',
  'test-plugin+app.c',
  include_directories: config_inc,
  dependencies: test_deps,
)
test('plugin-app', test_plugin_app)
