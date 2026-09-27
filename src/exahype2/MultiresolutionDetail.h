// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2 {
  /**
   * Hysteresis ratio between the refine and the erase threshold of the detail
   * criterion: The detail of an affine fit shrinks by this factor from one
   * level to the next.
   */
  constexpr double DefaultCoarsenSafetyFactor{peano4::power(peano4::SubdivisionFactor, 2)};

  INLINE_METHOD GPU_CALLABLE_METHOD RefinementCommand getRefinementCommandFromDetail(
    double detail,
    double eps                 = 1.0e-2,
    double coarsenSafetyFactor = exahype2::DefaultCoarsenSafetyFactor
  );
} // namespace exahype2
TARGET_DECLARE_END

namespace exahype2 {
  template <std::size_t NumberOfVolumesPerAxis>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD constexpr std::array<double, NumberOfVolumesPerAxis>
  getVolumeCentresInUnitInterval();

  template <std::size_t NumberOfNodesPerAxis, typename Value, typename Indicator>
  INLINE_METHOD GPU_CALLABLE_METHOD double computeMultiresolutionDetail(
    const Value* NOALIAS                            Q,
    const std::array<double, NumberOfNodesPerAxis>& nodesInUnitInterval,
    int                                             numberOfUnknownsPlusAuxiliaryVariables,
    const Indicator&                                indicator,
    double                                          floor = 1.0e-12
  );

  template <std::size_t NumberOfNodesPerAxis, typename Value, typename... Indicators>
  INLINE_METHOD GPU_CALLABLE_METHOD RefinementCommand refineOnDetail(
    const Value*                                    Q,
    const std::array<double, NumberOfNodesPerAxis>& nodesInUnitInterval,
    int                                             numberOfUnknownsPlusAuxiliaryVariables,
    double                                          eps,
    double                                          coarsenSafetyFactor,
    const Indicators&... indicators
  );
} // namespace exahype2

#include "exahype2/MultiresolutionDetail.cpph"
