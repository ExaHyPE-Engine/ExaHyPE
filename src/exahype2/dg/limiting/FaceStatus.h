// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/LimiterStatus.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::dg::limiting {
  template <typename CellLabel, typename FaceLabels>
  INLINE_METHOD void projectCellStatusOntoFaces(const CellLabel& cellLabel, const FaceLabels& faceLabels);

  template <typename FaceLabel>
  INLINE_METHOD void resetRefinedFaceStatus(FaceLabel& faceLabel);

  template <typename FaceLabel>
  INLINE_METHOD void snapshotFaceStatus(FaceLabel& faceLabel);

  template <typename FaceLabel, typename CoarseFaceLabels>
  INLINE_METHOD void copyCoarseFaceStatusOntoHangingFace(
    int                     faceNumber,
    FaceLabel&              faceLabel,
    const CoarseFaceLabels& coarseFaceLabels
  );

  template <typename FaceLabel, typename CoarseFaceLabels>
  INLINE_METHOD void raiseHangingFaceStatusOntoCoarseFace(
    int                     faceNumber,
    const FaceLabel&        faceLabel,
    const CoarseFaceLabels& coarseFaceLabels
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/FaceStatus.cpph"
