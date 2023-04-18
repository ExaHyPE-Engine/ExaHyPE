// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  namespace statistics {
    /**
     * It is important that we have an if here and don't do anything if the
     * cell is empty. Because if we did set the default values, we'd likely
     * overwite the .
     */
    template <typename Particle>
    void reduceVelocityAndCutOffRadius(
      const std::list< Particle* >&  localParticles
    );
  }
}


#include "Statistics.cpph"


