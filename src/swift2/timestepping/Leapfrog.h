// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  namespace timestepping {
    /**
     * Update a particle with the leapfrog KDK integrator
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

    /** 
     * Drift step. In this, we move the particles by a full time-step using:
     *
     *    {\bf x}^{n+1} = {\bf x}^{n} + {\bf v}^{n+1/2}{\Delta t}
     *
     * Notice that this operation modifies the particle topology and hence
     * particles must be re-sorted before further operations can be executed.
     */

    template <typename Particle>
    void computeLeapfrogDriftWithGlobalTimeStepSize(
      const std::vector< Particle* >&  localParticles
    );

    /**
    *  Kick steps. There are two kick steps in the KDK scheme, in which we
    *  update the particle velocity by a half-timestep using:
    *
    *  Kick1:
    *     {\bf v}^{n+\frac{1}{2}} &= {\bf v}^{n} + {\bf a}^{n}\frac{\Delta t}{2}
    *  Kick2:
    *     {\bf v}^{n+1} &= {\bf v}^{n+\frac{1}{2}} + {\bf a}^{n+1}\frac{\Delta t}{2}
    *  
    * Notice that the last kick in this sequence is implicit if ${\bf a}$
    * depends on ${\bf v}$ (e.g. due to artificial viscosity terms), which in
    * principle requires extra treatments (see e.g. Price et al. 2017).
    *
    * @param kickNumber: wether we are doing first (kick 1) or second (kick 2) in
    * the leapfrog scheme.
    * @param isSPH: whether we are running an SPH simulation or not. True means
    * we not only update the velocity but also other SPH variables.
    */

    template <typename Particle>
    void computeLeapfrogKickWithGlobalTimeStepSize(
      const std::vector< Particle* >&  localParticles,
      const int kickNumber,
      const bool isSPH
    );


  }
}


#include "Leapfrog.cpph"


