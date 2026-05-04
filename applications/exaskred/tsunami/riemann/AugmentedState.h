// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace applications::exahype2::exaskred::riemann {
  /**
   * A implementation of the augmented state Riemann solver from David L. George modified for 2D scenarios
   * and for compatibility for the ExaHyPE 2 project
   *
   * Main scientific source for the implementation:
   * @article{George2008,
   *      title = {Augmented Riemann solvers for the shallow water equations over variable topography with steady states
   * and inundation}, journal = {Journal of Computational Physics}, volume = {227}, number = {6}, pages = {3089-3113},
   *      year = {2008},
   *      issn = {0021-9991},
   *      doi = {https://doi.org/10.1016/j.jcp.2007.10.027},
   *      url = {https://www.sciencedirect.com/science/article/pii/S0021999107004767},
   *      author = {David L. George},
   *      keywords = {Shallow water equations, Hyperbolic conservation laws, Finite volume methods, Godunov methods,
   *      Riemann solvers, Wave propagation, Shock capturing methods, Tsunami modeling},
   *      abstract = {We present a class of augmented approximate Riemann solvers for the shallow water equations
   *      in the presence of a variable bottom surface. These belong to the class of simple approximate solvers that use
   *      a set of propagating jump discontinuities, or waves, to approximate the true Riemann solution. Typically,
   *      a simple solver for a system of m conservation laws uses m such discontinuities. We present a four wave solver
   *      for use with the the shallow water equations—a system of two equations in one dimension. The solver is based
   * on a decomposition of an augmented solution vector—the depth, momentum as well as momentum flux and bottom surface.
   *      By decomposing these four variables into four waves the solver is endowed with several desirable properties
   *      simultaneously. This solver is well-balanced: it maintains a large class of steady states by the use of a
   *      properly defined steady state wave—a stationary jump discontinuity in the Riemann solution that acts as a
   * source term. The form of this wave is introduced and described in detail. The solver also maintains depth
   *      non-negativity and extends naturally to Riemann problems with an initial dry state. These are important
   *      properties for applications with steady states and inundation, such as tsunami and flood modeling.
   * Implementing the solver with LeVeque’s wave propagation algorithm [R.J. LeVeque, Wave propagation algorithms for
   *      multi-dimensional hyperbolic systems, J. Comput. Phys. 131 (1997) 327–335] is also described. Several
   * numerical simulations are shown, including a test problem for tsunami modeling.}
   *  }
   *
   * Additional information about the solver can be found here:
   * @book{George2006,
   *      title={Finite volume methods and adaptive refinement for tsunami propagation and inundation},
   *      author={George, David L},
   *      year={2006},
   *      publisher={University of Washington}
   * }
   *
   * Additionally, this solver has been based on two reference implementations:
   * The Clawpack implementation written in Fortran, which incorporates additional modifications for stability
   * and 2D domains (especially the subroutine riemann_aug_JCP):
   * https://github.com/clawpack/riemann/blob/master/src/rpn2_sw_aug.f90
   *
   * And another C++ implementation, where the structure of the code is based on in some parts:
   * https://github.com/TUM-I5/SWE-Solvers/blob/master/Source/AugRieSolver.hpp
   *
   * Further sources utilized by https://github.com/TUM-I5/swe-Solvers/blob/master/Source/AugRieSolver.hpp:
   *
   * @book{LeVeque2002,
   *      title={Finite volume methods for hyperbolic problems},
   *      author={LeVeque, Randall J},
   *      volume={31},
   *      year={2002},
   *      publisher={Cambridge university press}
   * }
   *
   *  @webpage{LeVeque2011,
   *       Author = {LeVeque, R. J.},
   *       Lastchecked = {January, 05, 2011},
   *       Title = {Clawpack Sofware},
   *       Url = {https://github.com/clawpack/clawpack-4.x/blob/master/geoclaw/2d/lib}
   *  }
   */
  template <class Shortcuts, class T>
  static inline GPU_CALLABLE_METHOD double augmentedState(
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
} // namespace applications::exahype2::exaskred::riemann

#include "AugmentedState.cpph"
