// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"


namespace swift2 {
  namespace timestepping {
    /**
     * Drift step.
     *
     * In this, we move the particles by a full time-step using:
     *
     * @f$ {\bf x}^{n+1} = {\bf x}^{n} + {\bf v}^{n+1/2}{\Delta t} @f$
     *
     * Notice that this operation modifies the particle topology and hence
     * particles must be re-sorted before further operations can be executed.
     */
    template <typename Particle>
    void leapfrogDriftWithGlobalTimeStepSize(
      const peano4::datamanagement::VertexMarker& marker,
      Particle&                                   localParticle
    );

    /**
     *  Kick steps. There are two kick steps in the KDK scheme, in which we
     *  update the particle velocity by a half-timestep using:
     *
     *  Kick1:
     *  @f$ {\bf v}^{n+\frac{1}{2}} &= {\bf v}^{n} +
     *  {\bf a}^{n}\frac{\Delta * t}{2}@f$
     *
     *  Kick2:
     *  @f$ {\bf v}^{n+1} &= {\bf v}^{n+\frac{1}{2}} + {\bf a}^{n+1}\frac{\Delta
     * t}{2} @f$
     *
     * Notice that the last kick in this sequence is implicit if ${\bf a}$
     * depends on ${\bf v}$ (e.g. due to artificial viscosity terms), which in
     * principle requires extra treatments (see e.g. Price et al. 2017).
     *
     * The present version of the kick is the vanilla version: Enough for
     * leapfrog for normal ODE integrators, but schemes such as SPH require
     * us to do additional stuff besides the sole kick of the velocities.
     */
    template <typename Particle>
    void leapfrogKickWithGlobalTimeStepSize(
      const peano4::datamanagement::VertexMarker& marker,
      Particle&                                   localParticle
    );
  } // namespace timestepping
} // namespace swift2


#include "Leapfrog.cpph"
