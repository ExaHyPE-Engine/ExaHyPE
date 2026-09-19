// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/RepositoryCallbacks.h"
#include "tarch/CompilerSpecificSettings.h"

namespace runner {
  INLINE_METHOD SolverStatistic makeValueStatistic(const char* label, double value);
  INLINE_METHOD SolverStatistic makeRangeStatistic(const char* label, double minimum, double maximum);
  INLINE_METHOD SolverStatistic makeCountStatistic(const char* label, long value);

  template <typename Solver, typename AppendAdditionalStatistics>
  INLINE_METHOD std::pair<std::string, std::vector<SolverStatistic>> makeSolverStatistics(
    const char*                       solverName,
    const Solver&                     solver,
    const AppendAdditionalStatistics& appendAdditionalStatistics
  );

  INLINE_METHOD std::pair<std::string, std::vector<SolverStatistic>> makeParticleStatistics(
    const char* particleName,
    int         numberOfParticles
  );
} // namespace runner

#include "runner/SolverStatistics.cpph"
