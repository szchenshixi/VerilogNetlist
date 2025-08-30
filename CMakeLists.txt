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

include(cmake/find-3rdparty.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(hdl STATIC
    src/ast/expr.cpp
    src/net/connectivity.cpp
    src/net/bitmap.cpp
    src/elab/spec.cpp
    src/elab/flatten.cpp
    src/elab/elaborate.cpp
    src/hier/instance.cpp
    src/util/id_string.cpp
)

add_executable(hdl_demo
    src/demo/main.cpp
)
target_link_libraries(hdl_demo PRIVATE hdl)

enable_testing()
add_executable(hdl_tests
    test/test_hdl.cpp
)
target_link_libraries(hdl_tests PRIVATE hdl gtest_main)
include(GoogleTest)
gtest_discover_tests(hdl_tests)
