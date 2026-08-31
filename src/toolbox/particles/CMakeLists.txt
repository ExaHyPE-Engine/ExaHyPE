# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  particles.cpp
  internal.cpp
  FileReader.cpp
  MultiscaleTransitions.cpp
  ParticleFactory.cpp
  ParticleSet.cpp
  TrajectoryDatabase.cpp
  UnitTests.cpp
  tests/MultiscaleTransitionsTest.cpp
  tests/ParticlesTest.cpp
  tests/TestHelpers.cpp
  tests/TestParticle.cpp
  tests/TrajectoryDatabaseTest.cpp

  assignmentchecks/Database.cpp
  assignmentchecks/Event.cpp
  assignmentchecks/MeshSweepData.cpp
  assignmentchecks/ParticleIdentifier.cpp
  assignmentchecks/TracingAPI.cpp
  assignmentchecks/tests/TestHelpers.cpp
  assignmentchecks/tests/TestParticle.cpp

  potentials/Springs.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
  INSTALL_SUBDIR "toolbox"
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME ToolboxParticles${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano4${mode}${dim}d
    )
  endforeach()
endforeach()
