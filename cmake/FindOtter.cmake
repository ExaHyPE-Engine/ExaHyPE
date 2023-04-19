include(FindPackageHandleStandardArgs)

find_path(Otter_INCLUDE_DIR NAMES otter/otter-task-graph.h)

# Provide find library first with *.a to prefer static libraries
# libotter-ompt requires OpenMP standard at least 5.0
if(OpenMP_FOUND)
  if(OpenMP_CXX_VERSION VERSION_LESS "5.0")
    message(WARNING "Otter with OpenMP Tracing requires OpenMP 5.0 or newer.\
OpenMP ${OpenMP_CXX_VERSION} was provided, reverting to serial otter")
    find_library(Otter_LIBRARY NAMES libotter-task-graph.a otter-task-graph)
  else()
    #find_library(Otter_LIBRARY NAMES libotter-ompt.a otter-ompt)
    find_library(Otter_LIBRARY NAMES libotter-task-graph.a otter-task-graph)
  endif()
else()
  find_library(Otter_LIBRARY NAMES libotter-task-graph.a otter-task-graph)
  message(STATUS "Link to otter-task-graph as OpenMP Is not Ffund")
endif()

find_package_handle_standard_args(Otter
  DEFAULT_MSG
  Otter_INCLUDE_DIR
  Otter_LIBRARY
)

message(STATUS "Otter_LIBRARY: ${Otter_LIBRARY}, Otter_INCLUDE_DIR: ${Otter_INCLUDE_DIR}")
mark_as_advanced(Otter_LIBRARY Otter_INCLUDE_DIR)

if(Otter_FOUND)
  set(Otter_LIBRARIES ${Otter_LIBRARY})
  set(Otter_INCLUDE_DIRS ${Otter_INCLUDE_DIR})
endif()

if(Otter_FOUND AND NOT TARGET Otter::Otter)
  add_library(Otter::Otter UNKNOWN IMPORTED)
  set_target_properties(Otter::Otter PROPERTIES
    IMPORTED_LOCATION "${Otter_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_Otter_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${Otter_INCLUDE_DIR}"
  )
  find_package(OTF2 REQUIRED)
  target_link_libraries(Otter::Otter INTERFACE OTF2::OTF2)
endif()
