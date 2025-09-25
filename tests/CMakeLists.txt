cmake_minimum_required(VERSION 3.10)

if(NOT WITH_GUI)
  message(STATUS "tests/: WITH_GUI=OFF -> not building legacy GLib/GTK tests")
  return()
endif()

set(UGLIB_SOURCES
  ${CMAKE_SOURCE_DIR}/uglib/UgStdio.c
  ${CMAKE_SOURCE_DIR}/uglib/UgString.c
  ${CMAKE_SOURCE_DIR}/uglib/UgThread.c
  ${CMAKE_SOURCE_DIR}/uglib/UgSocket.c
  ${CMAKE_SOURCE_DIR}/uglib/UgUtil.c
  ${CMAKE_SOURCE_DIR}/uglib/UgFileUtil.c
  ${CMAKE_SOURCE_DIR}/uglib/UgArray.c
  ${CMAKE_SOURCE_DIR}/uglib/UgList.c
  ${CMAKE_SOURCE_DIR}/uglib/UgSLink.c
  ${CMAKE_SOURCE_DIR}/uglib/UgOption.c
  ${CMAKE_SOURCE_DIR}/uglib/UgUri.c
  ${CMAKE_SOURCE_DIR}/uglib/UgNode.c
  ${CMAKE_SOURCE_DIR}/uglib/UgData.c
  ${CMAKE_SOURCE_DIR}/uglib/UgInfo.c
  ${CMAKE_SOURCE_DIR}/uglib/UgRegistry.c
  ${CMAKE_SOURCE_DIR}/uglib/UgValue.c
  ${CMAKE_SOURCE_DIR}/uglib/UgEntry.c
  ${CMAKE_SOURCE_DIR}/uglib/UgBuffer.c
  ${CMAKE_SOURCE_DIR}/uglib/UgJson.c
  ${CMAKE_SOURCE_DIR}/uglib/UgJson-custom.c
  ${CMAKE_SOURCE_DIR}/uglib/UgJsonFile.c
  ${CMAKE_SOURCE_DIR}/uglib/UgJsonrpc.c
  ${CMAKE_SOURCE_DIR}/uglib/UgJsonrpcSocket.c
  ${CMAKE_SOURCE_DIR}/uglib/UgJsonrpcCurl.c
  ${CMAKE_SOURCE_DIR}/uglib/UgHtml.c
  ${CMAKE_SOURCE_DIR}/uglib/UgHtmlEntry.c
  ${CMAKE_SOURCE_DIR}/uglib/UgHtmlFilter.c
)

