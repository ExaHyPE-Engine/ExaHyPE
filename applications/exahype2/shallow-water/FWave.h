// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "WetDryState.h"
#include "VariableShortcuts.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::swe {
  /**
   ****
   **** F-Wave Riemann Solver for the Shallow Water Equation
   ****
   * Note:
   * This solver does not support inundation. For dry cells, we assume an infinitely high solid wall
   * at the shore-line, which is equivalent to reflecting boundary conditions. Thus no water is
   * able to pass the shoreline, and we have to assure that the particle velocity at the boundary is
   * zero.
   *
   * Literature:
   *
   *   @article{bale2002wave,
   *            title={A wave propagation method for conservation laws and balance laws with spatially varying flux functions},
   *            author={Bale, D.S. and LeVeque, R.J. and Mitran, S. and Rossmanith, J.A.}, journal={SIAM Journal on Scientific Computing},
   *            volume={24},
   *            number={3},
   *            pages={955--978},
   *            year={2002},
   *            publisher={Citeseer}}
   *
   *   @book{leveque2002finite,
   *         Author = {LeVeque, R. J.},
   *         Date-Added = {2011-09-13 14:09:31 +0000},
   *         Date-Modified = {2011-10-31 09:46:40 +0000},
   *         Publisher = {Cambridge University Press},
   *         Title = {Finite Volume Methods for Hyperbolic Problems},
   *         Volume = {31},
   *         Year = {2002}}
   *
   *   @webpage{levequeclawpack,
   *            Author = {LeVeque, R. J.},
   *            Lastchecked = {January, 05, 2011},
   *            Title = {Clawpack Sofware},
   *            Url = {https://github.com/clawpack/clawpack-4.x/blob/master/geoclaw/2d/lib}}
   ****
   */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  double fWave(
    const double* const __restrict__             QR,
    const double* const __restrict__             QL,
    const double* const __restrict__             FR,
    const double* const __restrict__             FL,
    const double* const __restrict__             LR,
    const double* const __restrict__             LL,
    const tarch::la::Vector<Dimensions, double>& xR,
    const tarch::la::Vector<Dimensions, double>& xL,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   APDQ,
    double* const __restrict__                   AMDQ
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::swe

#include "FWave.cpph"
