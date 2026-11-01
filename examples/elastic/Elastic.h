// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace examples::elastic {
  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxEigenvalue(const Precision* NOALIAS Q);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeFlux(const Precision* NOALIAS Q, int normal, Precision* NOALIAS F);

  template <class Shortcuts, typename Precision, typename Nodes>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS Q,
    const Nodes&             nodes,
    int                      numberOfValues,
    double                   detailTolerance
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD void writeMedium(Precision* NOALIAS Q);
} // namespace examples::elastic

#include "examples/elastic/Elastic.cpph"
