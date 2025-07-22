// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/geometry/Block.h"

namespace toolbox {
  namespace curvi {
    namespace Transformation {
      void metricDerivativesAndJacobian3D(
        double**            dudx,
        const Block&        in,
        const double* const dx,
        std::vector<int>    range_z,
        std::vector<int>    range_y,
        std::vector<int>    range_x,
        Block&              out
      );

      void computeDerivatives_x_3D(
        double** dudx,
        int      k,
        int      j,
        int      i,
        double*  values,
        int      num_nodes,
        double&  der_x,
        double   dx
      );

      void computeDerivatives_y_3D(
        double** dudx,
        int      k,
        int      j,
        int      i,
        double*  values,
        int      num_nodes,
        double&  der_y,
        double   dy
      );

      void computeDerivatives_z_3D(
        double** dudx,
        int      k,
        int      j,
        int      i,
        double*  values,
        int      num_nodes,
        double&  der_z,
        double   dz
      );
    }; // namespace Transformation
  } // namespace curvi
} // namespace toolbox