include(FindPackageHandleStandardArgs)

find_path(OTF2_INCLUDE_DIR NAMES otf2/otf2.h)

# Provide find library first with *.a to prefer static libraries
# libOTF2-ompt requires OpenMP standard at least 5.0
find_library(OTF2_LIBRARY NAMES libotf2.a libotf2)
	
find_package_handle_standard_args(OTF2
 DEFAULT_MSG
  OTF2_INCLUDE_DIR
  OTF2_LIBRARY
)

message(STATUS "OTF2_LIBRARY: ${OTF2_LIBRARY}, OTF2_INCLUDE_DIR: ${OTF2_INCLUDE_DIR}")
mark_as_advanced(OTF2_LIBRARY OTF2_INCLUDE_DIR)

if(OTF2_FOUND)
	set(OTF2_LIBRARIES   ${OTF2_LIBRARY})
	set(OTF2_INCLUDE_DIRS ${OTF2_INCLUDE_DIR})
endif()

if(OTF2_FOUND AND NOT TARGET OTF2::OTF2)
  add_library(OTF2::OTF2 UNKNOWN IMPORTED)
  set_target_properties(OTF2::OTF2 PROPERTIES
    IMPORTED_LOCATION "${OTF2_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_OTF2_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${OTF2_INCLUDE_DIR}"
  )
endif()