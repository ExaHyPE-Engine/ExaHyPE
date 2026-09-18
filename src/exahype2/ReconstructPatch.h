// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  template <int NumberOfDofsPerAxis, int Overlap, int NumberOfUnknowns, typename Precision>
  INLINE_METHOD void reconstructPatch(
    const Precision* NOALIAS inputPatch,
    Precision* NOALIAS const faces[peano4::FacesPerCell],
    Precision* NOALIAS       reconstructedPatch
  );
} // namespace exahype2

#include "exahype2/ReconstructPatch.cpph"
