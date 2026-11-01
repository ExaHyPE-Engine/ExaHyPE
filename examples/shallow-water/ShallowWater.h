// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace examples::shallowwater {
  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxEigenvalue(
    const Precision* NOALIAS Q,
    int                      normal,
    double                   gravity,
    double                   dryTolerance
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeFlux(
    const Precision* NOALIAS Q,
    int                      normal,
    double                   dryTolerance,
    Precision* NOALIAS       F
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeNonconservativeProduct(
    const Precision* NOALIAS Q,
    const Precision* NOALIAS deltaQ,
    int                      normal,
    double                   gravity,
    Precision* NOALIAS       BTimesDeltaQ
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double computeWellBalancedFlux(
    const Precision* NOALIAS QL,
    const Precision* NOALIAS QR,
    int                      normal,
    double                   gravity,
    double                   dryTolerance,
    Precision* NOALIAS       FL,
    Precision* NOALIAS       FR
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const Precision* NOALIAS Q,
    int                      numberOfStates,
    int                      numberOfValues
  );

  template <class Shortcuts, typename Precision, typename Nodes>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS Q,
    const Nodes&             nodes,
    int                      numberOfValues,
    double                   detailTolerance
  );

  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommandOfStep(
    const tarch::la::Vector<DIMENSIONS, double>& x
  );
} // namespace examples::shallowwater

#include "examples/shallow-water/ShallowWater.cpph"
