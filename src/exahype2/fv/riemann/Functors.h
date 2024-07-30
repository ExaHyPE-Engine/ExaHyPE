// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

namespace exahype2::fv::riemann {
  using SourceFunctor = std::function<void(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    double* __restrict__ S
  )>;

  using FluxFunctor = std::function<void(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  )>;

  using NonconservativeProductFunctor = std::function<void(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ BTimesDeltaQ
  )>;

  using EigenvaluesFunctor = std::function<void(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ L
  )>;

  using RiemannFunctor = std::function<double(
    const double* __restrict__ QR,
    const double* __restrict__ QL,
    const double* __restrict__ FR,
    const double* __restrict__ FL,
    const double* __restrict__ LR,
    const double* __restrict__ LL,
    const tarch::la::Vector<Dimensions, double>& xR,
    const tarch::la::Vector<Dimensions, double>& xL,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ APDQ,
    double* __restrict__ AMDQ
  )>;
} // namespace exahype2::fv::riemann
