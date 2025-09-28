# [Ref] A pretty version of all Boost modules
# https://stackoverflow.com/questions/29989512/where-can-i-find-the-list-of-boost-component-that-i-can-use-in-cmake
# find_package(Boost REQUIRED COMPONENTS ALL)
# find_package(Boost REQUIRED)

# Needs CMake 3.14 or above
include(FetchContent)
# -------------------------------------------------------------------
# plog
message(NOTICE "Fetching plog...")
set(PLOG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/plog)
if(EXISTS ${PLOG_ROOT}/CMakeLists.txt)
    message(NOTICE "Using pre-downloaded Plog: ${PLOG_ROOT}")
    fetchcontent_declare(
        plog
        SYSTEM
        SOURCE_DIR ${PLOG_ROOT}
        DOWNLOAD_COMMAND "")
else()
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
endif()
# -------------------------------------------------------------------
# nlohmann::json
message(NOTICE "Fetching nlohmann::json...")
set(NLOHMANN_JSON_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/json)
if(EXISTS ${NLOHMANN_JSON_ROOT}/CMakeLists.txt)
    FetchContent_Declare(
        nlohmann_json
        SYSTEM
        SOURCE_DIR ${NLOHMANN_JSON_ROOT}
        DOWNLOAD_COMMAND "")
else()
    FetchContent_Declare(
        nlohmann_json
        SYSTEM
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/downloads
        DOWNLOAD_NAME json-3.11.3.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json
        QUIET)
endif()
# -------------------------------------------------------------------
# GoogleTest
message(NOTICE "Fetching gtest...")
set(GTEST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/googletest)
if(EXISTS ${GTEST_ROOT}/CMakeLists.txt)
    fetchcontent_declare(
        gtest
        SYSTEM
        SOURCE_DIR ${GTEST_ROOT}
        DOWNLOAD_COMMAND "")
else()
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
endif()

fetchcontent_makeavailable(plog nlohmann_json gtest)

find_package(TCL REQUIRED)

option(HDL_USE_READLINE "Enable GNU Readline for the Tcl console" ON)
if (HDL_USE_READLINE)
#   find_package(Readline) # This doesn't work
  find_library(READLINE_LIBRARY readline)
  find_path(READLINE_INCLUDE_DIR readline/readline.h)
endif()
