// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/aderdg/KernelUtils.h"

namespace exahype2::aderdg::basis::legendre {
  /**
   * Returns the exact (point-wise) interpoland for position for x
   *
   * If you interpolate onto a equidistant grid, you can alternatively use the
   * array equidistantGridProjector1d that holds all mapping quantities in a
   * precomputed table and thus is faster.
   *
   * @param offsetOfPatch Array of doubles of size Dimensions. If you use
   *          Peano's tarch::la::Vector class, apply its data() function to
   *          get hold of a pointer to be passed into this function
   * @param x Position where to evaluate the function. This has to be a point
   *          within the patch
   * @param numberOfUnknowns Number of unknowns held per grid point withint the
   *          patch
   * @param unknown Which unknown to evaluate
   * @param order   Which order is used
   * @param u       Pointer to unknowns
   */
  double interpolate(const double* offsetOfPatch, const double* sizeOfPatch, const double* x, int numberOfUnknowns, int unknown, int order, const double* u);

#include "GaussLegendreBasis.hsnippet"
} // namespace exahype2::aderdg::basis::legendre
