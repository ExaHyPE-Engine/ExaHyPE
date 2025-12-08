# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(NETCDF_SEARCH_HINTS "")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_NetCDF QUIET netcdf)
  list(APPEND NETCDF_SEARCH_HINTS ${PC_NetCDF_LIBRARY_DIRS} ${PC_NetCDF_INCLUDE_DIRS})
endif()

list(APPEND NETCDF_SEARCH_HINTS
  $ENV{NETCDF_DIR}
  $ENV{NETCDF_DIR}/lib
  $ENV{NETCDF_DIR}/include
  $ENV{NETCDF_LIB_DIR}
  $ENV{NETCDF_INCLUDE_DIR}
)

find_path(NetCDF_INCLUDE_DIR
  NAMES netcdf.h
  HINTS ${NETCDF_SEARCH_HINTS}
  PATH_SUFFIXES include
  DOC "Path to netcdf.h"
)

find_library(NetCDF_LIBRARY
  NAMES netcdf
  HINTS ${NETCDF_SEARCH_HINTS}
  PATH_SUFFIXES lib lib64
  DOC "NetCDF C library"
)

set(NetCDF_VERSION "")

if(NetCDF_INCLUDE_DIR AND EXISTS "${NetCDF_INCLUDE_DIR}/netcdf_meta.h")
  file(STRINGS "${NetCDF_INCLUDE_DIR}/netcdf_meta.h" _netcdf_version_lines
    REGEX "#define[ \t]+NC_VERSION_(MAJOR|MINOR|PATCH|NOTE)")

  string(REGEX REPLACE ".*NC_VERSION_MAJOR *\([0-9]*\).*" "\\1" _netcdf_version_major "${_netcdf_version_lines}")
  string(REGEX REPLACE ".*NC_VERSION_MINOR *\([0-9]*\).*" "\\1" _netcdf_version_minor "${_netcdf_version_lines}")
  string(REGEX REPLACE ".*NC_VERSION_PATCH *\([0-9]*\).*" "\\1" _netcdf_version_patch "${_netcdf_version_lines}")
  string(REGEX REPLACE ".*NC_VERSION_NOTE *\"\([^\"]*\)\".*" "\\1" _netcdf_version_note "${_netcdf_version_lines}")

  if(_netcdf_version_major MATCHES "[0-9]+")
    set(NetCDF_VERSION "${_netcdf_version_major}.${_netcdf_version_minor}.${_netcdf_version_patch}${_netcdf_version_note}")
  endif()

  if(NOT NetCDF_VERSION)
    file(READ "${NetCDF_INCLUDE_DIR}/netcdf_meta.h" _netcdf_meta_header)
    string(REGEX MATCH "#define[ \t]+NC_VERSION[ \t]+\"([^\"]+)\"" _ver_match "${_netcdf_meta_header}")
    if(_ver_match)
      string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" NetCDF_VERSION "${_ver_match}")
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NetCDF
  REQUIRED_VARS NetCDF_LIBRARY NetCDF_INCLUDE_DIR
  VERSION_VAR NetCDF_VERSION
)

if(NetCDF_FOUND)
  set(NetCDF_INCLUDE_DIRS "${NetCDF_INCLUDE_DIR}")
  set(NetCDF_LIBRARIES "${NetCDF_LIBRARY}")

  if(NOT TARGET NetCDF::NetCDF)
    add_library(NetCDF::NetCDF UNKNOWN IMPORTED)
    set_target_properties(NetCDF::NetCDF PROPERTIES
      IMPORTED_LOCATION "${NetCDF_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${NetCDF_INCLUDE_DIR}")
  endif()

  set(NETCDF_INCLUDES "${NetCDF_INCLUDE_DIR}")
  set(NETCDF_LIBRARIES "${NetCDF_LIBRARY}")
  set(NETCDF_VERSION "${NetCDF_VERSION}")
endif()

mark_as_advanced(NETCDF_INCLUDES NETCDF_LIBRARIES NETCDF_VERSION)
