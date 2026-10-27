// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace examples::advection {
  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeLinearFlux(
    const Precision* NOALIAS Q,
    int                      normal,
    Precision* NOALIAS       F
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeDiagonalFlux(const Precision* NOALIAS Q, Precision* NOALIAS F);

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeRadialFlux(
    const Precision* NOALIAS                     Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          normal,
    Precision* NOALIAS                           F
  );

  template <class Shortcuts, typename Precision, typename Nodes>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS Q,
    const Nodes&             nodes,
    int                      numberOfValues,
    double                   detailTolerance
  );

  INLINE_METHOD GPU_CALLABLE_METHOD double getBump(
    const tarch::la::Vector<DIMENSIONS, double>& offsetFromCentre,
    double                                       radius,
    bool                                         smooth
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void writePeriodicBump(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       radius,
    bool                                         smooth,
    Precision* NOALIAS                           Q
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void writeLinear(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    Precision* NOALIAS                           Q
  );
} // namespace examples::advection

#include "examples/advection/Advection.cpph"
