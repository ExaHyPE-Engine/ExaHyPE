// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  namespace timestepping {
    /**
     * Reset the marker of a particle from whatever it has been before to
     * NotMoved. This routine should be called in any task graph before a
     * particle's position is actually altered. The name is not brilliant,
     * as you should also call this routine prior to a change of the interaction
     * radius.
     */
    template <typename Particle>
    void resetMovedParticleMarker(
      const std::vector< Particle* >&  localParticles
    );
  }
}


#include "TimeStepping.cpph"


