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
   **** An new formulation of the HLLEM Riemann solver for the Shallow Water Equation with support for inundation
   ****
   * Literature:
   *
   *   @article{DUMBSER2016275,
   *            title = {A new efficient formulation of the HLLEM Riemann solver for general conservative and non-conservative hyperbolic systems},
   *            journal = {Journal of Computational Physics},
   *            volume = {304},
   *            pages = {275-319},
   *            year = {2016},
   *            issn = {0021-9991},
   *            doi = {https://doi.org/10.1016/j.jcp.2015.10.014},
   *            url = {https://www.sciencedirect.com/science/article/pii/S0021999115006786},
   *            author = {Michael Dumbser and Dinshaw S. Balsara}}
   *
   *   @article{KURGANOV,
   *            title = {A Second-Order Well-Balanced Positivity Preserving Central-Upwind Scheme for the Saint-Venant System},
   *            year = {2007},
   *            month = {03},
   *            pages = {},
   *            volume = {5},
   *            journal = {Communications in mathematical sciences},
   *            author = {Kurganov, Alexander and Petrova, G.},
   *            doi = {10.4310/CMS.2007.v5.n1.a6}}
   ****
   */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  double hllem(
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

#include "HLLEM.cpph"
