cmake_minimum_required(VERSION 3.24)
project(ModernizedApp VERSION 2.3.0 LANGUAGES C)

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

set(_uget_private_include_dirs)
set(_uget_private_library_dirs)
set(_uget_private_libraries)
set(_uget_private_compile_options)

if(WITH_GUI)
  if(PkgConfig_FOUND)
    pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.24)
    pkg_check_modules(GLIB REQUIRED glib-2.0>=2.76 gobject-2.0)
    pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0)
    add_compile_definitions(HAVE_GLIB=1)

    set(HAVE_GTK 1)
    set(HAVE_GSTREAMER 1)
    set(HAVE_GLIB 1)

    list(APPEND _uget_private_include_dirs
      ${GTK_INCLUDE_DIRS}
      ${GLIB_INCLUDE_DIRS}
      ${GST_INCLUDE_DIRS})
    list(APPEND _uget_private_library_dirs
      ${GTK_LIBRARY_DIRS}
      ${GLIB_LIBRARY_DIRS}
      ${GST_LIBRARY_DIRS})
    list(APPEND _uget_private_libraries
      ${GTK_LIBRARIES}
      ${GLIB_LIBRARIES}
      ${GST_LIBRARIES})
    list(APPEND _uget_private_compile_options
      ${GTK_CFLAGS_OTHER}
      ${GLIB_CFLAGS_OTHER}
      ${GST_CFLAGS_OTHER})
  else()
    message(FATAL_ERROR "WITH_GUI=ON but pkg-config is unavailable on this toolchain.")
  endif()
else()
  message(STATUS "WITH_GUI=OFF: skipping GTK/GLib/GStreamer detection")
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

list(APPEND _uget_private_include_dirs ${UGET_CURL_INCLUDE_DIRS})
list(APPEND _uget_private_library_dirs ${UGET_CURL_LIBRARY_DIRS})
list(APPEND _uget_private_libraries ${UGET_CURL_LIBRARIES})
list(APPEND _uget_private_compile_options ${UGET_CURL_CFLAGS_OTHER})

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
      list(APPEND _uget_private_include_dirs ${AYATANA_INCLUDE_DIRS})
      list(APPEND _uget_private_library_dirs ${AYATANA_LIBRARY_DIRS})
      list(APPEND _uget_private_libraries ${AYATANA_LIBRARIES})
      set(HAVE_TRAY 1)
      set(HAVE_AYATANA 1)
      message(STATUS "Tray: using ayatana-appindicator3")
    else()
      pkg_check_modules(APPINDICATOR QUIET appindicator3-0.1)
      if(APPINDICATOR_FOUND)
        add_compile_definitions(HAVE_APPINDICATOR=1)
        list(APPEND _uget_private_include_dirs ${APPINDICATOR_INCLUDE_DIRS})
        list(APPEND _uget_private_library_dirs ${APPINDICATOR_LIBRARY_DIRS})
        list(APPEND _uget_private_libraries ${APPINDICATOR_LIBRARIES})
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
      list(APPEND _uget_private_include_dirs ${PWMD_INCLUDE_DIRS})
      list(APPEND _uget_private_library_dirs ${PWMD_LIBRARY_DIRS})
      list(APPEND _uget_private_libraries ${PWMD_LIBRARIES})
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
  list(APPEND _uget_private_libraries OpenSSL::SSL OpenSSL::Crypto)
  set(HAVE_OPENSSL 1)
endif()

if(WITH_GNUTLS)
  if(PkgConfig_FOUND)
    pkg_check_modules(GNUTLS REQUIRED gnutls)
    pkg_check_modules(GCRYPT REQUIRED libgcrypt)
    add_compile_definitions(HAVE_GNUTLS=1 HAVE_LIBGCRYPT=1)
    set(HAVE_GNUTLS 1)
    set(HAVE_LIBGCRYPT 1)

    list(APPEND _uget_private_include_dirs ${GNUTLS_INCLUDE_DIRS} ${GCRYPT_INCLUDE_DIRS})
    list(APPEND _uget_private_library_dirs ${GNUTLS_LIBRARY_DIRS} ${GCRYPT_LIBRARY_DIRS})
    list(APPEND _uget_private_libraries ${GNUTLS_LIBRARIES} ${GCRYPT_LIBRARIES})
  else()
    message(FATAL_ERROR "WITH_GNUTLS=ON requires pkg-config.")
  endif()
endif()

# TEMP: discover sources (to be replaced with explicit lists later)
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS src/*.c)
add_library(core STATIC ${CORE_SRC})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(core PRIVATE HAVE_CONFIG_H)
target_include_directories(core PRIVATE ${CMAKE_BINARY_DIR}/generated)

if(_uget_private_compile_options)
  target_compile_options(core PRIVATE ${_uget_private_compile_options})
endif()
if(_uget_private_include_dirs)
  target_include_directories(core PRIVATE ${_uget_private_include_dirs})
endif()
if(_uget_private_library_dirs)
  target_link_directories(core PRIVATE ${_uget_private_library_dirs})
endif()
if(_uget_private_libraries)
  target_link_libraries(core PRIVATE ${_uget_private_libraries})
endif()

add_executable(app src/main.c) # adjust if main file differs
target_link_libraries(app PRIVATE core)

if(_uget_private_compile_options)
  target_compile_options(app PRIVATE ${_uget_private_compile_options})
endif()
if(_uget_private_include_dirs)
  target_include_directories(app PRIVATE ${_uget_private_include_dirs})
endif()
if(_uget_private_library_dirs)
  target_link_directories(app PRIVATE ${_uget_private_library_dirs})
endif()
if(_uget_private_libraries)
  target_link_libraries(app PRIVATE ${_uget_private_libraries})
endif()

# --- Project-wide compile warnings (already set globally, but ensure targets inherit) ---
if(MSVC)
  target_compile_options(core PRIVATE /W4 /permissive- /utf-8)
  target_compile_options(app  PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(app  PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Sanitizers: only in Debug, only on non-MSVC ---
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
  target_compile_options(core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(  core PRIVATE -fsanitize=address,undefined)

  target_compile_options(app  PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(  app  PRIVATE -fsanitize=address,undefined)
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

install(TARGETS app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
