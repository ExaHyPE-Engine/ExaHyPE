// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ProjectPatch.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::fv {
  template <
    int NumberOfDofsPerAxis,
    int Overlap,
    int NumberOfUnknowns,
    typename Precision,
    typename FaceLabelEnumerator,
    typename FaceDataEnumerator>
  INLINE_METHOD void projectPatchOntoFaces(
    const FaceLabelEnumerator& faceLabels,
    const Precision*           patch,
    const FaceDataEnumerator&  faceData,
    double                     timeStamp
  );
} // namespace exahype2::fv

#include "exahype2/fv/ProjectPatchOntoFaces.cpph"
