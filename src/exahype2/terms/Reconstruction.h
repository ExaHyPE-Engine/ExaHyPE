// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasReconstruction = requires(
    const Precision* const                        QL,
    const Precision* const                        Q,
    const Precision* const                        QR,
    const Precision* const                        limitedDifferences,
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const tarch::la::Vector<DIMENSIONS, double>&  h,
    const int                                     normal,
    const int                                     unknown,
    Precision&                                    reconstructedMinus,
    Precision&                                    reconstructedPlus,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    {
      SolverType::reconstruct(
        QL,
        Q,
        QR,
        limitedDifferences,
        x,
        h,
        normal,
        unknown,
        reconstructedMinus,
        reconstructedPlus,
        parameters
      )
    } -> std::same_as<void>;
  };
} // namespace exahype2::terms
