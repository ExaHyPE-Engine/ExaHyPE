# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

cmake_minimum_required(VERSION 3.27)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_BRANCH GIT_COMMIT_HASH)
get_git_commit_message(GIT_COMMIT_MESSAGE)
get_git_commit_timestamp(GIT_COMMIT_TIMESTAMP)

function(escape_for_cpp_string_literal INPUT OUTPUT)
  set(_escaped "${INPUT}")
  string(REPLACE "\\" "\\\\" _escaped "${_escaped}")
  string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
  string(REPLACE "\r" "" _escaped "${_escaped}")
  string(REPLACE "\n" "\\n" _escaped "${_escaped}")
  set(${OUTPUT} "${_escaped}" PARENT_SCOPE)
endfunction()

if(GIT_BRANCH MATCHES "^refs/heads/")
  string(REGEX REPLACE "^refs/heads/" "" GIT_BRANCH "${GIT_BRANCH}")
elseif(GIT_BRANCH MATCHES "^refs/remotes/")
  string(REGEX REPLACE "^refs/remotes/" "" GIT_BRANCH "${GIT_BRANCH}")
elseif(GIT_BRANCH MATCHES "^.git/")
  string(REGEX REPLACE "^.git/" "" GIT_BRANCH "${GIT_BRANCH}")
endif()
if(NOT GIT_BRANCH)
  if(DEFINED ENV{CI_MERGE_REQUEST_SOURCE_BRANCH_NAME} AND NOT "$ENV{CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}" STREQUAL "")
    set(GIT_BRANCH "$ENV{CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}")
  elseif(DEFINED ENV{CI_COMMIT_BRANCH} AND NOT "$ENV{CI_COMMIT_BRANCH}" STREQUAL "")
    set(GIT_BRANCH "$ENV{CI_COMMIT_BRANCH}")
  elseif(DEFINED ENV{CI_COMMIT_REF_NAME} AND NOT "$ENV{CI_COMMIT_REF_NAME}" STREQUAL "")
    set(GIT_BRANCH "$ENV{CI_COMMIT_REF_NAME}")
  else()
    set(GIT_BRANCH "Unknown")
  endif()
endif()
if(NOT GIT_COMMIT_HASH)
  set(GIT_COMMIT_HASH "Unknown")
endif()
if(NOT GIT_COMMIT_MESSAGE)
  set(GIT_COMMIT_MESSAGE "Unknown")
endif()
if(NOT GIT_COMMIT_TIMESTAMP)
  set(GIT_COMMIT_TIMESTAMP "Unknown")
endif()

escape_for_cpp_string_literal("${GIT_BRANCH}" GIT_BRANCH_ESCAPED)
escape_for_cpp_string_literal("${GIT_COMMIT_HASH}" GIT_COMMIT_HASH_ESCAPED)
escape_for_cpp_string_literal("${GIT_COMMIT_MESSAGE}" GIT_COMMIT_MESSAGE_ESCAPED)

message(STATUS "Revision: ${GIT_BRANCH} -m \"${GIT_COMMIT_MESSAGE}\" (${GIT_COMMIT_HASH}) ${GIT_COMMIT_TIMESTAMP}")

# CMAKE_NOT_USING_CONFIG_FLAGS stops CMake from caching flags for its predefined build types.
set(CMAKE_NOT_USING_CONFIG_FLAGS TRUE)
project(Peano
  DESCRIPTION "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes"
  LANGUAGES
    C
    CXX
)

include(ProcessorCount)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)

set(CMAKE_C_STANDARD 17 CACHE STRING "")
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

option(BUILD_TESTING "Build the unit tests" OFF)
include(CTest)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
if(IPO_SUPPORTED)
  message(STATUS "IPO/LTO enabled")
endif()

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

include(PeanoInstallHeaders)
include(PeanoEnableGPULanguage)
include(PeanoTarget)
include(PeanoHostDependencies)
include(PeanoOptions)
include(PeanoFlags)
include(PeanoDetectCPUArch)
include(PeanoOpenMPFeatures)

peano_create_options_target()
peano_apply_flags()
peano_find_host_dependencies()
peano_check_openmp_features()
peano_resolve_gpu_backend()
peano_detect_cpu_architecture()

if(_peano_gpu_backend STREQUAL "cuda")
  peano_enable_cuda_language()
elseif(_peano_gpu_backend STREQUAL "hip")
  peano_enable_hip_language()
endif()
if(NOT _peano_gpu_backend STREQUAL "no")
  peano_find_gpu_backend()
endif()

add_subdirectory(src)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${GIT_COMMIT_HASH}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  FILES_MATCHING PATTERN "*.cmake"
)

install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/api
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

export(EXPORT ${PROJECT_NAME}Targets
  FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
)

install(EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  NAMESPACE ${PROJECT_NAME}::
)

install(FILES
  ${CMAKE_SOURCE_DIR}/LICENSE
  ${CMAKE_SOURCE_DIR}/README.md
  ${CMAKE_SOURCE_DIR}/CONTRIBUTING.md
  ${CMAKE_SOURCE_DIR}/CODE_OF_CONDUCT.md
  ${CMAKE_SOURCE_DIR}/CITATION.cff
  ${CMAKE_BINARY_DIR}/CMakeCache.txt
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)
