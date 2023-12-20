// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

#include "swift2/kernels/ParticleUpdatePredicates.h"


namespace swift2 {
  namespace kernels {
    namespace legacy {
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
      template <typename Particle>
      void leapfrog_drift_global_time_step_size(Particle* particle);

      template <typename Particle>
      void leapfrogDriftWithGlobalTimeStepSize(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeWorkedOnInVertexKernel(marker, localParticle)) {
          leapfrog_drift_global_time_step_size(&localParticle);
        }
      }

      /**
       *  Kick steps. There are two kick steps in the KDK scheme, in which we
       *  update the particle velocity by a half-timestep using:
       *
       *  Kick1:
       *     {\bf v}^{n+\frac{1}{2}} &= {\bf v}^{n} + {\bf a}^{n}\frac{\Delta t}{2}
       *
       *  Kick2:
       *     {\bf v}^{n+1} &= {\bf v}^{n+\frac{1}{2}} + {\bf a}^{n+1}\frac{\Delta t}{2}
       *
       * Notice that the last kick in this sequence is implicit if ${\bf a}$
       * depends on ${\bf v}$ (e.g. due to artificial viscosity terms), which in
       * principle requires extra treatments (see e.g. Price et al. 2017).
       *
       * In Swift, the second step is in practice more complicated: We kick and
       * afterwards call
       *
       * - hydro_kick_extra()
       * - hydro_reset_predicted_values()
       *
       * So there are two additional steps that you have to add afterwards.
       *
       * This version is tailored towards the SPH time integration, where we access additional
       * particle fields which are otherwise not present, such as the time derivative
       * of the internal energy.
       */
      template <typename Particle>
      void leapfrog_kick_global_time_step_size(Particle* particle);

      template <typename Particle>
      void leapfrogKickWithGlobalTimeStepSize(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeWorkedOnInVertexKernel(marker, localParticle)) {
          leapfrog_kick_global_time_step_size(&localParticle);
        }
      }


      /**
       * @brief Kick the additional variables
       *
       * Additional hydrodynamic quantites are kicked forward in time here. These
       * include thermal quantities (thermal energy or total energy or entropy, ...).
       *
       * Kick extra step, i.e. update some thermodynamic
       * quantities forward in time. In particular, U is integrated forward in
       * time treating it as the velocity.
       */
      template <typename Particle>
      void hydro_kick_extra_global_time_step_size(Particle* localParticle);

      template <typename Particle>
      void leapfrogKickExtraWithGlobalTimeStepSize(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeWorkedOnInVertexKernel(marker,localParticle)) {
          hydro_kick_extra_global_time_step_size(&localParticle);
        }
      }

      /**
       * @brief Sets the values to be predicted in the drifts to their values at a
       * kick time.
       */
      template <typename Particle>
      void hydro_reset_predicted_values(Particle* localParticle);

      template <typename Particle>
      void resetPredictedValues(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeWorkedOnInVertexKernel(marker,localParticle)) {
          hydro_reset_predicted_values(&localParticle);
        }
      }
    }
  } // namespace timestepping
} // namespace swift2


#include "Leapfrog.cpph"
