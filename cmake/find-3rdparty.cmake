# [Ref] A pretty version of all Boost modules
# https://stackoverflow.com/questions/29989512/where-can-i-find-the-list-of-boost-component-that-i-can-use-in-cmake
# find_package(Boost REQUIRED COMPONENTS ALL)
# find_package(Boost REQUIRED)

# Needs CMake 3.14 or above
include(FetchContent)
# -------------------------------------------------------------------
# fmt
message(NOTICE "Fetching fmt...")
set(FMT_TEST OFF CACHE BOOL "")
set(FMT_INSTALL OFF CACHE BOOL "")
set(FMT_HEADERS "")
fetchcontent_declare(
    fmt
    SYSTEM
    URL https://github.com/fmtlib/fmt/archive/refs/tags/9.1.0.tar.gz
    DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/downloads
    DOWNLOAD_NAME fmt-v9.1.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fmt
    URL_HASH MD5=21fac48cae8f3b4a5783ae06b443973a
    QUIET)
# -------------------------------------------------------------------
# CLI11
message(NOTICE "Fetching CLI11...")
FetchContent_Declare(
    CLI11
    SYSTEM
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/CLI11
    GIT_TAG v2.3.2
    # GIT_TAG main
    GIT_SHALLOW ON)
# -------------------------------------------------------------------
# plog
message(NOTICE "Fetching plog...")
fetchcontent_declare(
    plog
    SYSTEM
    URL https://github.com/SergiusTheBest/plog/archive/refs/tags/1.1.10.tar.gz
    DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/downloads
    DOWNLOAD_NAME plog-1.1.10.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/plog
    URL_HASH MD5=6a1563fd892146e5a40c3cdc854600ed
    QUIET)
# -------------------------------------------------------------------
# nlohmann::json
message(NOTICE "Fetching nlohmann::json...")
FetchContent_Declare(
    nlohmann_json
    SYSTEM
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/downloads
    DOWNLOAD_NAME json-3.11.3.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json
    QUIET)
# -------------------------------------------------------------------
# GoogleTest
message(NOTICE "Fetching gtest...")
fetchcontent_declare(
    gtest
    SYSTEM
    URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
    DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/downloads
    DOWNLOAD_NAME googletest-1.16.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/googletest
    # URL_HASH MD5=
    QUIET)

fetchcontent_makeavailable(fmt CLI11 plog nlohmann_json gtest)

find_package(TCL REQUIRED)

option(HDL_USE_READLINE "Enable GNU Readline for the Tcl console" ON)
if (HDL_USE_READLINE)
#   find_package(Readline) # This doesn't work
  find_library(READLINE_LIBRARY readline)
  find_path(READLINE_INCLUDE_DIR readline/readline.h)
endif()
