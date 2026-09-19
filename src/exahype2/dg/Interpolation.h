// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

/**
 * @file
 * The mesh transfer kernels the discontinuous Galerkin families use.
 *
 * A nodal patch transfers through the operator tables of its basis. Entry
 * [b * n * n + coarse * n + fine] of a prolongation table is the coarse shape function at
 * the fine node of child interval b.
 */
namespace exahype2::dg {
  template <typename T>
  class FaceProlongation;

  template <typename T>
  class CellProlongation;

  template <typename T>
  class NodalCellInterpolation;
} // namespace exahype2::dg

/**
 * Prolongs one side of a coarse face onto the subface hanging below it.
 */
template <typename T>
class exahype2::dg::FaceProlongation final {
public:
  INLINE_METHOD FaceProlongation(
    const double* prolongationMatrix1D,
    int           numberOfDofsPerAxis,
    int           numberOfValuesPerNode
  );

  INLINE_METHOD void operator()(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseFaceValues,
    T* NOALIAS                          fineFaceValues,
    int                                 side
  ) const;

private:
  const double* const _prolongationMatrix1D;
  const int           _numberOfDofsPerAxis;
  const int           _numberOfValuesPerNode;
};

/**
 * Prolongs a coarse cell into one child through a separable operator.
 */
template <typename T>
class exahype2::dg::CellProlongation final {
public:
  INLINE_METHOD CellProlongation(
    const double* prolongationMatrix1D,
    int           numberOfDofsPerAxis,
    int           numberOfValuesPerNode
  );

  INLINE_METHOD void operator()(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseGridValues,
    T* NOALIAS                          fineGridValues
  ) const;

private:
  const double* const _prolongationMatrix1D;
  const int           _numberOfDofsPerAxis;
  const int           _numberOfValuesPerNode;
};

/**
 * Prolongs a coarse cell into one child through the nodal interpolation operator.
 */
template <typename T>
class exahype2::dg::NodalCellInterpolation final {
public:
  INLINE_METHOD NodalCellInterpolation(
    const double* interpolationMatrix1D,
    int           numberOfDofsPerAxis,
    int           numberOfValuesPerNode
  );

  INLINE_METHOD void operator()(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseGridValues,
    T* NOALIAS                          fineGridValues
  ) const;

private:
  const double* const _interpolationMatrix1D;
  const int           _numberOfDofsPerAxis;
  const int           _numberOfValuesPerNode;
};

#include "exahype2/dg/Interpolation.cpph"
