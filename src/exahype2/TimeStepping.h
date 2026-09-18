// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/FaceEnumerator.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/ScalarOperations.h"

namespace exahype2 {
  template <typename FaceLabel>
  double getMinTimeStampOfNeighbours(const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator);

  template <typename CellLabel, typename FaceLabel>
  double getMinTimeStampOfNeighboursAhead(
    const CellLabel&                                   cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator
  );

  template <typename CellLabel, typename FaceLabel>
  bool runTimeStepOnCell(
    const CellLabel&                                   cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator,
    double                                             globalMinTimeStamp
  );

  template <typename CellLabel, typename FaceLabel>
  double removeTimeStepAccumulationErrorsFromCell(
    const CellLabel&                                   cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>& faceLabelEnumerator,
    double                                             timeStepSize
  );

  std::pair<double, double> getInterpolationWeights(
    double oldTimeStampOnFace,
    double newTimeStampOnFace,
    double cellTimeStamp
  );

  double discretiseAndTruncateTimeStepSizes(
    double cellTimeStepSize,
    double maxGlobalTimeStepSize,
    int    discretisationStepsSize
  );
} // namespace exahype2

#include "exahype2/TimeStepping.cpph"
