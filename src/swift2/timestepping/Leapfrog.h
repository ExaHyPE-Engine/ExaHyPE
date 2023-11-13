// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>

#include "swift2/kernels/SwiftKernelPlaceholders.h"


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
     * Drift step.
     *
     * In this, we move the particles by a full time-step using:
     *
     * @f$ {\bf x}^{n+1} = {\bf x}^{n} + {\bf v}^{n+1/2}{\Delta t} @f$
     *
     * Notice that this operation modifies the particle topology and hence
     * particles must be re-sorted before further operations can be executed.
     */
    template <typename ParticleContainer>
    void computeLeapfrogDriftWithGlobalTimeStepSize(const ParticleContainer& particles);

    /**
     * Vectorised alternative to computeLeapfrogDriftWithGlobalTimeStepSize().
     *
     * This routine assumes that the numberOfParticles are all consecutive in
     * memory so we can simply loop over them. Obviously, there's some
     * redundancy in this code, as we expect
     *
     *          particles.size()==numberOfParticles
     *
     * Having the size as explicit argument is convenient, as it means that we
     * can overload the function.
     */
    template <typename ParticleContainer>
    void computeLeapfrogDriftWithGlobalTimeStepSize(const ParticleContainer& particles, int numberOfParticles);

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

    template <typename ParticleContainer>
    void computeLeapfrogKickWithGlobalTimeStepSize(const ParticleContainer& localParticles);

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
     * This version is for the SPH time integration, where we access additional
     * particle fields which are otherwise not present, such as the time derivative
     * of the internal energy.
     *
     * @param kickNumber: wether we are doing first (kick 1) or second (kick 2) in
     * the leapfrog scheme.
     * @param isSPH: whether we are running an SPH simulation or not. True means
     * we not only update the velocity but also other SPH variables.
     */
    template <typename ParticleContainer>
    void computeSPHLeapfrogKickWithGlobalTimeStepSize(const ParticleContainer& localParticles, const int kickNumber);


    /**
     * Kick extra step, i.e. update some thermodynamic
     * quantities forward in time. In particular, U is integrated forward in
     * time treating it as the velocity.
     */
    template <typename Particle>
    void leapfrogKickExtraWithGlobalTimeStepSize(Particle& localParticle);

    /**
     * Predict hydro terms (SWIFT)
     * Relevant for inactive particles when using local-timestepping, but always
     * used.
     */
    template <typename ParticleContainer>
    void computePredictHydro(const ParticleContainer& localParticles);


    /**
     * Debugging:
     * Fake Drift step in which we move a particle into the midpoint of a cell.
     * Does that particle disappear from the simulation?
     */
    template <typename ParticleContainer>
    void moveParticleIntoCriticalLine(const ParticleContainer& localParticles);


  } // namespace timestepping
} // namespace swift2


#include "Leapfrog.cpph"
