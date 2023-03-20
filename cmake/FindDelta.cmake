include(FindPackageHandleStandardArgs)

find_path(Delta_INCLUDE_DIR Delta/Delta-serial.h)
# Provide find library first with *.a to prefer static libraries
# libDelta-ompt requires OpenMP standard at least 5.0
find_library(Delta_LIBRARY libDelta.a Delta)

find_package_handle_standard_args(Delta
  DEFAULT_MSG
  Delta_INCLUDE_DIR
  Delta_LIBRARY
)

message(STATUS "Delta_LIBRARY: ${Delta_LIBRARY}, Delta_INCLUDE_DIR: ${Delta_INCLUDE_DIR}")
mark_as_advanced(Delta_LIBRARY Delta_INCLUDE_DIR)

if(Delta_FOUND)
  set(Delta_LIBRARIES ${Delta_LIBRARY})
  set(Delta_INCLUDE_DIRS ${Delta_INCLUDE_DIR})
endif()
