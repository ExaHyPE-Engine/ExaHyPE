// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  namespace timestepping {
    /**
     * Update a particle with the explicit Euler
     *
     * The code should invoke this routine only in touchVertexLastTime(). It
     * should never change the particle position throughout the mesh traversal,
     * as it might introduce inconsistent particle data structure.
     *
     */
    template <typename Particle>
    void computeExplicitEulerWithGlobalTimeStepSize(
      const std::vector< Particle* >&  localParticles
    );

    template <typename Particle>
    void computeExplicitEulerCromerWithGlobalTimeStepSize(
      const std::vector< Particle* >&  localParticles
    );
  }
}


#include "Euler.cpph"


