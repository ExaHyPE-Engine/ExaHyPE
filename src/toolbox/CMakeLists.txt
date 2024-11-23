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

if(ENABLE_PARTICLES)
  add_subdirectory(particles)
endif()
