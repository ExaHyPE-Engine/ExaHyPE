// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2::terms {
  template <typename SolverType, typename Particle>
  concept HasParticleAnalyticalSolution = requires(
    Particle* const                               particle,
    const double                                  t,
    const double                                  dt,
    double* const                                 solution,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    { SolverType::analyticalSolution(particle, t, dt, solution, parameters) } -> std::same_as<void>;
  };
} // namespace exahype2::terms
