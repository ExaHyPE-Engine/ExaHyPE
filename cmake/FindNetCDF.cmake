# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

find_path(NETCDF_INCLUDES
  NAMES netcdf.h
  PATH_SUFFIXES include
  DOC "Path to netcdf.h"
)

find_library(NETCDF_LIBRARIES
  NAMES netcdf
  PATH_SUFFIXES lib
  DOC "NetCDF C library"
)

set(NETCDF_VERSION "")
if(NETCDF_INCLUDES AND EXISTS "${NETCDF_INCLUDES}/netcdf_meta.h")
  file(READ "${NETCDF_INCLUDES}/netcdf_meta.h" _netcdf_meta_header)
  string(REGEX MATCH "#define[ \t]+NC_VERSION[ \t]+\"([^\"]+)\"" _ver_match "${_netcdf_meta_header}")
  if(_ver_match)
    string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" NETCDF_VERSION "${_ver_match}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NetCDF
  REQUIRED_VARS NETCDF_LIBRARIES NETCDF_INCLUDES
  VERSION_VAR NETCDF_VERSION
)

mark_as_advanced(NETCDF_INCLUDES NETCDF_LIBRARIES NETCDF_VERSION)
