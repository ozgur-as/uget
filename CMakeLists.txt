cmake_minimum_required(VERSION 3.28)
project(ModernizedApp VERSION 2.3.0 LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Verbose build" FORCE)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable ASan/UBSan in Debug on non-MSVC builds" ON)
option(WITH_GUI "Enable GTK-based GUI components" ON)

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
set(HAVE_GTK 0)
set(HAVE_GSTREAMER 0)
set(HAVE_GLIB 0)

if(WITH_GUI)
  if(NOT DEFINED PkgConfig_FOUND OR NOT PkgConfig_FOUND)
    find_package(PkgConfig REQUIRED)
  endif()
  pkg_check_modules(GTK  REQUIRED gtk+-3.0>=3.24)
  pkg_check_modules(GLIB REQUIRED glib-2.0>=2.76 gobject-2.0)
  pkg_check_modules(GST  REQUIRED gstreamer-1.0 gstreamer-base-1.0)

  set(HAVE_GTK ${GTK_FOUND})
  set(HAVE_GSTREAMER ${GST_FOUND})
  set(HAVE_GLIB ${GLIB_FOUND})
else()
  message(STATUS "WITH_GUI=OFF: skipping GTK/GLib/GStreamer detection")
endif()

if(NOT DEFINED PkgConfig_FOUND OR NOT PkgConfig_FOUND)
  find_package(PkgConfig REQUIRED)
endif()
pkg_check_modules(CURL REQUIRED libcurl)
find_package(Threads REQUIRED)

include(CTest)
if(NOT BUILD_TESTING)
  message(STATUS "BUILD_TESTING=OFF: test targets will not be generated")
endif()

set(HAVE_CURL ${CURL_FOUND})

# ---------- Feature toggles (default ON; compile gracefully without them) ----------
option(WITH_TRAY     "Enable system tray indicator" ON)
option(WITH_PWMD     "Enable libpwmd integration"   ON)
option(WITH_GNUTLS   "Enable GnuTLS backend"        ON)
option(WITH_OPENSSL  "Enable OpenSSL backend"       ON)
option(WITH_GCRYPT   "Enable libgcrypt usage"       ON)

set(HAVE_TRAY 0)
set(HAVE_TRAY_AYATANA 0)
set(HAVE_TRAY_APPINDICATOR 0)
set(HAVE_PWMD 0)
set(HAVE_GNUTLS 0)
set(HAVE_OPENSSL 0)
set(HAVE_GCRYPT 0)

