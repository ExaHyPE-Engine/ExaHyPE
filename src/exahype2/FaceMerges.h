// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "peano4/Loop.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2 {
  template <int DofsPerAxis, int Overlap, int NumberOfUnknowns, typename Precision>
  INLINE_METHOD void mergeOverlappingVolumes(
    const peano4::dataview::FaceMarker& marker,
    Precision* const                    destination,
    const Precision* const              source
  );

  template <int Cardinality, typename Precision>
  INLINE_METHOD void mergeProjectionHalves(
    const peano4::dataview::FaceMarker& marker,
    Precision* const                    destination,
    const Precision* const              source
  );
} // namespace exahype2

#include "exahype2/FaceMerges.cpph"
