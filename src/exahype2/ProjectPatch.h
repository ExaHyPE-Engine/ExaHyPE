// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2 {
  template <int NumberOfDofsPerAxis, int Overlap, int NumberOfUnknowns, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void projectPatch(
    const Precision* NOALIAS inputPatch,
    Precision* NOALIAS const faces[peano4::FacesPerCell]
  );
} // namespace exahype2
TARGET_DECLARE_END

#include "exahype2/ProjectPatch.cpph"
