// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

namespace swift2 {
  namespace timestepping {
    /**
     * Update a particle with the explicit Euler
     *
     * The code should invoke this routine only in touchVertexLastTime(). It
     * should never change the particle position throughout the mesh traversal,
     * as it might introduce inconsistent particle data structure.
     *
     * We do not update all particles but merely those which are labelled as
     * local. Virtual particles are mere copies from another tree. We should
     * not alter their states or positions. Any neighbour who is in charge
     * of the particle should do this, and we then expect such a rank to send
     * over a new copy.
     */
    template <typename Particle>
    void computeExplicitEulerWithGlobalTimeStepSize(
      const peano4::datamanagement::VertexMarker&  marker,
      Particle&                                    particle
    );


    template <typename Particle>
    void computeExplicitEulerCromerWithGlobalTimeStepSize(
      const peano4::datamanagement::VertexMarker&  marker,
      Particle&                                    particle
    );
  }
}


#include "Euler.cpph"


