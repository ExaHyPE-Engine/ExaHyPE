# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Blacklist.cpp
  Configuration.cpp
  InitialInterRankSplit.cpp
  InitialIntraRankSplit.cpp
  InitialSplitPlan.cpp
  LoadBalancer.cpp
  State.cpp
  Statistics.cpp
  UnitTests.cpp
  tests/BlacklistTest.cpp
  tests/InitialSplitScenarioTest.cpp
  tests/LoadBalancerTest.cpp
  tests/StatisticsTest.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
  INSTALL_SUBDIR "toolbox"
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME ToolboxLoadBalancing${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano4${mode}${dim}d
    )
  endforeach()
endforeach()
