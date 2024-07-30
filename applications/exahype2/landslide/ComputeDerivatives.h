// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::landslide {
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  void computeDerivatives(
    double* __restrict__ oldQWithHalo,
    int numberOfDofs,
    int numberOfUnknowns,
    int numberOfAuxiliaryVariables
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
} // namespace applications::exahype2::landslide
