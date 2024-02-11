# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
if(ENABLE_BLOCKSTRUCTURED)
  add_subdirectory(blockstructured)
endif()

if(ENABLE_FINITEELEMENTS)
  add_subdirectory(finiteelements)
endif()

if(ENABLE_LOADBALANCING)
  add_subdirectory(loadbalancing)
endif()

if(ENABLE_MULTIPRECISION)
  add_subdirectory(multiprecision)
endif()

if(ENABLE_PARTICLES)
  add_subdirectory(particles)
endif()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/toolbox"
)
