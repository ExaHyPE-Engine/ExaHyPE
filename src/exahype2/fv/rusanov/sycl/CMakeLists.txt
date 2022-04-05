cmake_minimum_required(VERSION 3.14)
project(reductionlibrary)

set(CMAKE_CXX_STANDARD 17)

# These need to be adjusted for the device you want to test
SET(SYCL_FLAGS "-fsycl -fsycl-targets=nvptx64-nvidia-cuda--sm_75 -Wno-error -g3 -DDEBUG")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${SYCL_FLAGS}")


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)

FetchContent_MakeAvailable(googletest)


enable_testing()

# Need this to get rid of Werror due to the target triplet stuff --- at least for now, also requries to run cmake twice to 
# take effect :(
execute_process (
    COMMAND sed -i "s| -Werror||g" ./_deps/googletest-src/googletest/cmake/internal_utils.cmake
)

add_executable(
  test_reductionlibrary
  test/test_reductionlibrary.cxx
)

add_executable(
  test_solver
  test/test_solver.cxx
)

target_include_directories(
    test_reductionlibrary PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
)

target_include_directories(
    test_solver PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
)

target_link_libraries(
  test_reductionlibrary
  gtest_main
)

target_link_libraries(
  test_solver
  gtest_main
)



include(GoogleTest)
gtest_discover_tests(test_reductionlibrary)
