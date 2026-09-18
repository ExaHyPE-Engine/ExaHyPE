// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <typename QuadraturePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getQuadraturePoint(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const tarch::la::Vector<DIMENSIONS, int>&    index,
    const QuadraturePrecision* NOALIAS           quadraturePoints
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

namespace exahype2::dg {
  template <typename StoragePrecision>
  INLINE_METHOD StoragePrecision evaluatePolynomial(
    const peano4::dataview::CellMarker&          marker,
    int                                          order,
    const StoragePrecision* NOALIAS              quadratureNodes,
    int                                          unknownsPerNode,
    const StoragePrecision* NOALIAS              values,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          unknown
  );

  template <typename StoragePrecision>
  INLINE_METHOD StoragePrecision evaluatePolynomial(
    const peano4::dataview::FaceMarker&          marker,
    int                                          order,
    const StoragePrecision* NOALIAS              quadratureNodes,
    int                                          unknownsPerNode,
    const StoragePrecision* NOALIAS              values,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    int                                          unknown
  );
} // namespace exahype2::dg

#include "exahype2/dg/DGUtils.cpph"
