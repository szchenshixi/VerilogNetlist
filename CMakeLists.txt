cmake_minimum_required(VERSION 3.15)
project(min_hdl_netlist CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# ---- GoogleTest for unit tests ----
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(hdl_tests
    test/test_hdl.cpp
)
target_link_libraries(hdl_tests PRIVATE hdl gtest_main)
include(GoogleTest)
gtest_discover_tests(hdl_tests)
