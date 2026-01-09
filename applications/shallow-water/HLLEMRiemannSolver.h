// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  /**
   ****
   **** A new formulation of the HLLEM Riemann solver for the Shallow Water Equation with support for inundation
   ****
   * Literature:
   *
   *   @article{Dumbser2016,
   *            title = {A new efficient formulation of the HLLEM Riemann solver for general conservative and
   *non-conservative hyperbolic systems}, journal = {Journal of Computational Physics}, volume = {304}, pages =
   *{275-319}, year = {2016}, issn = {0021-9991}, doi = {https://doi.org/10.1016/j.jcp.2015.10.014}, url =
   *{https://www.sciencedirect.com/science/article/pii/S0021999115006786}, author = {Michael Dumbser and Dinshaw S.
   *Balsara}}
   *
   *   @article{Kurganov2007,
   *            title = {A Second-Order Well-Balanced Positivity Preserving Central-Upwind Scheme for the Saint-Venant
   *System}, year = {2007}, month = {03}, pages = {}, volume = {5}, journal = {Communications in mathematical sciences},
   *            author = {Kurganov, Alexander and Petrova, G.},
   *            doi = {10.4310/CMS.2007.v5.n1.a6}}
   ****
   */
  template <class T, class Shortcuts>
  static inline GPU_CALLABLE_METHOD double hllemRiemannSolver(
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

#include "HLLEMRiemannSolver.cpph"
