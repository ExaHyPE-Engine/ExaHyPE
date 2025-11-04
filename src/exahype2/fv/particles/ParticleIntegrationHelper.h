// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#pragma once

#include "ParticleHelper.h"
#include "tarch/la/Vector.h"

using namespace exahype2::fv::particles::utils;

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace integration {

        /**
         * Compute a forward step using the current state:
         *
         * vₙ₊₁ = vₙ + stepFraction · Δt · aₙ
         * xₙ₊₁ = xₙ + stepFraction · Δt · vₙ
         */
        ParticleState updatePartialStep(const ParticleState& current, double timeStepSize, double stepFraction);

        /**
         * Combine two states by advancing the base using the update state:
         *
         * vₙ₊₁ = v_base + factor · Δt · a_update
         * xₙ₊₁ = x_base + factor · Δt · v_update
         */
        ParticleState updatePartialStepWithBase(
          const ParticleState& base,
          const ParticleState& update,
          double               timeStepSize,
          double               factor
        );

        /**
         * Compute a forward step using the current state:
         *
         * vₙ₊₁ = vₙ + stepFraction · Δt · aₙ
         * xₙ₊₁ = xₙ + stepFraction · Δt · vₙ₊₁
         */
        ParticleState updateMidpointStep(const ParticleState& current, double timeStepSize, double stepFraction);


      } // namespace integration
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleIntegrationHelper.cpph"
