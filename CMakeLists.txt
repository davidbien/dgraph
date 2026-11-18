# CMakeLists.txt for dgraph.
# dgraph is a header-only library: this defines an interface library for it and builds its unit tests.
# It may be built standalone or added to another project with add_subdirectory().
#
# dgraph requires the bienutil headers: set DGRAPH_BIENUTIL_DIR to where they are found if they are not in ../bienutil.
# Set DGRAPH_BUILD_TESTS to OFF to not build the unit tests.

cmake_minimum_required (VERSION 3.14)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(DGRAPH_STANDALONE TRUE)
  project ("dgraph" CXX)
else()
  set(DGRAPH_STANDALONE FALSE)
endif()

set(DGRAPH_BIENUTIL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bienutil" CACHE PATH "Directory containing the bienutil headers.")
if(NOT EXISTS "${DGRAPH_BIENUTIL_DIR}/bienutil.h")
  message(FATAL_ERROR "dgraph: bienutil headers not found in [${DGRAPH_BIENUTIL_DIR}] - set DGRAPH_BIENUTIL_DIR.")
endif()

if(DGRAPH_STANDALONE)
  # When part of another project we expect it to have set things up - bienutil.cmake modifies global compiler flags.
  include ( "${DGRAPH_BIENUTIL_DIR}/bienutil.cmake" )
  enable_testing()
  bienutil_ctest_log_dir( "${CMAKE_BINARY_DIR}/testlogs" ) # The test programs' logs go here - cleared each ctest run.
endif()

add_library(dgraph INTERFACE)
target_include_directories(dgraph INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${DGRAPH_BIENUTIL_DIR}>
)

option(DGRAPH_BUILD_TESTS "Build the dgraph unit tests." ON)
if(DGRAPH_BUILD_TESTS)
  add_subdirectory(test)
endif()
