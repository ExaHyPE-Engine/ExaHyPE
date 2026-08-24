# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  AMRRoutines.cpp
  ChildDataStage.cpp
  Curvi.cpp
  Enumeration.cpp
  MultiresolutionDetail.cpp
  ProjectPatch.cpp
  ReconstructPatch.cpp
  UnitTests.cpp
  tests/ChildDataStageTest.cpp
  tests/CurviTest.cpp
  tests/EnumerationTest.cpp
  tests/InterpolationTest.cpp
  tests/MultiresolutionDetailTest.cpp
  tests/PatchKernelsTest.cpp
  tests/RestrictionTest.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
  INSTALL_SUBDIR "toolbox"
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME ToolboxBlockStructured${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano4${mode}${dim}d
    )
  endforeach()
endforeach()
