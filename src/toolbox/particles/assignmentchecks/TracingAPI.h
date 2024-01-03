// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace particles {
    namespace assignmentchecks {
      /**
       * Inform API that this is the end of a grid run-throuch
       */
      void endMeshSweep()
      #if PeanoDebug>0
      ;
      #else
      {}
      #endif

      /**
       * Log that a particle is erased
       *
       * This should solely happen for virtual particles, i.e. halo particles.
       * Before this function is called, the corresponding particle has to be
       * removed from its associated vertex.
       */
      void eraseParticle(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        int                                        treeId
      )
      #if PeanoDebug>0
      ;
      #else
      {}
      #endif

      /**
       * Assign a particle to a vertex
       *
       * To make this a valid operation, the particle has to be "free", i.e. it
       * may not be assigned to any other vertex yet. Consequently, the particle
       * is either new (created or flying in from somewhere else), or we have
       * triggered removeParticleFromVertex() before.
       */
      void assignParticleToVertex(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        const tarch::la::Vector< Dimensions, double >& vertexX,
        int                                        vertexLevel,
        int                                        treeId
      )
      #if PeanoDebug>0
      ;
      #else
      {}
      #endif

      /**
       * Remove particle from vertex
       *
       * To make this API call valid, the particle has to be assigned to a
       * vertex beforehand. So ther has to be a assignParticleToVertex()
       * before this one.
       */
      void removeParticleFromVertex(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        const tarch::la::Vector< Dimensions, double >& vertexX,
        int                                        vertexLevel,
        int                                        treeId
      )
      #if PeanoDebug>0
      ;
      #else
      {}
      #endif

      /**
       * Assign a particle to a sieve set
       *
       * To make this routine valid, the particle may not be assigned to a
       * vertex anymore, i.e. removeParticleFromVertex() has to be called
       * before.
       */
      void assignParticleToSieveSet(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        int                                        treeId
      )
      #if PeanoDebug>0
      ;
      #else
      {}
      #endif
    }
  }
}

