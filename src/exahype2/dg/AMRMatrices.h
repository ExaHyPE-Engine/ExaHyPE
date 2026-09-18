// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/sfc/ActiveCurve.h"
#include "tarch/CompilerSpecificSettings.h"

/**
 * One-dimensional transfer matrices between a cell and its children for a
 * nodal Lagrange basis.
 *
 * Every matrix set holds one NodesPerAxis x NodesPerAxis matrix per child
 * interval, stored flat as [child][row][column]. The d-dimensional operators
 * result from tensor products of these one-dimensional matrices.
 */
namespace exahype2::dg {
  template <int NodesPerAxis>
  using AMRMatrices1D = std::
    array<double, static_cast<std::size_t>(peano4::sfc::ActiveCurve::SubdivisionFactor * NodesPerAxis * NodesPerAxis)>;

  template <int NodesPerAxis>
  INLINE_METHOD constexpr double lagrangeBasisFunction1D(const double* nodes, int basisFunction, double x);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> interpolationMatrix1D(const double* nodes);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> restrictionMatrix1D(const double* nodes);

  template <int NodesPerAxis>
  INLINE_METHOD constexpr AMRMatrices1D<NodesPerAxis> weightedRestrictionMatrix1D(
    const double* nodes,
    const double* weights
  );

  template <int NodesPerAxis>
  INLINE_METHOD constexpr void averageMirrorPairs(AMRMatrices1D<NodesPerAxis>& matrices);
} // namespace exahype2::dg

#include "exahype2/dg/AMRMatrices.cpph"
