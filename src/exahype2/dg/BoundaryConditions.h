// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>


namespace exahype2 {
  namespace dg {
    /**
     * Apply boundary conditions.
     *
     * Assumes we solely have the solution projected on the face.
     *
     * ## Arguments
     *
     * @param faceNumber The number of the face relativ from the cell
     *   that recognises that its face is called for the first time.
     *   So it is a number from [0,2d-1] according to Peano's standard
     *   face enumeration scheme. See grid::datatraversal::FaceEnumerator
     *   for more details.
     *
     * @param quadNodes quadraturePoints of the integration/quadrature
     *   points along a 1d unit interval. The array consequently has
     *   order+1 entries from (0,1). You can scale them with the actual
     *   cell width (cellSize) to fit it to a cell, and you can multiply
     *   is component-wisely with the quadrature point of interest to
     *   get a 2d or 3d coordinate. This is straightforward, as we
     *   employ Cartesian meshes, i.e. coordinates can be constructed
     *   via a tensor product approach.
     *
     */
    void applyBoundaryConditions(
      std::function< void(
        const double* __restrict__                   Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   boundaryCondition,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      int                                          faceNumber,
      const double* __restrict__                   quadraturePoints,
      double* __restrict__                         Q
    );
  }
}


