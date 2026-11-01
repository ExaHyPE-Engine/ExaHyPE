// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace examples::euler {
  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getMomentumSquared(const Precision* NOALIAS Q);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getPressure(const Precision* NOALIAS Q, double gamma);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxEigenvalue(const Precision* NOALIAS Q, int normal, double gamma);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeFlux(
    const Precision* NOALIAS Q,
    int                      normal,
    double                   gamma,
    Precision* NOALIAS       F
  );

  template <class Shortcuts, typename Precision, typename Nodes>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS Q,
    const Nodes&             nodes,
    int                      numberOfValues,
    double                   gamma,
    double                   detailTolerance
  );

  template <typename Precision, typename Deviation>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommandOfDeviation(
    const Precision* NOALIAS Q,
    int                      numberOfStates,
    int                      numberOfValues,
    double                   refineTolerance,
    double                   eraseTolerance,
    const Deviation&         deviationOfState
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const Precision* NOALIAS Q,
    int                      numberOfStates,
    int                      numberOfValues,
    double                   gamma
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void mapDmpObservables(
    Precision* NOALIAS       observables,
    const Precision* NOALIAS Q,
    double                   gamma
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void reflectAtWall(
    const Precision* NOALIAS Qinside,
    Precision* NOALIAS       Qoutside,
    int                      normal
  );
} // namespace examples::euler

#include "examples/euler/Euler.cpph"
