// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace applications::exahype2::exaskred::limiting {
  template <typename T>
  struct CellStatistics {
    bool finite         = true;
    T    minHeight      = std::numeric_limits<T>::max();
    T    maxHeight      = std::numeric_limits<T>::lowest();
    T    minBathymetry  = std::numeric_limits<T>::max();
    T    maxBathymetry  = std::numeric_limits<T>::lowest();
    T    minFreeSurface = std::numeric_limits<T>::max();
    T    maxFreeSurface = std::numeric_limits<T>::lowest();
    T    maxMomentum    = T{};
  };

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD CellStatistics<T> collectCellStatistics(const T* const Q, const int numberOfStates);

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isWetCell(
    const T* const Q,
    const int      numberOfStates,
    const double   minimumWaterHeight
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool hasBoundedDepthVariation(
    const T* const Q,
    const int      numberOfStates,
    const double   minimumWaterHeight,
    const double   maximumRelativeDepthVariation
  );

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleState(const T* const Q, const double minimumWaterHeight);

  template <class Shortcuts, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleCell(
    const T* const Q,
    const int      numberOfStates,
    const double   minimumWetHeight
  );
} // namespace applications::exahype2::exaskred::limiting

#include "Limiting.cpph"
