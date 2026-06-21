# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

# Install the public headers of the current source directory into the package include tree.
function(install_peano_headers)
  set(options)
  set(oneValueArgs INSTALL_SUBDIR)
  set(multiValueArgs PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(ARG_INSTALL_SUBDIR)
    set(header_install_dest "${CMAKE_INSTALL_INCLUDEDIR}/${ARG_INSTALL_SUBDIR}")
  else()
    set(header_install_dest "${CMAKE_INSTALL_INCLUDEDIR}")
  endif()

  foreach(pattern IN LISTS ARG_PATTERNS)
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DESTINATION ${header_install_dest}
      FILES_MATCHING
      PATTERN ${pattern}
    )
  endforeach()
endfunction()
