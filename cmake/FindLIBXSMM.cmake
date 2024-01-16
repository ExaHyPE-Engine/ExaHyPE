if(LIBXSMM_INCLUDE_DIRS AND LIBXSMM_INCLUDE_DIRS AND LIBXSMM_LIBRARIES AND LIBXSMM_VERSION AND LIBXSMM_EXECUTABLE)
  set(LIBXSMM_FOUND TRUE)
else()
  find_program(LIBXSMM_EXECUTABLE libxsmm_gemm_generator
    HINTS ENV LIBXSMM_DIR
    PATH_SUFFIXES bin
    DOC "Directory where the LIBXSMM binary file is located"
  )
  find_path(LIBXSMM_INCLUDE_DIRS libxsmm.h
    PATH_SUFFIXES include
    DOC "Directory where LIBXSMM header files are located"
  )
  find_library(LIBXSMM_LIBRARIES
    NAMES xsmm
    PATH_SUFFICES lib
    DOC "Path to LIBXSMM library"
  )
  find_library(LIBDL_LIBRARIES
    NAMES dl
    PATH_SUFFICES lib
    DOC "Path to libdl library"
  )
  list(APPEND LIBXSMM_LIBRARIES ${LIBDL_LIBRARIES})

  if(LIBXSMM_INCLUDE_DIRS STREQUAL "LIBXSMM_INCLUDE_DIRS-NOTFOUND" OR
     LIBXSMM_LIBARIES STREQUAL "LIBXSMM_LIBRARIES-NOTFOUND")
    message(STATUS "LIBXSMM not found. Trying to find it using pkg-config...")
    find_package(PkgConfig QUIET)
    pkg_check_modules(_LIBXSMM QUIET libxsmm)
    if(_LIBXSMM_FOUND)
      set(LIBXSMM_INCLUDE_DIRS "${_LIBXSMM_INCLUDE_DIRS}")
      set(LIBXSMM_LIBRARIES "${_LIBXSMM_LIBRARIES}")
      set(LIBXSMM_VERSION "${_LIBXSMM_VERSION}")
      message(STATUS "Found LIBXSMM using pkg-config.")
    else()
      message(STATUS "LIBXSMM not found. Fetching LIBXSMM from source...")
      include(FetchContent)
      FetchContent_Declare(
        XSMM
        GIT_REPOSITORY https://github.com/libxsmm/libxsmm.git
        GIT_TAG main
      )
      FetchContent_GetProperties(XSMM)
      if(NOT XSMM_POPULATED)
        FetchContent_Populate(XSMM)
        file(GLOB _GLOB_XSMM_SRCS LIST_DIRECTORIES false CONFIGURE_DEPENDS ${xsmm_SOURCE_DIR}/src/*.c)
        list(REMOVE_ITEM _GLOB_XSMM_SRCS ${xsmm_SOURCE_DIR}/src/libxsmm_generator_gemm_driver.c)
        set(LIBXSMM_INCLUDE_DIRS "${xsmm_SOURCE_DIR}/include")
        if(NOT TARGET LIBXSMM::LIBXSMM)
          add_library(XSMM STATIC ${_GLOB_XSMM_SRCS})
          add_library(LIBXSMM::LIBXSMM ALIAS XSMM)
          target_include_directories(XSMM PUBLIC ${LIBXSMM_INCLUDE_DIRS})
          target_compile_definitions(XSMM PUBLIC LIBXSMM_DEFAULT_CONFIG)
          target_compile_definitions(XSMM PRIVATE __BLAS=0)
        endif()
      endif()
      message(STATUS "LIBXSMM fetched from source and added to the project.")
      return()
    endif()
  endif()

  # Find version file from header
  if(LIBXSMM_INCLUDE_DIRS)
    file(READ "${LIBXSMM_INCLUDE_DIRS}/libxsmm_version.h" LIBXSMM_VERSION_HEADER)
    string(REGEX MATCH "#define LIBXSMM_CONFIG_VERSION_MAJOR [0-9]+" LIBXSMM_VERSION_MAJOR "${LIBXSMM_VERSION_HEADER}")
    string(REGEX MATCH "[0-9]+" LIBXSMM_VERSION_MAJOR "${LIBXSMM_VERSION_MAJOR}")
    string(REGEX MATCH "#define LIBXSMM_CONFIG_VERSION_MINOR [0-9]+" LIBXSMM_VERSION_MINOR "${LIBXSMM_VERSION_HEADER}")
    string(REGEX MATCH "[0-9]+" LIBXSMM_VERSION_MINOR "${LIBXSMM_VERSION_MINOR}")
    set(LIBXSMM_VERSION "${LIBXSMM_VERSION_MAJOR}.${LIBXSMM_VERSION_MINOR}")
    unset(LIBXSMM_VERSION_HEADER)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LIBXSMM REQUIRED_VARS LIBXSMM_EXECUTABLE LIBXSMM_INCLUDE_DIRS LIBXSMM_LIBRARIES
    VERSION_VAR LIBXSMM_VERSION)

  mark_as_advanced(LIBXSMM_EXECUTABLE LIBXSMM_INCLUDE_DIRS LIBXSMM_LIBRARIES
    LIBXSMM_VERSION_MAJOR LIBXSMM_VERSION_MINOR LIBXSMM_VERSION)
endif()

if(LIBXSMM_FOUND AND NOT TARGET LIBXSMM::LIBXSMM)
  add_library(LIBXSMM::LIBXSMM INTERFACE IMPORTED)
  set_target_properties(LIBXSMM::LIBXSMM PROPERTIES
    INTERFACE_LINK_LIBRARIES "${LIBXSMM_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBXSMM_INCLUDE_DIRS}")
endif()
