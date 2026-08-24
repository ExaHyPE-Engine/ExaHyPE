// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"
#include "toolbox/blockstructured/Enumeration.h"

namespace toolbox::blockstructured {
  template <int NumberOfDofsPerAxis, int Overlap, int NumberOfUnknowns, typename Precision>
  INLINE_METHOD void projectPatch(
    const Precision* NOALIAS const inputPatch,
    Precision* NOALIAS const       faces[TWO_TIMES_D]
  );
} // namespace toolbox::blockstructured

#include "ProjectPatch.cpph"
