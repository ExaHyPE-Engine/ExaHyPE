// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include "tarch/la/Matrix.h"


#include <string>


namespace mghype {
  /**
   * Compute a weighted linear combination of matrices
   *
   * @see applyMatrixFromHWeightedLinearCombination() for a description 
   *   of the arguments.
   */
  template <int Rows, int Cols>
  tarch::la::Matrix< Rows, Cols, double > composeMatrixFromHWeightedLinearCombination(
    const std::vector< tarch::la::Matrix< Rows, Cols, double > >&   matrices,
    const std::vector<int>&                                         scaleFactors,
    const tarch::la::Vector<Dimensions, double>&                    h
  );


  /**
   * Apply a series of vectors to another vector
   *
   * Compute
   *
   * @f$
   *  r = \sum _i h^{s_i} M_i v
   * @f$
   *
   * which is the standard operation that we need in any multigrid code. The
   * @f$ s_i @f$ from above are from scaleFactors, while the @f$ M_i @f$ are
   * the elements from matrices.
   */
  template <int Rows, int Cols>
  tarch::la::Vector< Rows, double > applyMatrixFromHWeightedLinearCombination(
    const std::vector< tarch::la::Matrix< Rows, Cols, double > >&   matrices,
    const std::vector<int>&                                         scaleFactors,
    const tarch::la::Vector<Dimensions, double>&                    h,
    const tarch::la::Vector< Cols, double >&                        v
  );
}


#include "mghype.cpph"

