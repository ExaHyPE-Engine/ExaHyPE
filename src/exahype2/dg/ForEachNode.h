// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/DGUtils.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <int NodesPerAxis, int Values, typename Precision, typename QuadraturePrecision, typename Functor>
  INLINE_METHOD GPU_CALLABLE_METHOD void forEachNode(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const QuadraturePrecision*                   quadraturePoints,
    const QuadraturePrecision*                   quadratureWeights,
    Precision*                                   values,
    Functor&&                                    functor
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/ForEachNode.cpph"