add_library(uglib STATIC ${UGLIB_SOURCES})
target_include_directories(uglib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/uglib
    ${GLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)
target_link_directories(uglib PUBLIC ${GLIB_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(uglib
  PUBLIC
    Threads::Threads
    ${GLIB_LIBRARIES}
    ${CURL_LIBRARIES}
)
target_compile_options(uglib PRIVATE ${GLIB_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
target_compile_definitions(uglib PUBLIC HAVE_CONFIG_H)

set(UGET_SOURCES
  ${CMAKE_SOURCE_DIR}/uget/UgetSequence.c
  ${CMAKE_SOURCE_DIR}/uget/UgetRss.c
  ${CMAKE_SOURCE_DIR}/uget/UgetRpc.c
  ${CMAKE_SOURCE_DIR}/uget/UgetOption.c
  ${CMAKE_SOURCE_DIR}/uget/UgetData.c
  ${CMAKE_SOURCE_DIR}/uget/UgetFiles.c
  ${CMAKE_SOURCE_DIR}/uget/UgetNode.c
  ${CMAKE_SOURCE_DIR}/uget/UgetNode-compare.c
  ${CMAKE_SOURCE_DIR}/uget/UgetNode-filter.c
  ${CMAKE_SOURCE_DIR}/uget/UgetTask.c
  ${CMAKE_SOURCE_DIR}/uget/UgetHash.c
  ${CMAKE_SOURCE_DIR}/uget/UgetSite.c
  ${CMAKE_SOURCE_DIR}/uget/UgetApp.c
  ${CMAKE_SOURCE_DIR}/uget/UgetEvent.c
  ${CMAKE_SOURCE_DIR}/uget/UgetPlugin.c
  ${CMAKE_SOURCE_DIR}/uget/UgetA2cf.c
  ${CMAKE_SOURCE_DIR}/uget/UgetCurl.c
  ${CMAKE_SOURCE_DIR}/uget/UgetAria2.c
  ${CMAKE_SOURCE_DIR}/uget/UgetMedia.c
  ${CMAKE_SOURCE_DIR}/uget/UgetMedia-youtube.c
  ${CMAKE_SOURCE_DIR}/uget/UgetPluginCurl.c
  ${CMAKE_SOURCE_DIR}/uget/UgetPluginAria2.c
  ${CMAKE_SOURCE_DIR}/uget/UgetPluginMedia.c
  ${CMAKE_SOURCE_DIR}/uget/UgetPluginAgent.c
  ${CMAKE_SOURCE_DIR}/uget/UgetPluginEmpty.c
)

set(_uget_has_mega FALSE)
if(HAVE_GCRYPT AND HAVE_OPENSSL)
  list(APPEND UGET_SOURCES ${CMAKE_SOURCE_DIR}/uget/UgetPluginMega.c)
  set(_uget_has_mega TRUE)
else()
  message(STATUS "Skipping UgetPluginMega.c (requires libgcrypt and OpenSSL)")
endif()

if(HAVE_PWMD)
  list(APPEND UGET_SOURCES ${CMAKE_SOURCE_DIR}/uget/pwmd.c)
endif()

add_library(uget STATIC ${UGET_SOURCES})
target_include_directories(uget
  PUBLIC
    ${CMAKE_SOURCE_DIR}/uget
    ${CMAKE_SOURCE_DIR}/uglib
    ${GLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)
target_link_directories(uget PUBLIC ${GLIB_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(uget
  PUBLIC
    uglib
    Threads::Threads
    ${GLIB_LIBRARIES}
    ${CURL_LIBRARIES}
)
target_compile_options(uget PRIVATE ${GLIB_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
target_compile_definitions(uget PUBLIC HAVE_CONFIG_H)

if(HAVE_PWMD)
  target_include_directories(uget PUBLIC ${PWMD_INCLUDE_DIRS})
  target_link_directories(uget PUBLIC ${PWMD_LIBRARY_DIRS})
  target_link_libraries(uget PUBLIC ${PWMD_LIBRARIES})
endif()

if(HAVE_GNUTLS)
  target_include_directories(uget PUBLIC ${GNUTLS_INCLUDE_DIRS})
  target_link_directories(uget PUBLIC ${GNUTLS_LIBRARY_DIRS})
  target_link_libraries(uget PUBLIC ${GNUTLS_LIBRARIES})
endif()

if(HAVE_OPENSSL)
  set(_openssl_include_dirs)
  set(_openssl_library_dirs)
  set(_openssl_libs)
  if(OPENSSL_FOUND)
    list(APPEND _openssl_include_dirs ${OPENSSL_INCLUDE_DIRS})
    list(APPEND _openssl_library_dirs ${OPENSSL_LIBRARY_DIRS})
    list(APPEND _openssl_libs ${OPENSSL_LIBRARIES})
  endif()
  if(LIBSSL_FOUND)
    list(APPEND _openssl_include_dirs ${LIBSSL_INCLUDE_DIRS})
    list(APPEND _openssl_library_dirs ${LIBSSL_LIBRARY_DIRS})
    list(APPEND _openssl_libs ${LIBSSL_LIBRARIES})
  endif()
  if(LIBCRYPTO_FOUND)
    list(APPEND _openssl_include_dirs ${LIBCRYPTO_INCLUDE_DIRS})
    list(APPEND _openssl_library_dirs ${LIBCRYPTO_LIBRARY_DIRS})
    list(APPEND _openssl_libs ${LIBCRYPTO_LIBRARIES})
  endif()
  if(_openssl_include_dirs)
    target_include_directories(uget PUBLIC ${_openssl_include_dirs})
  endif()
  if(_openssl_library_dirs)
    target_link_directories(uget PUBLIC ${_openssl_library_dirs})
  endif()
  if(_openssl_libs)
    target_link_libraries(uget PUBLIC ${_openssl_libs})
  endif()
endif()

if(HAVE_GCRYPT)
  target_include_directories(uget PUBLIC ${GCRYPT_INCLUDE_DIRS})
  target_link_directories(uget PUBLIC ${GCRYPT_LIBRARY_DIRS})
  target_link_libraries(uget PUBLIC ${GCRYPT_LIBRARIES})
endif()

function(add_uget_test target)
  set(options)
  set(oneValueArgs LABELS WORKING_DIRECTORY)
  set(multiValueArgs SOURCES LIBS)
  cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT TEST_SOURCES)
    message(FATAL_ERROR "add_uget_test(${target}) requires SOURCES")
  endif()

  add_executable(${target} ${TEST_SOURCES})
  target_link_libraries(${target} PRIVATE ${TEST_LIBS})
  target_include_directories(${target} PRIVATE
    ${CMAKE_SOURCE_DIR}/uglib
    ${CMAKE_SOURCE_DIR}/uget
    ${GLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/generated
  )
  target_compile_options(${target} PRIVATE ${GLIB_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
  target_compile_definitions(${target} PRIVATE HAVE_CONFIG_H)

  add_test(NAME ${target} COMMAND ${target})

  if(TEST_LABELS)
    set(_labels "${TEST_LABELS}")
  else()
    set(_labels "unit")
  endif()
  if(TEST_WORKING_DIRECTORY)
    set(_work_dir "${TEST_WORKING_DIRECTORY}")
  else()
    set(_work_dir "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  set_tests_properties(${target} PROPERTIES
    LABELS "${_labels}"
    WORKING_DIRECTORY "${_work_dir}" )
endfunction()

add_uget_test(test_json
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-json.c
  LIBS uglib
)

add_uget_test(test_info
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-info.c
  LIBS uglib
)

add_uget_test(test_uglib
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-uglib.c
  LIBS uglib
)

add_uget_test(test_uget
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-uget.c
  LIBS uget
  LABELS "unit;net"
)

add_uget_test(test_jsonrpc
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-jsonrpc.c
  LIBS uget
  LABELS "unit;net"
)

if(TEST test_http)
  set_tests_properties(test_http PROPERTIES LABELS "unit;net")
endif()

if(_uget_has_mega)
  add_uget_test(test_plugin_app
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-plugin+app.c
    LIBS uget
    LABELS "unit;net"
  )
else()
  message(STATUS "Skipping test_plugin_app (Mega plug-in not available)")
endif()

set_property(TEST PROPERTY TIMEOUT 60)
