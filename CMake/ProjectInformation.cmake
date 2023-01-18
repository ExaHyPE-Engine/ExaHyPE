# Project meta information
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  set(GIT_BRANCH "")
  set(GIT_COMMIT_HASH "")
  set(GIT_COMMIT_HASH_SHORT "")
endif()

set(META_PROJECT_NAME "Peano")
set(META_PROJECT_DESCRIPTION
    "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes"
)
set(META_AUTHOR_ORGANIZATION
    "Durham University and Technische Universitaet Muenchen")
set(META_AUTHOR_DOMAIN "www.peano-framework.org")
set(META_VERSION_REVISION "${GIT_COMMIT_HASH_SHORT}")
set(META_GIT_BRANCH "${GIT_BRANCH}")
set(META_GIT_HASH "${GIT_COMMIT_HASH}")
set(META_GIT_BUGREPORT "https://gitlab.lrz.de/hpcsoftware/Peano/-/issues")
set(META_GIT_URL "https://gitlab.lrz.de/hpcsoftware/Peano")


project(
  ${META_PROJECT_NAME}
  DESCRIPTION ${META_PROJECT_DESCRIPTION}
  HOMEPAGE_URL ${META_AUTHOR_DOMAIN}
  LANGUAGES ${LANGUAGES})
file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")
