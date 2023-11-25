// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>
#include <vector>

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

namespace exahype2::dg {
  struct PointSource {
    const tarch::la::Vector<Dimensions, double>& x;
    const double* __restrict__ values;
  };

  /**
   * Flux functor
   *
   * This functor defines the signature of the flux evaluation.
   *
   *
   * ## Comparison to finite volume flux
   *
   * The finite volume flux accepts a volume size h as well. For DG, we do not
   * need an h, as we actually work point-wisely: the flux is evaluated in integration
   * points and then scales an underlying shape function. So there's no need for any
   * spatial averaging, e.g. We still pass the h to be consistent with the finite volume
   * kernels.
   */
  typedef std::function<void(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  )>
    Flux;

  typedef std::function<void(
    const double* __restrict__ Q,
    const double* __restrict__ dQdx,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  )>
    NonConservativeProduct;

  typedef std::function<
    void(const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S)>
    Source;

  typedef std::function<
    std::vector<PointSource>(const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt)>
    PointSources;

  typedef std::function<
    double(const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal)>
    MaxEigenvalue;

  typedef std::function<void(
    const double* __restrict__ Qinside,
    double* __restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    int                                          normal
  )>
    BoundaryConditions;
} // namespace exahype2::dg
