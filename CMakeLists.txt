cmake_minimum_required(VERSION 3.24)
project(uget VERSION 2.3.0 LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Verbose build" FORCE)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(WITH_GUI "Build GTK-based UI and GStreamer integrations" ON)
option(WITH_TRAY "Build system tray integration (requires GUI)" ON)
option(WITH_OPENSSL "Enable OpenSSL-based crypto/TLS" ON)
option(WITH_GNUTLS "Enable GnuTLS/libgcrypt backend" OFF)
option(WITH_PWMD "Enable libpwmd integration" ON)
option(ENABLE_SANITIZERS "Enable ASan/UBSan in Debug on non-MSVC builds" ON)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckSymbolExists)

# ---- Robust pkg-config discovery (works with vcpkg, MSYS2, Linux) ----
# Prefer 'pkgconf' on Windows (vcpkg installs it), 'pkg-config' elsewhere.
find_program(PKG_CONFIG_EXECUTABLE NAMES pkgconf pkg-config)
if(PKG_CONFIG_EXECUTABLE)
  message(STATUS "Using pkg-config at: ${PKG_CONFIG_EXECUTABLE}")
  # Some FindPkgConfig variants also read the env var:
  set(ENV{PKG_CONFIG} "${PKG_CONFIG_EXECUTABLE}")
else()
  message(STATUS "pkg-config not found on PATH yet; FindPkgConfig may fail unless the toolchain provides it.")
endif()

find_package(PkgConfig QUIET)

# Feature test macros on non-MSVC to expose POSIX prototypes (getaddrinfo, ftruncate, popen, etc.)
if(NOT MSVC)
  add_compile_definitions(_GNU_SOURCE)
endif()

set(HEADER_NAMES
  "sys/types.h"
  "sys/select.h"
  "sys/socket.h"
  "netdb.h"
  "arpa/inet.h"
  "netinet/in.h"
  "sys/time.h"
  "unistd.h"
  "fcntl.h"
  "poll.h"
  "sys/stat.h"
  "inttypes.h"
  "stdint.h"
)
set(HEADER_MACROS
  HAVE_SYS_TYPES_H
  HAVE_SYS_SELECT_H
  HAVE_SYS_SOCKET_H
  HAVE_NETDB_H
  HAVE_ARPA_INET_H
  HAVE_NETINET_IN_H
  HAVE_SYS_TIME_H
  HAVE_UNISTD_H
  HAVE_FCNTL_H
  HAVE_POLL_H
  HAVE_SYS_STAT_H
  HAVE_INTTYPES_H
  HAVE_STDINT_H
)

list(LENGTH HEADER_NAMES _header_count)
if(_header_count GREATER 0)
  math(EXPR _header_last "${_header_count} - 1")
  foreach(_idx RANGE 0 ${_header_last})
    list(GET HEADER_NAMES ${_idx} _hdr)
    list(GET HEADER_MACROS ${_idx} _macro)
    check_include_file("${_hdr}" ${_macro})
  endforeach()
endif()

check_symbol_exists(fileno "stdio.h" HAVE_FILENO)
check_symbol_exists(ftruncate "unistd.h" HAVE_FTRUNCATE)
check_symbol_exists(popen "stdio.h" HAVE_POPEN)
check_symbol_exists(pclose "stdio.h" HAVE_PCLOSE)

# pkg-config deps
if(NOT WITH_GUI AND WITH_TRAY)
  message(STATUS "WITH_GUI=OFF -> disabling system tray support")
  set(WITH_TRAY OFF CACHE BOOL "" FORCE)
endif()

set(HAVE_GTK 0)
set(HAVE_GSTREAMER 0)
set(HAVE_GLIB 0)
set(HAVE_CURL 0)

set(_uget_common_include_dirs)
set(_uget_common_library_dirs)
set(_uget_common_libraries)
set(_uget_common_compile_options)
set(_uget_gui_include_dirs)
set(_uget_gui_library_dirs)
set(_uget_gui_libraries)
set(_uget_gui_compile_options)

if(PkgConfig_FOUND)
  pkg_check_modules(GLIB REQUIRED glib-2.0>=2.76 gobject-2.0)
  add_compile_definitions(HAVE_GLIB=1)
  set(HAVE_GLIB 1)

  list(APPEND _uget_common_include_dirs ${GLIB_INCLUDE_DIRS})
  list(APPEND _uget_common_library_dirs ${GLIB_LIBRARY_DIRS})
  list(APPEND _uget_common_libraries ${GLIB_LIBRARIES})
  list(APPEND _uget_common_compile_options ${GLIB_CFLAGS_OTHER})
