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
   * @Sean, Alex: Can we please have docu here on what is computed adn what the parameters mean?
   *  Some links to why and where it is used would be great, too.
   */
  template <int Rows, int Cols>
  tarch::la::Matrix< Rows, Cols, double > composeMatrixFromHWeightedLinearCombination(
    const std::vector< tarch::la::Matrix< Rows, Cols, double > >&   matrices,
    const std::vector<int>&                                         scaleFactors,
    const tarch::la::Vector<Dimensions, double>&                    h
  );
}


#include "mghype.cpph"

