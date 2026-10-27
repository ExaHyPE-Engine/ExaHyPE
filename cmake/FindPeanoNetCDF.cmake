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
set(NetCDF_HAS_HDF5 FALSE)
set(NetCDF_IS_PARALLEL FALSE)

if(NetCDF_INCLUDE_DIR AND EXISTS "${NetCDF_INCLUDE_DIR}/netcdf_meta.h")
  file(READ "${NetCDF_INCLUDE_DIR}/netcdf_meta.h" _netcdf_meta_content)

  # Parse Version
  # Try macro parsing first
  string(REGEX REPLACE ".*#define[ \t]+NC_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _ver_major "${_netcdf_meta_content}")
  string(REGEX REPLACE ".*#define[ \t]+NC_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _ver_minor "${_netcdf_meta_content}")
  string(REGEX REPLACE ".*#define[ \t]+NC_VERSION_PATCH[ \t]+([0-9]+).*" "\\1" _ver_patch "${_netcdf_meta_content}")
  string(REGEX REPLACE ".*#define[ \t]+NC_VERSION_NOTE[ \t]+\"([^\"]*)\".*" "\\1" _ver_note "${_netcdf_meta_content}")

  if("${_ver_major}" MATCHES "^[0-9]+$")
    set(NetCDF_VERSION "${_ver_major}.${_ver_minor}.${_ver_patch}${_ver_note}")
  else()
    # Fallback to string parsing
    string(REGEX MATCH "#define[ \t]+NC_VERSION[ \t]+\"([^\"]+)\"" _ver_match "${_netcdf_meta_content}")
    if(_ver_match)
      string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" NetCDF_VERSION "${_ver_match}")
    endif()
  endif()

  # Check for NetCDF-4 support (HDF5 backend)
  # NetCDF-C 4.9.3 may not define NC_HAS_NC4 in netcdf_meta.h anymore.
  # Use NC_HAS_HDF5 as the primary check, and keep NC_HAS_NC4 as fallback.
  if("${_netcdf_meta_content}" MATCHES "#define[ \t]+NC_HAS_HDF5[ \t]+1"
     OR "${_netcdf_meta_content}" MATCHES "#define[ \t]+NC_HAS_NC4[ \t]+1")
    set(NetCDF_HAS_HDF5 TRUE)
  endif()

  # Check for parallel support
  if("${_netcdf_meta_content}" MATCHES "#define[ \t]+NC_HAS_PARALLEL4[ \t]+1"
     OR "${_netcdf_meta_content}" MATCHES "#define[ \t]+NC_HAS_PARALLEL[ \t]+1")
    set(NetCDF_IS_PARALLEL TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PeanoNetCDF
  REQUIRED_VARS NetCDF_LIBRARY NetCDF_INCLUDE_DIR
  VERSION_VAR NetCDF_VERSION
)

if(PeanoNetCDF_FOUND)
  set(NetCDF_INCLUDE_DIRS "${NetCDF_INCLUDE_DIR}")
  set(NetCDF_LIBRARIES "${NetCDF_LIBRARY}")

  if(NOT TARGET NetCDF::NetCDF)
    add_library(NetCDF::NetCDF UNKNOWN IMPORTED)
    set_target_properties(NetCDF::NetCDF PROPERTIES
      IMPORTED_LOCATION "${NetCDF_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${NetCDF_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(
  NetCDF_INCLUDE_DIR
  NetCDF_LIBRARY
)
