// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::riemann {
  template <
    auto Bathymetry,
    typename FaceStates,
    typename FaceFluxes,
    typename MaxEigenvalue,
    typename InterfaceGravity
  >
  INLINE_METHOD GPU_CALLABLE_METHOD double wellBalancedRusanov(
    const FaceStates&                            QL,
    const FaceStates&                            QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    MaxEigenvalue&&                              maxEigenvalue,
    InterfaceGravity&&                           interfaceGravity,
    FaceFluxes&                                  FL,
    FaceFluxes&                                  FR
  );
} // namespace applications::exahype2::exaskred::riemann

#include "WellBalancedRusanov.cpph"
