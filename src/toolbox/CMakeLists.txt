# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
if(ENABLE_TOOLBOX_BLOCKSTRUCTURED)
  add_subdirectory(blockstructured)
endif()

if(ENABLE_TOOLBOX_LOADBALANCING)
  add_subdirectory(loadbalancing)
endif()

if(ENABLE_TOOLBOX_PARTICLES)
  add_subdirectory(particles)
endif()

if(ENABLE_TOOLBOX_CURVI)
  add_subdirectory(curvi)
endif()
