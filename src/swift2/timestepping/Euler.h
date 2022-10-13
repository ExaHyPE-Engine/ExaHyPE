// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  namespace timestepping {
    template <typename Particle>
    void computeExplicitEulerWithGlobalTimeStepSize(
      const std::list< Particle* >&  localParticles
    );

    template <typename Particle>
    void computeExplicitEulerCromerWithGlobalTimeStepSize(
      const std::list< Particle* >&  localParticles
    );
  }
}


#include "Euler.cpph"


