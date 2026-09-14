// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonlinearWeights.h"

namespace {
  template <typename Weighting, typename ComputePrecision>
  [[maybe_unused]] double instantiateWeighting() {
    const ComputePrecision idealWeights[exahype2::fv::weno::NumberOfCandidateStencils]         = {};
    const ComputePrecision smoothnessIndicators[exahype2::fv::weno::NumberOfCandidateStencils] = {};
    ComputePrecision       nonlinearWeights[exahype2::fv::weno::NumberOfCandidateStencils]     = {};
    Weighting::weights(idealWeights, smoothnessIndicators, nonlinearWeights);
    return static_cast<double>(nonlinearWeights[0]);
  }

  [[maybe_unused]] double instantiateWeightings() {
    double result = 0.0;
    result += instantiateWeighting<exahype2::fv::weno::JiangShu, float>();
    result += instantiateWeighting<exahype2::fv::weno::JiangShu, double>();
    result += instantiateWeighting<exahype2::fv::weno::JiangShu, long double>();
    return result;
  }
} // namespace
