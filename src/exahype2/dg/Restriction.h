// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

/**
 * @file
 * The restriction kernels the discontinuous Galerkin families use.
 *
 * Entry [b * n * n + fine * n + coarse] of a restriction table weights the fine node of child
 * interval b against the coarse node it feeds.
 */
namespace exahype2::dg {
  template <typename T>
  class FaceRestriction;

  template <typename T>
  class CellRestriction;
} // namespace exahype2::dg

/**
 * Restricts one side of a face from the subfaces staged below it. The key may alias the face
 * values.
 */
template <typename T>
class exahype2::dg::FaceRestriction final {
public:
  INLINE_METHOD FaceRestriction(const double* restrictionMatrix1D, int numberOfDofsPerAxis, int numberOfValuesPerNode);

  INLINE_METHOD void operator()(
    exahype2::amr::ChildDataStage<T>& stage,
    const void*                       coarseFaceKey,
    T*                                coarseFaceValues
  ) const;

private:
  const double* const _restrictionMatrix1D;
  const int           _numberOfDofsPerAxis;
  const int           _numberOfValuesPerNode;
};

/**
 * Restricts a complete sibling set into a cell through the weighted operator.
 */
template <typename T>
class exahype2::dg::CellRestriction final {
public:
  INLINE_METHOD CellRestriction(const double* restrictionMatrix1D, int numberOfDofsPerAxis, int numberOfValuesPerNode);

  INLINE_METHOD void operator()(exahype2::amr::ChildDataStage<T>& stage, T* NOALIAS coarseGridValues) const;

private:
  const double* const _restrictionMatrix1D;
  const int           _numberOfDofsPerAxis;
  const int           _numberOfValuesPerNode;
};

#include "exahype2/dg/Restriction.cpph"
