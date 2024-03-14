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
       *
       * It's important to take the "V_full" here, not the fluid velocity V.
       * V_full doesn't get touched outside of kicks. V will be interpolated/
       * integrated in time as the time step progresses. V_full is the correct
       * choice here.
       *
       * Note that original SWIFT predicts the velocities at this point. We
       * moved that into hydroPredictExtra().
       */
      template <typename Particle>
      void leapfrog_drift_global_time_step_size(Particle* particle);

      template <typename Particle>
      void leapfrogDriftWithGlobalTimeStepSize(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedAndMovedInVertexKernel(marker, localParticle)) {
          leapfrog_drift_global_time_step_size(&localParticle);
        }
      }


      /**
       * @brief Predict additional particle fields forward in time when drifting
       *
       * Additional hydrodynamic quantites are drifted forward in time here. These
       * include thermal quantities (thermal energy or total energy or entropy, ...).
       * Note that which and how quantities get updated depends on the specific SPH
       * flavour.
       */
      template <typename Particle>
      void hydro_predict_extra(Particle* localParticle);

      template <typename Particle>
      void hydroPredictExtra(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_predict_extra(&localParticle);
        }
      }


      /**
       * @todo No masking yet
       */
      template <typename Particle>
      void hydroPredictExtraWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        hydroPredictExtra(marker, localParticle);
      }


      /**
       * @todo No masking in here yet
       */
      template <typename Particle>
      void leapfrogDriftWithGlobalTimeStepSizeWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        leapfrogDriftWithGlobalTimeStepSize(marker, localParticle);
      }

      /**
       *  Kick steps. There are two kick steps in the KDK scheme, in which we
       *  update the particle velocity by a half-timestep using:
       *
       *  Kick1:
       *  @f$ {\bf v}^{n+\frac{1}{2}} = {\bf v}^{n} + {\bf a}^{n}\frac{\Delta t}{2} @f$
       *
       *  Kick2:
       *  @f$ {\bf v}^{n+1} = {\bf v}^{n+\frac{1}{2}} + {\bf a}^{n+1}\frac{\Delta t}{2} @f$
       *
       * Additionally, thermal quantities are being updated. In the case of "Minimal"
       * SPH, we update
       *
       * Kick1:
       *  @f$ {u}^{n+\frac{1}{2}} = u^{n} + \frac{\partial u}{\partial t}^{n}\frac{\Delta t}{2} @f$
       *
       * Kick2:
       *  @f$ {u}^{n+1} = u^{n+\frac{1}{2}} + \frac{\partial u}{\partial t}^{n+1}\frac{\Delta t}{2} @f$
       *
       * The update of thermal quantities has been separated into hydro_kick_extra_global_time_step_size
       * as it depends on the SPH flavour used, and needs to be replaceable.
       *
       * Notice that the last kick in this sequence is implicit if @f${\bf a}@f$
       * depends on @f${\bf v}@f$ (e.g. due to artificial viscosity terms), which in
       * principle requires extra treatments (see e.g. Price et al. 2017).
       *
       * While the time integration scheme should be kick1 - drift - kick2, we do a
       * slightly different sequence. First, we do a kick1 during initialization.
       * Then every subsequent normal simulation step does a drift - kick2 - kick1
       * sequence. The reason for that is to permit individual time step sizes for
       * individual particles: By starting the particle time step with a drift, we
       * can always drift neighbours to their correct positions, even when they
       * are inactive (= not being updated in the current step). The drifts are
       * linear operations which in principle can be concatenated in any number,
       * provided the total time a particle is being drifted sums up to the correct
       * value. So for example, it should be equivalent to compute
       *
       * @f$ K_1(\Delta t/2) \circ D(\Delta t) \circ K_2(\Delta t/2) @f$
       *
       * or
       *
       * @f$ K_1(\Delta t/2) \circ D(\Delta t/4) \circ D(\Delta t/4) \circ D(\Delta t/4)
       * \circ D(\Delta t/4) \circ K_2(\Delta t/2) @f$.
       *
       *
       * Once the kick2 operation is complete, the "predicted" values stemming from
       * the "prediction" step in hydro_predict_extra() after a drift need to be
       * reset to up-to-date current values that the particle carries. So after a kick2,
       * we reset them using hydro_reset_predicted_values().
       *
       * So in total, there are two additional steps that you have to add afterwards.
       *
       * This version is tailored towards the SPH time integration, where we access additional
       * particle fields which are otherwise not present, such as the time derivative
       * of the internal energy.
       */
      template <typename Particle>
      void leapfrog_kick_global_time_step_size(Particle* particle);

      template <typename Particle>
      void leapfrogKickWithGlobalTimeStepSize(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker, localParticle)) {
          leapfrog_kick_global_time_step_size(&localParticle);
        }
      }

      /**
       * @todo No maksing
       */
      template <typename Particle>
      void leapfrogKickWithGlobalTimeStepSizeWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        leapfrogKickWithGlobalTimeStepSize(marker, localParticle);
      }


      /**
       * @brief Kick the additional variables
       *
       * Additional hydrodynamic quantites are kicked forward in time here. These
       * include thermal quantities (thermal energy or total energy or entropy, ...).
       * The additional quantities being updated here depend on the exact SPH flavour
       * being used.
       *
       * Kick extra step, i.e. update some thermodynamic
       * quantities forward in time. In particular, U is integrated forward in
       * time treating it as the velocity.
       */
      template <typename Particle>
      void hydro_kick_extra_global_time_step_size(Particle* localParticle);

      template <typename Particle>
      void leapfrogKickExtraWithGlobalTimeStepSize(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_kick_extra_global_time_step_size(&localParticle);
        }
      }

      /**
       * @Todo No masking yet
       */
      template <typename Particle>
      void leapfrogKickExtraWithGlobalTimeStepSizeWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        leapfrogKickExtraWithGlobalTimeStepSize(marker, localParticle);
      }

      /**
       * @brief Sets the values to be predicted in the drifts to their values at a
       * kick time.
       */
      template <typename Particle>
      void hydro_reset_predicted_values(Particle* localParticle);

      template <typename Particle>
      void resetPredictedValues(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
          hydro_reset_predicted_values(&localParticle);
        }
      }

      /**
       * @todo No masking yet
       */
      template <typename Particle>
      void resetPredictedValuesWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
        resetPredictedValues(marker, localParticle);
      }
    }
  } // namespace timestepping
} // namespace swift2


#include "Leapfrog.cpph"
