# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Curvi.cpp
  ErrorMeasurement.cpp
  RefinementRequests.cpp
  TimeStepping.cpp

  particles/FileReader.cpp
  particles/TrajectoryDatabase.cpp

  plotter/FacePlotter.cpp
  plotter/PatchPlotter.cpp
  runner/Console.cpp
  runner/Runner.cpp
  runner/RuntimeParameters.cpp
  runner/Statistics.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME ExaHyPE${mode}${dimensions}D
      MODE ${mode}
      DIMENSION ${dimensions}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano${mode}${dimensions}D
    )
  endforeach()
endforeach()