# TEMP: discover sources (to be replaced with explicit lists later)
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS src/*.c)
add_library(core STATIC ${CORE_SRC})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(core PRIVATE ${GTK_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER} ${GST_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
target_include_directories(core PRIVATE ${GTK_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_directories(core PRIVATE ${GTK_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS} ${GST_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_compile_definitions(core PRIVATE HAVE_CONFIG_H)
target_include_directories(core PRIVATE ${CMAKE_BINARY_DIR}/generated)

add_executable(app src/main.c) # adjust if main file differs
target_link_libraries(app PRIVATE core
  ${GTK_LIBRARIES} ${GLIB_LIBRARIES} ${GST_LIBRARIES} ${CURL_LIBRARIES})
target_compile_options(app PRIVATE ${GTK_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER} ${GST_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
target_include_directories(app PRIVATE ${GTK_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_directories(app PRIVATE ${GTK_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS} ${GST_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})

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

# ---------- TRAY (Ayatana/AppIndicator) ----------
if(WITH_TRAY)
  pkg_check_modules(AYATANA QUIET ayatana-appindicator3-0.1)
  if(AYATANA_FOUND)
    target_include_directories(core PRIVATE ${AYATANA_INCLUDE_DIRS})
    target_link_directories(core  PRIVATE ${AYATANA_LIBRARY_DIRS})
    target_link_libraries(core    PRIVATE ${AYATANA_LIBRARIES})
    target_compile_definitions(core PRIVATE HAVE_TRAY=1 HAVE_TRAY_AYATANA=1)
    set(HAVE_TRAY 1)
    set(HAVE_TRAY_AYATANA 1)
    message(STATUS "Tray: using ayatana-appindicator3")
  else()
    pkg_check_modules(APPIND QUIET appindicator3-0.1)
    if(APPIND_FOUND)
      target_include_directories(core PRIVATE ${APPIND_INCLUDE_DIRS})
      target_link_directories(core  PRIVATE ${APPIND_LIBRARY_DIRS})
      target_link_libraries(core    PRIVATE ${APPIND_LIBRARIES})
      target_compile_definitions(core PRIVATE HAVE_TRAY=1 HAVE_TRAY_APPINDICATOR=1)
      set(HAVE_TRAY 1)
      set(HAVE_TRAY_APPINDICATOR 1)
      message(STATUS "Tray: using appindicator3-0.1 (legacy)")
    else()
      message(STATUS "Tray: disabled (ayatana/appindicator not found)")
    endif()
  endif()
endif()

# ---------- PWMD ----------
if(WITH_PWMD)
  pkg_check_modules(PWMD QUIET libpwmd)
  if(PWMD_FOUND)
    target_include_directories(core PRIVATE ${PWMD_INCLUDE_DIRS})
    target_link_directories(core  PRIVATE ${PWMD_LIBRARY_DIRS})
    target_link_libraries(core    PRIVATE ${PWMD_LIBRARIES})
    target_compile_definitions(core PRIVATE HAVE_PWMD=1)
    set(HAVE_PWMD 1)
    message(STATUS "PWMD: enabled")
  else()
    message(STATUS "PWMD: not found; disabling")
  endif()
endif()

# ---------- Crypto/TLS backends ----------
if(WITH_GNUTLS)
  pkg_check_modules(GNUTLS QUIET gnutls)
  if(GNUTLS_FOUND)
    target_include_directories(core PRIVATE ${GNUTLS_INCLUDE_DIRS})
    target_link_directories(core PRIVATE ${GNUTLS_LIBRARY_DIRS})
    target_link_libraries(core PRIVATE ${GNUTLS_LIBRARIES})
    target_compile_definitions(core PRIVATE HAVE_GNUTLS=1)
    set(HAVE_GNUTLS 1)
    message(STATUS "TLS: GnuTLS enabled")
  else()
    message(STATUS "TLS: GnuTLS not found")
  endif()
endif()

if(WITH_OPENSSL)
  # Prefer 'openssl' pkg (provides crypto/ssl). On some distros it's 'openssl', on others split:
  pkg_check_modules(OPENSSL QUIET openssl)
  if(OPENSSL_FOUND)
    target_include_directories(core PRIVATE ${OPENSSL_INCLUDE_DIRS})
    target_link_directories(core PRIVATE ${OPENSSL_LIBRARY_DIRS})
    target_link_libraries(core PRIVATE ${OPENSSL_LIBRARIES})
    target_compile_definitions(core PRIVATE HAVE_OPENSSL=1)
    set(HAVE_OPENSSL 1)
    message(STATUS "TLS: OpenSSL enabled")
  else()
    # Fallback separate crypto/ssl (if available in your environment)
    pkg_check_modules(LIBSSL QUIET libssl)
    pkg_check_modules(LIBCRYPTO QUIET libcrypto)
    if(LIBSSL_FOUND AND LIBCRYPTO_FOUND)
      target_include_directories(core PRIVATE ${LIBSSL_INCLUDE_DIRS} ${LIBCRYPTO_INCLUDE_DIRS})
      target_link_directories(core PRIVATE ${LIBSSL_LIBRARY_DIRS} ${LIBCRYPTO_LIBRARY_DIRS})
      target_link_libraries(core PRIVATE ${LIBSSL_LIBRARIES} ${LIBCRYPTO_LIBRARIES})
      target_compile_definitions(core PRIVATE HAVE_OPENSSL=1)
      set(HAVE_OPENSSL 1)
      message(STATUS "TLS: OpenSSL (split ssl/crypto) enabled")
    else()
      message(STATUS "TLS: OpenSSL not found")
    endif()
  endif()
endif()

if(WITH_GCRYPT)
  pkg_check_modules(GCRYPT QUIET libgcrypt)
  if(GCRYPT_FOUND)
    target_include_directories(core PRIVATE ${GCRYPT_INCLUDE_DIRS})
    target_link_directories(core PRIVATE ${GCRYPT_LIBRARY_DIRS})
    target_link_libraries(core PRIVATE ${GCRYPT_LIBRARIES})
    target_compile_definitions(core PRIVATE HAVE_GCRYPT=1)
    set(HAVE_GCRYPT 1)
    message(STATUS "Crypto: libgcrypt enabled")
  else()
    message(STATUS "Crypto: libgcrypt not found")
  endif()
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in
               ${CMAKE_BINARY_DIR}/generated/config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/generated)


if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
