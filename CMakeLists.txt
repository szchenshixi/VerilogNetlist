cmake_minimum_required(VERSION 3.15)
project(
  min_hdl_netlist
  VERSION 0.1.0
  DESCRIPTION "Test repository for project utilities"
  HOMEPAGE_URL ""
  LANGUAGES C CXX)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/options.cmake)
include(cmake/stamp_build.cmake)
include(cmake/find-3rdparty.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ------------------------------------------------------------------------------
# hdl
add_library(
  hdl STATIC
  src/ast/expr.cpp
  src/net/connectivity.cpp
  src/net/bitmap.cpp
  src/elab/spec.cpp
  src/elab/flatten.cpp
  src/elab/elaborate.cpp
  src/hier/instance.cpp
  src/vis/json.cpp
  src/util/id_string.cpp)
target_link_libraries(hdl PUBLIC nlohmann_json::nlohmann_json)
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# hdl_demo
add_executable(hdl_demo src/demo/main.cpp)
target_link_libraries(hdl_demo PRIVATE hdl)
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# hdl_tcl
set(CMD_SOURCES
    src/tcl/cmd/register_all.cpp
    src/tcl/cmd/cmd_help.cpp
    src/tcl/cmd/cmd_specs.cpp
    src/tcl/cmd/cmd_invert.cpp
    src/tcl/cmd/cmd_elab.cpp
    src/tcl/cmd/cmd_select.cpp
    src/tcl/cmd/cmd_selection.cpp
    src/tcl/cmd/cmd_ports.cpp
    src/tcl/cmd/cmd_wires.cpp
    src/tcl/cmd/cmd_dump.cpp
    src/tcl/cmd/cmd_query.cpp
    src/tcl/cmd/cmd_undo.cpp
    src/tcl/cmd/cmd_history.cpp)
add_executable(hdl_tcl src/demo/tcl_console_main.cpp src/tcl/console.cpp
                       ${CMD_SOURCES})

# Automatically include user-defined commands, if any
file(GLOB USER_CMD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/tcl/cmd/user/*.cpp)
target_sources(hdl_tcl PRIVATE ${USER_CMD_SOURCES})

# Optional GNU Readline support (only if requested and found)
if(HDL_USE_READLINE
   AND READLINE_LIBRARY
   AND READLINE_INCLUDE_DIR)
  target_compile_definitions(hdl_tcl PRIVATE HDL_HAVE_READLINE=1)
  target_include_directories(hdl_tcl PRIVATE ${READLINE_INCLUDE_DIR})
  target_link_libraries(hdl_tcl PRIVATE ${READLINE_LIBRARY})
else()
  message(STATUS "Readline disabled or not found; building hdl_tcl without it")
endif()
target_include_directories(hdl_tcl PRIVATE ${TCL_INCLUDE_PATH}
                                           ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hdl_tcl PRIVATE hdl ${TCL_LIBRARY})
# ------------------------------------------------------------------------------

enable_testing()
add_executable(hdl_tests test/test_hdl.cpp)
target_link_libraries(hdl_tests PRIVATE hdl gtest_main)
include(GoogleTest)
gtest_discover_tests(hdl_tests)
