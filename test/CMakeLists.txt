# CMakeLists.txt for the dgraph unit tests.
#
# dgraphgtest: Google test based unit tests - see dgraphgtest.cpp.
# gr_test: Creates a random graph then copies, dumps, saves and loads it while throwing exceptions at random
#   from the debug throw points - see _gr_test.cpp.
# gr_test_nothrow: The same as gr_test but with the debug throw points compiled out.

include(GoogleTest)

if(NOT TARGET gtest)
  include(FetchContent)
  set(GTEST_VERSION 1.14.0 CACHE STRING "Google test version")
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v${GTEST_VERSION})
  # Use the same runtime library settings as we do under MSVC.
  set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable (dgraphgtest dgraphgtest.cpp )
target_link_libraries(dgraphgtest dgraph gtest)
target_compile_definitions( dgraphgtest
  PUBLIC __NDEBUG_THROW
  PUBLIC TRACESENABLED=0
  # Count allocations and constructions of graph nodes and links so that the tests can detect leaks.
  PUBLIC __DGRAPH_COUNT_EL_ALLOC_LIFETIME
)
gtest_discover_tests(dgraphgtest)

add_executable (gr_test _gr_test.cpp )
target_link_libraries(gr_test dgraph)
target_compile_definitions( gr_test
  PUBLIC TRACESENABLED=0
  PUBLIC __DGRAPH_COUNT_EL_ALLOC_LIFETIME
)

add_executable (gr_test_nothrow _gr_test.cpp )
target_link_libraries(gr_test_nothrow dgraph)
target_compile_definitions( gr_test_nothrow
  PUBLIC __NDEBUG_THROW
  PUBLIC TRACESENABLED=0
  PUBLIC __DGRAPH_COUNT_EL_ALLOC_LIFETIME
)

# Each run writes its output files into the current directory so give each test its own - this allows running in parallel.
# Arguments: <num nodes> <num extra links> <random seed>
function(dgraph_add_gr_test NUM_NODES NUM_EXTRA_LINKS RAND_SEED)
  foreach(GR_TEST_EXE gr_test gr_test_nothrow)
    set(GR_TEST_NAME "${GR_TEST_EXE}.${NUM_NODES}_${NUM_EXTRA_LINKS}_${RAND_SEED}")
    set(GR_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/${GR_TEST_NAME}")
    file(MAKE_DIRECTORY "${GR_TEST_DIR}")
    add_test(NAME ${GR_TEST_NAME}
      COMMAND ${GR_TEST_EXE} ${NUM_NODES} ${NUM_EXTRA_LINKS} ${RAND_SEED}
      WORKING_DIRECTORY "${GR_TEST_DIR}")
  endforeach()
endfunction()

dgraph_add_gr_test(1 0 5)
dgraph_add_gr_test(2 3 7)
dgraph_add_gr_test(10 10 1000)
dgraph_add_gr_test(50 50 1241)
dgraph_add_gr_test(200 300 77)
dgraph_add_gr_test(1000 2000 1241)
