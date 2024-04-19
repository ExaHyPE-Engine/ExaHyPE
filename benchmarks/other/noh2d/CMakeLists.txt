cmake_minimum_required(VERSION 3.20)
project(noh2d)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_FLAGS_RELEASE "-g -Ofast -march=native -mtune=native -fno-pic")

find_package(OpenMP REQUIRED)
find_library(RT NAMES rt) # may be necessary to use AIO on some systems

file(GLOB_RECURSE CPP_FILES LIST_DIRECTORIES false ./noh2d-common/**/*.cpp)
add_executable(noh2d main.cpp ${CPP_FILES})

target_include_directories(noh2d PRIVATE "./../noh2d-common/spacetree" "./../noh2d-common/swift")
target_link_libraries(noh2d PRIVATE OpenMP::OpenMP_CXX ${RT})
