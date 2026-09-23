# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  RunControl.cpp
  ParameterTable.cpp
  RuntimeParameters.cpp
  GlobalStatistics.cpp
  Format.cpp
  TextLayout.cpp
  ReportBox.cpp
  GridSnapshot.cpp
  phases/Configuration.cpp
  phases/GridConstruction.cpp
  phases/AllocateMemory.cpp
  phases/GridInitialisation.cpp
  phases/DynamicAMR.cpp
  phases/TimeLoop.cpp
  phases/FinalSummary.cpp
  Orchestrator.cpp
  Engine.cpp
  Launch.cpp
  UnitTests.cpp
  tests/DynamicAMRTest.cpp
  tests/FormatTest.cpp
  tests/GridSnapshotTest.cpp
  tests/ParameterTableTest.cpp
  tests/RunControlTest.cpp
  tests/RuntimeParametersTest.cpp
  tests/SolverStatisticsTest.cpp
  tests/TableTest.cpp
  tests/TextLayoutTest.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME Runner${mode}${dim}D
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano${mode}${dim}D
    )
  endforeach()
endforeach()
