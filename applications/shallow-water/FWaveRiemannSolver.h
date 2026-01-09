// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "WetDryState.h"

namespace applications::exahype2::ShallowWater {
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
  *   @article{Bale2002,
  *            title={A wave propagation method for conservation laws and balance laws with spatially varying flux
  *functions}, author={Bale, D.S. and LeVeque, R.J. and Mitran, S. and Rossmanith, J.A.}, journal={SIAM Journal on
  *Scientific Computing}, volume={24}, number={3}, pages={955--978}, year={2002}, publisher={Citeseer}}
  *
  *   @book{LeVeque2002,
  *         Author = {LeVeque, R. J.},
  *         Date-Added = {2011-09-13 14:09:31 +0000},
  *         Date-Modified = {2011-10-31 09:46:40 +0000},
  *         Publisher = {Cambridge University Press},
  *         Title = {Finite Volume Methods for Hyperbolic Problems},
  *         Volume = {31},
  *         Year = {2002}}
  *
  *   @webpage{LeVeque2011,
  *            Author = {LeVeque, R. J.},
  *            Lastchecked = {January, 05, 2011},
  *            Title = {Clawpack Sofware},
  *            Url = {https://github.com/clawpack/clawpack-4.x/blob/master/geoclaw/2d/lib}}
  ****
  */
  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD double fWaveRiemannSolver(
    const T* const NOALIAS                       QL,
    const T* const NOALIAS                       QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    T* const NOALIAS                             FL,
    T* const NOALIAS                             FR
  );
} // namespace applications::exahype2::ShallowWater

#include "FWaveRiemannSolver.cpph"
