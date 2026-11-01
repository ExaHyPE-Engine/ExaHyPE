// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace examples::acoustic {
  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeFlux(
    const Precision* NOALIAS Q,
    int                      normal,
    double                   density,
    double                   waveSpeed,
    Precision* NOALIAS       F
  );

  template <class Shortcuts, typename Precision, typename Nodes>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS Q,
    const Nodes&             nodes,
    int                      numberOfValues,
    double                   density,
    double                   waveSpeed,
    double                   detailTolerance
  );

  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommandOfFront(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       startRadius,
    bool                                         cellStoresData
  );

  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommandOfDisc(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  );

  INLINE_METHOD GPU_CALLABLE_METHOD bool isNearFiringSource(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t
  );

  template <class Shortcuts, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void reflectAtWall(
    const Precision* NOALIAS Qinside,
    Precision* NOALIAS       Qoutside,
    int                      normal
  );
} // namespace examples::acoustic

#include "examples/acoustic/Acoustic.cpph"
