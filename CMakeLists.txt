cmake_minimum_required(VERSION 3.20)
project(Peano)

#Inlcude fetch content to fetch and initialize project options
include(FetchContent)
FetchContent_Declare(
    _project_options
    GIT_REPOSITORY https://github.com/aminya/project_options.git
    GIT_TAG main
)
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)
include(${_project_options_SOURCE_DIR}/Index.cmake)

# C++ Standard, some feature of C++20 is required within the project
set(CMAKE_CXX_STANDARD 20)
# Not sure if they are mandatory
set(CMAKE_CXX_STANDARD_REQUIRED 1)
# Abide to the c++ stanadard solely
set(CMAKE_CXX_EXTENSIONS OFF)
# Generate compile_commands.json to make it easier to work with Clang based
# tools.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake")
add_subdirectory(CMake) 

include(CTest)
include(CompileAndLinkFlags)
include(ParseUserOptions)
include(CheckLanguage)
include(HeaderLookup)
include(GNUInstallDirs)
include(TestBigEndian)
include(ProcessorCount)
include(CheckIPOSupported) # Check and enable IPO Snipped from: https://stackoverflow.com/questions/31355692/how-do-i-enable-link-time-optimization-lto-with-cmake
include(ProjectInformation)
include(GPUOptions)

set(LANGUAGES C CXX)
check_language(Fortran) 
if(CMAKE_Fortran_COMPILER) 
  list(APPEND LANGUAGES Fortran) 
  enable_language(Fortran) 
endif()

if(WITH_MULTITHREADING STREQUAL "omp")
  find_package(OpenMP REQUIRED)
elseif(WITH_MULTITHREADING STREQUAL "tbb")
  find_package(TBB REQUIRED)
elseif(WITH_MULTITHREADING STREQUAL "cpp")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

if(WITH_MPI)
  find_package(MPI REQUIRED)
  include_directories(SYSTEM ${MPI_INCLUDE_PATH})
  add_compile_definitions(OMPI_SKIP_MPICXX) # To prevent compiler warnings
  add_compile_definitions(Parallel)
  set(Parallel 1)
endif()

if(ENABLE_EXAHYPE)
  if(NOT ENABLE_LOADBALANCING)
    set(ENABLE_LOADBALANCING ON)
    message(
      WARNING
        "ExaHype 2 requires the loadbalancing toolbox, therefore the user setting was overriden from OFF to ON"
    )
  endif()

  if(NOT ENABLE_BLOCKSTRUCTURED)
    set(ENABLE_BLOCKSTRUCTURED ON)
    message(
      WARNING
        "ExaHype 2 requires the blockstructured toolbox, therefore the user setting was overriden from OFF to ON"
    )
  endif()

  set(UseExaHyPE 1)
endif()

if(WITH_OTTER)
  find_package(OTF2 REQUIRED)
  find_package(Otter REQUIRED)
  set(UseOtter 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/cmake.config.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)

check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(IPO_SUPPORTED)
  message(STATUS "IPO / LTO enabled")
  set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION 1)
else()
  message(STATUS "IPO / LTO not supported: ${IPO_ERROR}")
endif()

set(DEFAULT_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

test_big_endian(BIG_ENDIAN)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)

dynamic_project_options()

add_subdirectory(src)

file(COPY "${PROJECT_SOURCE_DIR}/applications" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${PROJECT_SOURCE_DIR}/benchmarks" DESTINATION "${CMAKE_BINARY_DIR}")

set(BUILD_DIRECTORY "${CMAKE_BINARY_DIR}" CACHE STRING "${CMAKE_BINARY_DIR}" FORCE)

file(TOUCH ${CMAKE_BINARY_DIR}/GeneratedSubdirectories.cmake)
include(${CMAKE_BINARY_DIR}/GeneratedSubdirectories.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_BINARY_DIR}/GeneratedSubdirectories.cmake)