else()
  message(FATAL_ERROR "GLib detection requires pkg-config.")
endif()

if(WITH_GUI)
  if(PkgConfig_FOUND)
    pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.24)
    pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0)

    set(HAVE_GTK 1)
    set(HAVE_GSTREAMER 1)

    list(APPEND _uget_gui_include_dirs
      ${GTK_INCLUDE_DIRS}
      ${GST_INCLUDE_DIRS})
    list(APPEND _uget_gui_library_dirs
      ${GTK_LIBRARY_DIRS}
      ${GST_LIBRARY_DIRS})
    list(APPEND _uget_gui_libraries
      ${GTK_LIBRARIES}
      ${GST_LIBRARIES})
    list(APPEND _uget_gui_compile_options
      ${GTK_CFLAGS_OTHER}
      ${GST_CFLAGS_OTHER})
  else()
    message(FATAL_ERROR "WITH_GUI=ON but pkg-config is unavailable on this toolchain.")
  endif()
else()
  message(STATUS "WITH_GUI=OFF: skipping GTK/GStreamer detection")
endif()

if(WIN32)
  find_package(CURL REQUIRED)
  if(TARGET CURL::libcurl)
    set(LIBCURL_LIBRARIES CURL::libcurl)
  endif()
  set(HAVE_CURL 1)
else()
  if(PkgConfig_FOUND)
    pkg_check_modules(LIBCURL REQUIRED libcurl)
    set(HAVE_CURL 1)
  else()
    message(FATAL_ERROR "libcurl detection on this platform requires pkg-config.")
  endif()
endif()

