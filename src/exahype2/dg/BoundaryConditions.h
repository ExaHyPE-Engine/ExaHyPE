// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace dg {
    /**
     * Apply boundary conditions.
     *
     * Assumes we solely have the solution projected on the face.
     *
     * The functor should prescribe a physical state (wall reflection, far-field
     * data). Copying the inside trace zeroes the jump, so the Riemann solve
     * degenerates to the one-sided interior flux: upwind for the outgoing
     * characteristic but downwind for the incoming one, and the semi-discrete
     * operator gains wall modes with positive real part that grow under any
     * explicit time integrator.
     *
     * ## Arguments
     *
     * @param faceNumber The number of the face relative to the cell
     *   that recognises that its face is called for the first time.
     *   So it is a number from [0,2d-1] according to Peano's standard
     *   face enumeration scheme. See peano4::datamanagement::FaceEnumerator
     *   for more details.
     *
     * @param quadraturePoints Positions of the quadrature points along a
     *   1d unit interval. The array consequently has order+1 entries from
     *   (0,1). The routine scales them with the actual cell width
     *   (cellSize) and combines them component-wise into a 2d or 3d
     *   coordinate. This is straightforward, as we employ Cartesian
     *   meshes, i.e. coordinates can be constructed via a tensor product
     *   approach.
     */
    void applyBoundaryConditions(
      std::function<void(
        const double* NOALIAS                        Qinside,
        double* NOALIAS                              Qoutside,
        const tarch::la::Vector<DIMENSIONS, double>& x,
        double                                       t,
        double                                       dt,
        int                                          normal
      )>                                           boundaryCondition,
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      int                                          faceNumber,
      const double* NOALIAS                        quadraturePoints,
      double* NOALIAS                              Q
    );
  } // namespace dg
} // namespace exahype2
