// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleHelper.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::particles::integration {
  /**
   * Returns the state with
   * v_new = v + stepFraction * timeStepSize * a and
   * x_new = x + stepFraction * timeStepSize * v.
   */
  exahype2::fv::particles::utils::ParticleState updatePartialStep(
    const exahype2::fv::particles::utils::ParticleState& current,
    double                                               timeStepSize,
    double                                               stepFraction
  );

  /**
   * Returns the state with
   * v_new = v of base + factor * timeStepSize * a of update and
   * x_new = x of base + factor * timeStepSize * v of update.
   */
  exahype2::fv::particles::utils::ParticleState updatePartialStepWithBase(
    const exahype2::fv::particles::utils::ParticleState& base,
    const exahype2::fv::particles::utils::ParticleState& update,
    double                                               timeStepSize,
    double                                               factor
  );

  /**
   * Returns the state with
   * v_new = v + stepFraction * timeStepSize * a and
   * x_new = x + stepFraction * timeStepSize * v_new.
   */
  exahype2::fv::particles::utils::ParticleState updateMidpointStep(
    const exahype2::fv::particles::utils::ParticleState& current,
    double                                               timeStepSize,
    double                                               stepFraction
  );
} // namespace exahype2::fv::particles::integration

#include "exahype2/fv/particles/ParticleIntegrationHelper.cpph"
