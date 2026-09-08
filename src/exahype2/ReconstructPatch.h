// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"
#include "exahype2/Enumeration.h"

namespace exahype2 {
  template <int NumberOfDofsPerAxis, int Overlap, int NumberOfUnknowns, typename Precision>
  INLINE_METHOD void reconstructPatch(
    const Precision* NOALIAS const inputPatch,
    Precision* NOALIAS const       faces[TWO_TIMES_D],
    Precision* NOALIAS const       reconstructedPatch
  );
} // namespace exahype2

#include "ReconstructPatch.cpph"
