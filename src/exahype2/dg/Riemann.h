// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_RIEMANN_H_
#define _EXAHYPE2_DG_RIEMANN_H_


#include <vector>

#include "DGUtils.h"
#include "Functors.h"

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace dg {


  /**
   * Take polynomial within cell and project it onto the faces
   *
   * ## Ordering
   *
   *                           2 3
   *                           0 1
   *                     2 3 [ o o ] 2 3
   *                     0 1 [ o o ] 0 1
   *                           2 3
   *                           0 1
   *
   * Each individual face is ordered lexicographically. So we go from left to
   * right, and we always start closest to the origin or the coordinate system
   * if we interpret the cell as the unit square.
   *
   * The sketch above illustrates the ordering of the four faces for a p=1
   * discretisation where we have two dofs per direction per face per side.
   *
   *
   *
   * @param cellQ Pointer to cell data. The array has the size
   *   @f$ unknowns \times (order+1)^d@f$.
   *
   * @param unkonwns Integer greater or equal to 1. These number comprises both
   *   real unknowns in the hyperbolic sense and auxiliary variables.
   *
   * @param order Integer greater or equal to 0. The number of nodes in each
   *  dimension will be equal to order+1
   *
   * @param BasisFunctionValuesLeft Array of size order+1 which clarifies (in 1d)
   *   how to project the order+1 quadrature weights onto a value on the left
   *   face. For Gauss Lobatto, this is a trivial array (you can read out the
   *   value directly from the leftmost quadrature point, but for any other
   *   basis, you need a linear combination of all the quadrature points in the
   *   cell.
   *
   * @param faceQLeft Left unknowns. This array hosts @f$ 2 \times (order+1)^(d-1) * unknowns@f$
   *   values, as it holds the (projected) values from the left side of the face
   *   and those from the right. The ordering of these values is lexicographic,
   *   i.e. the first unknowns entries are those at the bottom of the vertical
   *   face from the left adjacent cell. The next unknowns entries are those
   *   at the bottom of the face from the right adjacent cell. The next are then
   *   again left, then right, then left and so on.
   *   This function will fill the right values since the current cell is to the
   *   right of its own left face.
   *
   * @param faceQBottom Bottom unknowns. See faceQLeft. Again, we order
   *   lexicographically. This time, the first half of the entries are those
   *   from the adjacent cell below. The remaining entries are the projections
   *   from above.
   *
   */
  void projectVolumetricDataOntoFaces(
    const double* __restrict__          cellQ,
    int                                 order,
    int                                 unknowns,
    const double* const __restrict__    BasisFunctionValuesLeft,
    double* const __restrict__          faceQLeft,
    double* const __restrict__          faceQRight,
    double* const __restrict__          faceQBottom,
    double* const __restrict__          faceQUp
  );

  /**
   * 3d counterpart to other projectVolumetricDataOntoFaces() variant.
   */
  void projectVolumetricDataOntoFaces(
    const double* __restrict__          cellQ,
    int                                 order,
    int                                 unknowns,
    const double* const __restrict__    BasisFunctionValuesLeft,
    double* const __restrict__          faceQLeft,
    double* const __restrict__          faceQRight,
    double* const __restrict__          faceQFront,
    double* const __restrict__          faceQBack,
    double* const __restrict__          faceQBottom,
    double* const __restrict__          faceQUp
  );



  /*
   * Given a numerical flux at the various faces, this computes and adds the Riemann
   * integral of this flux on the nodes of the cell to compute the solution at the
   * following timestep.
   * Computes this flux for a 2-dimensional cell
   *
   * @param cellQ the current solution, expected to be the solution already updated
   *   with local contributions. (See Euler.h)
   *
   * @param faceQX This should contain a value for the numerical flux at each node along
   * the corresponding face. This is typically computed by a Riemann solver such as
   * a Rusanov solver.
   *
   * @param: BasisFunctionValuesLeft (respectively Right): as the name implies, this is
   *  the numerical value of the basis function value evaluated at the left node in
   *  1 dimension. This has a part in determining how much the value at the boundary
   *  influences the value at a given node.
   *  Note that the Basis function values on the right side are symmetric to those on the
   *  left side, so the BasisFunctionValuesLeft are also used there.
   *
   * @param quadratureWeights: quadrature weight of the given quadrature node 1 dimension
   */
  void computeRiemannIntegralAndAddToCell(
    const double* const __restrict__    faceQLeft,
    const double* const __restrict__    faceQRight,
    const double* const __restrict__    faceQBottom,
    const double* const __restrict__    faceQUp,
    int                                 order,
    int                                 unknowns,
    const int                           auxiliaryVariables,
    const tarch::la::Vector<2,double>&  cellSize,
    const double                        dt,
    const double* const __restrict__    BasisFunctionValuesLeft,
    const double* __restrict__          quadratureWeights,
    double* __restrict__                cellQ
  );


  void computeRiemannIntegralAndAddToCell(
    const double* const __restrict__    faceQLeft,
    const double* const __restrict__    faceQRight,
    const double* const __restrict__    faceQBottom,
    const double* const __restrict__    faceQUp,
    const double* const __restrict__    faceQFront,
    const double* const __restrict__    faceQBack,
    int                                 order,
    int                                 unknowns,
    const int                           auxiliaryVariables,
    const tarch::la::Vector<2,double>&  cellSize,
    const double                        dt,
    const double* const __restrict__    BasisFunctionValuesLeft,
    const double* __restrict__          quadratureWeights,
    double* __restrict__                cellQ
  );

  /*
   * Given a numerical flux at the surface from QHullIn, this applies this flux to
   * the values of Q to compute the next solution.
   * As opposed to previous two functions this is a generic function which works for
   * any number of dimensions.
   *
   * @param Q the current solution, expected to be the solution already updated
   *   with local contributions. (See Euler.h)
   *
   * @param QHullIn see computeRusanov above. This should contain a value for the
   *   numerical flux at each node along each face
   *
   * @param: BasisFunctionValuesLeft (respectively Right): as the name implies, this is
   *  the numerical value of the basis function value evaluated at the left node in
   *  1 dimension. This has a part in determining how much the value at the boundary
   *  influences the value at a given node.
   *
   * @param quadratureWeights: quadrature weight of the given quadrature node 1 dimension
   *
   * @todo Name ist nicht gut. Sollte wohl eher add sein.
   */
  void applyRiemannToPatch(
      double* __restrict__        Q,
      const double* __restrict__  QHullIn,
      const int                   order,
      const int                   unknowns,
      const int                   auxiliaryVariables,
      const double                dx,
      const double                dt,
      const double* __restrict__  BasisFunctionValuesLeft,
      const double* __restrict__  BasisFunctionValuesRight,
      const double* __restrict__  quadratureWeights
      );

  }
}

#endif
