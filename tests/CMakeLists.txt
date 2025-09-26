cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0079 NEW)

if(NOT WITH_GUI)
  message(STATUS "tests/: WITH_GUI=OFF -> not building legacy GLib/GTK tests")
  return()
endif()

add_library(uglib_alias ALIAS uglib)
add_library(uget_core_alias ALIAS uget_core)

set(_uget_has_mega FALSE)
if(HAVE_LIBGCRYPT AND HAVE_OPENSSL)
  set(_uget_has_mega TRUE)
else()
  message(STATUS "Skipping test_plugin_app (Mega plug-in not available)")
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
    WORKING_DIRECTORY "${_work_dir}"
    TIMEOUT 60)
endfunction()

add_uget_test(test_json
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-json.c
  LIBS uglib_alias
)

add_uget_test(test_info
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-info.c
  LIBS uglib_alias
)

add_uget_test(test_uglib
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-uglib.c
  LIBS uglib_alias
)

add_uget_test(test_uget
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-uget.c
  LIBS uget_core_alias uglib_alias
  LABELS "unit;net"
)

add_uget_test(test_jsonrpc
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-jsonrpc.c
  LIBS uget_core_alias uglib_alias
  LABELS "unit;net"
)

if(TEST test_http)
  set_tests_properties(test_http PROPERTIES LABELS "unit;net" TIMEOUT 60)
endif()

if(_uget_has_mega)
  add_uget_test(test_plugin_app
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test-plugin+app.c
    LIBS uget_core_alias uglib_alias
    LABELS "unit;net"
  )
endif()
