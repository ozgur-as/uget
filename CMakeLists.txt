cmake_minimum_required(VERSION 3.28)
project(ModernizedApp VERSION 2.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
include(CheckIncludeFile)

# pkg-config deps
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK   REQUIRED gtk+-3.0>=3.24)
pkg_check_modules(GLIB  REQUIRED glib-2.0>=2.76 gobject-2.0)
pkg_check_modules(GST   REQUIRED gstreamer-1.0 gstreamer-base-1.0)
pkg_check_modules(CURL  REQUIRED libcurl)

set(HAVE_GTK ${GTK_FOUND})
set(HAVE_GSTREAMER ${GST_FOUND})
set(HAVE_CURL ${CURL_FOUND})
set(HAVE_TRAY_AYATANA 0)

# Config header
configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in
               ${CMAKE_BINARY_DIR}/generated/config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/generated)

# TEMP: discover sources (to be replaced with explicit lists later)
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS src/*.c)
add_library(core STATIC ${CORE_SRC})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(core PRIVATE ${GTK_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER} ${GST_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
target_include_directories(core PRIVATE ${GTK_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_directories(core PRIVATE ${GTK_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS} ${GST_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})

add_executable(app src/main.c) # adjust if main file differs
target_link_libraries(app PRIVATE core
  ${GTK_LIBRARIES} ${GLIB_LIBRARIES} ${GST_LIBRARIES} ${CURL_LIBRARIES})
target_compile_options(app PRIVATE ${GTK_CFLAGS_OTHER} ${GLIB_CFLAGS_OTHER} ${GST_CFLAGS_OTHER} ${CURL_CFLAGS_OTHER})
target_include_directories(app PRIVATE ${GTK_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_directories(app PRIVATE ${GTK_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS} ${GST_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})

enable_testing()

install(TARGETS app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
