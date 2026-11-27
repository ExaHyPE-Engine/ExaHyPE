// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace applications::exahype2::exaskred::limiting {
  template <typename T>
  struct CellStatistics;

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD CellStatistics<decltype(State::h)> collectCellStatistics(std::span<const State> Q);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isWetCell(std::span<const State> Q, double minimumWaterHeight);

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool hasBoundedDepthVariation(
    std::span<const State> Q,
    double                 minimumWaterHeight,
    double                 maximumRelativeDepthVariation
  );

  template <typename State>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissibleState(const State& Q, double minimumWaterHeight);
} // namespace applications::exahype2::exaskred::limiting

template <typename T>
struct applications::exahype2::exaskred::limiting::CellStatistics final {
  bool finite{true};
  T    minHeight{std::numeric_limits<T>::max()};
  T    maxHeight{std::numeric_limits<T>::lowest()};
  T    minBathymetry{std::numeric_limits<T>::max()};
  T    maxBathymetry{std::numeric_limits<T>::lowest()};
  T    minFreeSurface{std::numeric_limits<T>::max()};
  T    maxFreeSurface{std::numeric_limits<T>::lowest()};
  T    maxMomentum{};
};

#include "Limiting.cpph"