if(WIN32)
  set(UGET_CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
  set(UGET_CURL_LIBRARY_DIRS "")
  set(UGET_CURL_LIBRARIES ${CURL_LIBRARIES})
  set(UGET_CURL_CFLAGS_OTHER "")
else()
  set(UGET_CURL_INCLUDE_DIRS ${LIBCURL_INCLUDE_DIRS})
  set(UGET_CURL_LIBRARY_DIRS ${LIBCURL_LIBRARY_DIRS})
  set(UGET_CURL_LIBRARIES ${LIBCURL_LIBRARIES})
  set(UGET_CURL_CFLAGS_OTHER ${LIBCURL_CFLAGS_OTHER})
endif()

if(NOT DEFINED CURL_LIBRARIES OR CURL_LIBRARIES STREQUAL "")
  set(CURL_LIBRARIES ${UGET_CURL_LIBRARIES})
endif()
if(NOT DEFINED CURL_INCLUDE_DIRS OR CURL_INCLUDE_DIRS STREQUAL "")
  set(CURL_INCLUDE_DIRS ${UGET_CURL_INCLUDE_DIRS})
endif()

list(APPEND _uget_common_include_dirs ${UGET_CURL_INCLUDE_DIRS})
list(APPEND _uget_common_library_dirs ${UGET_CURL_LIBRARY_DIRS})
list(APPEND _uget_common_libraries ${UGET_CURL_LIBRARIES})
list(APPEND _uget_common_compile_options ${UGET_CURL_CFLAGS_OTHER})

find_package(Threads REQUIRED)

include(CTest)
if(NOT BUILD_TESTING)
  message(STATUS "BUILD_TESTING=OFF: test targets will not be generated")
endif()

set(HAVE_TRAY 0)
set(HAVE_AYATANA 0)
set(HAVE_APPINDICATOR 0)
set(HAVE_PWMD 0)
set(HAVE_GNUTLS 0)
set(HAVE_OPENSSL 0)
set(HAVE_LIBGCRYPT 0)

# ---------- TRAY (Ayatana/AppIndicator) ----------
if(WITH_GUI AND WITH_TRAY)
  if(PkgConfig_FOUND)
    pkg_check_modules(AYATANA QUIET ayatana-appindicator3-0.1)
    if(AYATANA_FOUND)
      add_compile_definitions(HAVE_AYATANA=1)
      list(APPEND _uget_gui_include_dirs ${AYATANA_INCLUDE_DIRS})
      list(APPEND _uget_gui_library_dirs ${AYATANA_LIBRARY_DIRS})
      list(APPEND _uget_gui_libraries ${AYATANA_LIBRARIES})
      set(HAVE_TRAY 1)
      set(HAVE_AYATANA 1)
      message(STATUS "Tray: using ayatana-appindicator3")
    else()
      pkg_check_modules(APPINDICATOR QUIET appindicator3-0.1)
      if(APPINDICATOR_FOUND)
        add_compile_definitions(HAVE_APPINDICATOR=1)
        list(APPEND _uget_gui_include_dirs ${APPINDICATOR_INCLUDE_DIRS})
        list(APPEND _uget_gui_library_dirs ${APPINDICATOR_LIBRARY_DIRS})
        list(APPEND _uget_gui_libraries ${APPINDICATOR_LIBRARIES})
        set(HAVE_TRAY 1)
        set(HAVE_APPINDICATOR 1)
        message(STATUS "Tray: using appindicator3-0.1 (fallback)")
      else()
        message(STATUS "Tray requested, but no Ayatana/AppIndicator found -> tray disabled")
        set(WITH_TRAY OFF CACHE BOOL "" FORCE)
      endif()
    endif()
  else()
    message(FATAL_ERROR "WITH_TRAY=ON requires pkg-config.")
  endif()
elseif(WITH_TRAY)
  message(STATUS "WITH_TRAY requested but WITH_GUI=OFF -> disabling tray support")
  set(WITH_TRAY OFF CACHE BOOL "" FORCE)
endif()

# ---------- PWMD ----------
if(WITH_PWMD)
  if(PkgConfig_FOUND)
    pkg_check_modules(PWMD QUIET libpwmd)
    if(PWMD_FOUND)
      list(APPEND _uget_common_include_dirs ${PWMD_INCLUDE_DIRS})
      list(APPEND _uget_common_library_dirs ${PWMD_LIBRARY_DIRS})
      list(APPEND _uget_common_libraries ${PWMD_LIBRARIES})
      set(HAVE_PWMD 1)
      message(STATUS "PWMD: enabled")
    else()
      message(STATUS "PWMD: not found; disabling")
    endif()
  else()
    message(FATAL_ERROR "WITH_PWMD=ON requires pkg-config.")
  endif()
endif()

# ---------- Crypto/TLS backends ----------
if(WITH_OPENSSL)
  find_package(OpenSSL REQUIRED)
  add_compile_definitions(HAVE_OPENSSL=1)
  list(APPEND _uget_common_libraries OpenSSL::SSL OpenSSL::Crypto)
  set(HAVE_OPENSSL 1)
endif()

if(PkgConfig_FOUND)
  # Optional crypto stacks used by the Mega plug-in and alternative TLS backends
  pkg_check_modules(GNUTLS QUIET gnutls)
  pkg_check_modules(GCRYPT QUIET gcrypt)
endif()

if(NOT DEFINED GNUTLS_FOUND)
  set(GNUTLS_FOUND 0)
endif()
if(NOT DEFINED GCRYPT_FOUND)
  set(GCRYPT_FOUND 0)
endif()

set(HAVE_GNUTLS ${GNUTLS_FOUND})
set(HAVE_LIBGCRYPT ${GCRYPT_FOUND})

if(OPENSSL_FOUND)
  set(HAVE_OPENSSL 1)
endif()

if(WITH_GNUTLS)
  if(PkgConfig_FOUND)
    if(NOT GNUTLS_FOUND)
      message(FATAL_ERROR "WITH_GNUTLS=ON but gnutls was not found")
    endif()
    if(NOT GCRYPT_FOUND)
      message(FATAL_ERROR "WITH_GNUTLS=ON but libgcrypt was not found")
    endif()
    add_compile_definitions(HAVE_GNUTLS=1 HAVE_LIBGCRYPT=1)
    set(HAVE_GNUTLS 1)
    set(HAVE_LIBGCRYPT 1)
  else()
    message(FATAL_ERROR "WITH_GNUTLS=ON requires pkg-config.")
  endif()
else()
  # Treat GnuTLS as unavailable if the feature is explicitly disabled
  set(HAVE_GNUTLS 0)
endif()

if(HAVE_LIBGCRYPT)
  add_compile_definitions(HAVE_LIBGCRYPT=1)
endif()

# ---- Libraries and executables ----
set(UGLIB_SOURCES
  uglib/UgArray.c
  uglib/UgBuffer.c
  uglib/UgData.c
  uglib/UgEntry.c
  uglib/UgFileUtil.c
  uglib/UgHtml.c
  uglib/UgHtmlEntry.c
  uglib/UgHtmlFilter.c
  uglib/UgInfo.c
  uglib/UgJson-custom.c
  uglib/UgJson.c
  uglib/UgJsonFile.c
  uglib/UgJsonrpc.c
  uglib/UgJsonrpcCurl.c
  uglib/UgJsonrpcSocket.c
  uglib/UgList.c
  uglib/UgNode.c
  uglib/UgOption.c
  uglib/UgRegistry.c
  uglib/UgSocket.c
  uglib/UgSLink.c
  uglib/UgStdio.c
  uglib/UgString.c
  uglib/UgThread.c
  uglib/UgUri.c
  uglib/UgUtil.c
  uglib/UgValue.c
)

add_library(uglib STATIC ${UGLIB_SOURCES})
target_include_directories(uglib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/uglib
    ${CMAKE_BINARY_DIR}/generated)
target_compile_definitions(uglib PUBLIC HAVE_CONFIG_H)
target_link_libraries(uglib PUBLIC Threads::Threads)

set(UGET_CORE_SOURCES
  uget/UgetA2cf.c
  uget/UgetApp.c
  uget/UgetAria2.c
  uget/UgetCurl.c
  uget/UgetData.c
  uget/UgetEvent.c
  uget/UgetFiles.c
  uget/UgetHash.c
  uget/UgetMedia-youtube.c
  uget/UgetMedia.c
  uget/UgetNode-compare.c
  uget/UgetNode-filter.c
  uget/UgetNode.c
  uget/UgetOption.c
  uget/UgetPlugin.c
  uget/UgetPluginAgent.c
  uget/UgetPluginAria2.c
  uget/UgetPluginCurl.c
  uget/UgetPluginEmpty.c
  uget/UgetPluginMedia.c
  uget/UgetRpc.c
  uget/UgetRss.c
  uget/UgetSequence.c
  uget/UgetSite.c
  uget/UgetTask.c
)

if(HAVE_PWMD)
  list(APPEND UGET_CORE_SOURCES uget/pwmd.c)
endif()

add_library(uget_core STATIC ${UGET_CORE_SOURCES})
target_include_directories(uget_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/uget
    ${CMAKE_CURRENT_SOURCE_DIR}/uglib
    ${CMAKE_BINARY_DIR}/generated)
target_compile_definitions(uget_core PUBLIC HAVE_CONFIG_H)
target_link_libraries(uget_core PUBLIC uglib Threads::Threads)

if(HAVE_GNUTLS)
  target_include_directories(uget_core PRIVATE ${GNUTLS_INCLUDE_DIRS})
  target_link_libraries(uget_core PUBLIC ${GNUTLS_LIBRARIES})
  target_compile_definitions(uget_core PRIVATE HAVE_GNUTLS=1)
endif()

if(HAVE_LIBGCRYPT)
  target_include_directories(uget_core PRIVATE ${GCRYPT_INCLUDE_DIRS})
  target_link_libraries(uget_core PUBLIC ${GCRYPT_LIBRARIES})
  target_compile_definitions(uget_core PRIVATE HAVE_LIBGCRYPT=1)
endif()

# Mega requires (OpenSSL OR GnuTLS) AND libgcrypt
if((HAVE_OPENSSL OR HAVE_GNUTLS) AND HAVE_LIBGCRYPT)
  target_sources(uget_core PRIVATE uget/UgetPluginMega.c)
  target_compile_definitions(uget_core PRIVATE HAVE_MEGA=1)
  message(STATUS "Mega: enabled")
else()
  message(STATUS "Mega: disabled (needs OpenSSL or GnuTLS AND libgcrypt)")
endif()

# --- CLI entrypoint discovery
set(UGET_CLI_CANDIDATES
  ${CMAKE_SOURCE_DIR}/uget/cli/uget.c
  ${CMAKE_SOURCE_DIR}/uget/uget.c
  ${CMAKE_SOURCE_DIR}/uget/main.c
  ${CMAKE_SOURCE_DIR}/src/uget.c
)

set(UGET_CLI_MAIN "")
foreach(_c ${UGET_CLI_CANDIDATES})
  if(EXISTS "${_c}")
    set(UGET_CLI_MAIN "${_c}")
    break()
  endif()
endforeach()

option(UGET_CLI_STUB "Generate a minimal CLI main() if no real CLI is found" OFF)

if(UGET_CLI_MAIN)
  message(STATUS "uget CLI: using ${UGET_CLI_MAIN}")
  add_executable(uget ${UGET_CLI_MAIN})
elseif(UGET_CLI_STUB)
  message(WARNING "uget CLI: real entrypoint not found; generating stub (prints version and exits 0)")
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
  file(WRITE ${CMAKE_BINARY_DIR}/generated/uget_stub.c
"#include <stdio.h>\n"
"int main(int argc, char** argv){\n"
"  (void)argc; (void)argv;\n"
"  printf(\"uget (stub) built with CMake %s\\n\", \"${CMAKE_VERSION}\");\n"
"  return 0;\n"
"}\n")
  add_executable(uget ${CMAKE_BINARY_DIR}/generated/uget_stub.c)
else()
  message(STATUS "uget CLI: no entrypoint found; skipping CLI build")
endif()

if(TARGET uget)
  target_link_libraries(uget PRIVATE uget_core uglib Threads::Threads ${CURL_LIBRARIES})
  target_include_directories(uget PRIVATE
    ${CMAKE_SOURCE_DIR}/uget
    ${CMAKE_SOURCE_DIR}/uglib
    ${CMAKE_BINARY_DIR}/generated
    ${CURL_INCLUDE_DIRS}
  )
  target_compile_definitions(uget PRIVATE
    $<TARGET_PROPERTY:uget_core,INTERFACE_COMPILE_DEFINITIONS>
  )
  install(TARGETS uget RUNTIME DESTINATION bin)
endif()

if(WITH_GUI)
  file(GLOB UGET_GTK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui-gtk/*.c")
  if(UGET_GTK_SOURCES)
    add_executable(uget-gtk ${UGET_GTK_SOURCES})
    target_link_libraries(uget-gtk PRIVATE uget_core uglib)
    target_include_directories(uget-gtk
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ui-gtk
        ${CMAKE_CURRENT_SOURCE_DIR}/uget
        ${CMAKE_CURRENT_SOURCE_DIR}/uglib
        ${CMAKE_BINARY_DIR}/generated)
    target_compile_definitions(uget-gtk PRIVATE HAVE_CONFIG_H)
  else()
    message(WARNING "WITH_GUI=ON but no GTK sources were found in ui-gtk/")
  endif()
endif()

set(_uget_targets uglib uget_core)
if(TARGET uget)
  list(APPEND _uget_targets uget)
endif()
if(TARGET uget-gtk)
  list(APPEND _uget_targets uget-gtk)
endif()

foreach(_target IN LISTS _uget_targets)
  if(TARGET ${_target})
    if(_uget_common_compile_options)
      target_compile_options(${_target} PRIVATE ${_uget_common_compile_options})
    endif()
    if(_uget_common_include_dirs)
      target_include_directories(${_target} PRIVATE ${_uget_common_include_dirs})
    endif()
    if(_uget_common_library_dirs)
      target_link_directories(${_target} PRIVATE ${_uget_common_library_dirs})
    endif()
  endif()
endforeach()

if(_uget_common_libraries)
  if(TARGET uglib)
    target_link_libraries(uglib PUBLIC ${_uget_common_libraries})
  endif()
  if(TARGET uget_core)
    target_link_libraries(uget_core PUBLIC ${_uget_common_libraries})
  endif()
endif()

if(TARGET uget-gtk)
  if(_uget_gui_compile_options)
    target_compile_options(uget-gtk PRIVATE ${_uget_gui_compile_options})
  endif()
  if(_uget_gui_include_dirs)
    target_include_directories(uget-gtk PRIVATE ${_uget_gui_include_dirs})
  endif()
  if(_uget_gui_library_dirs)
    target_link_directories(uget-gtk PRIVATE ${_uget_gui_library_dirs})
  endif()
  if(_uget_gui_libraries)
    target_link_libraries(uget-gtk PRIVATE ${_uget_gui_libraries})
  endif()
endif()

# --- Project-wide compile warnings (already set globally, but ensure targets inherit) ---
foreach(_target IN LISTS _uget_targets)
  if(TARGET ${_target})
    if(MSVC)
      target_compile_options(${_target} PRIVATE /W4 /permissive- /utf-8)
    else()
      target_compile_options(${_target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
  endif()
endforeach()

# --- Sanitizers: only in Debug, only on non-MSVC ---
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
  foreach(_target IN LISTS _uget_targets)
    if(TARGET ${_target})
      target_compile_options(${_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${_target} PRIVATE -fsanitize=address,undefined)
    endif()
  endforeach()
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in
               ${CMAKE_BINARY_DIR}/generated/config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/generated)


if(BUILD_TESTING)
  if(WITH_GUI)
    add_subdirectory(tests)
  else()
    message(STATUS "BUILD_TESTING=ON but WITH_GUI=OFF -> skipping GLib/GTK tests on this platform")
  endif()
endif()

if(TARGET uget-gtk)
  install(TARGETS uget-gtk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
  install(DIRECTORY data/ DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/uget)
endif()
