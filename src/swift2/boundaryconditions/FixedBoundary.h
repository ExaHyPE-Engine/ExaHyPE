// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "peano4/datamanagement/VertexMarker.h"

namespace swift2 {
  namespace boundaryconditions {
    /**
     * Apply fixed boundary conditions to particle
     *
     * This routine is usually inserted into your code by the action set
     * swift2.api.boundaryconditions.Fixed. It serves as a proxy to the
     * other applyFixedBoundaryCondition(), i.e. forwards the actual
     * position update computation to this routine, and then takes the
     * updated position, writes it into the particle, and logs the position
     * update within the particle database (so all assertions are still
     * correct).
     *
     * Please consult the action set above for further information about
     * the actual algorithm implemented.
     */
    template <typename Particle>
    void applyFixedBoundaryCondition(
        Particle&                                     particle,
        const peano4::datamanagement::VertexMarker&   marker,
        const tarch::la::Vector<Dimensions, double>&  domainOffset,
        const tarch::la::Vector<Dimensions, double>&  domainSize,
        double                                        relativeDomainHalo,
        bool                                          dampVelocity,
        int                                           treeId
    );

    /**
     * Determine updated position and velocity due to boundary conditions
     *
     * See applyFixedBoundaryCondition() and notably swift2.api.boundaryconditions.Fixed
     * for some detailed information what is actually implemented here.
     *
     * @return Tuple of new position and velocity
     */
    std::pair< tarch::la::Vector<Dimensions, double>, tarch::la::Vector<Dimensions, double> >  getUpdateDueToFixedBoundaryCondition(
      const tarch::la::Vector<Dimensions, double>&  particleX,
      const tarch::la::Vector<Dimensions, double>&  particleV,
      double                                        maxV,
      double                                        maxDt,
      double                                        h,
      const tarch::la::Vector<Dimensions, double>&  domainOffset,
      const tarch::la::Vector<Dimensions, double>&  domainSize,
      double                                        relativeDomainHalo,
      bool                                          dampVelocity
    );
  }
}

#include "FixedBoundary.cpph"
