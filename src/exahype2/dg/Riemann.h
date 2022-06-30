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
  /*
   * Evaluates the eigenvalue at each node along each face and, for each face
   * returns the greatest eigenvalue along this face.
   * This is used for the computation of the rusanov flux
   *
   * @param QHull is an array of double pointers, it contains pointers to each
   *  face of the cell in order of dimension and negative then positive direction
   *  e.g. QHull[0] is a pointer to the first element of the left face along
   *  direction 0, QHull[1] to the right face along direction 0, QHull[2] to the left
   *  face along direction 1, etc.
   *  Note that each face has a left and right side because it contains values from its
   *  left and right cells. The order of the nodes is always left, right, left, right
   *  and each node contains unknowns+auxiliaryVariables elements so the first
   *  unknowns+auxiliaryVariable elements correspond to first node of the left side,
   *  the second to the first node of the right side and so on.
   *
   *
   * @param eigenvalues is the array into which the values are inserted in the same order
   *
   *  ordering on each face:
   *
   *         2 3
   *         0 1
   *   2 3 [ o o ] 2 3
   *   0 1 [ o o ] 0 1
   *         2 3
   *         0 1
   *
   */
  void getMaxEigenvaluePerFace(
      MaxEigenvalue                                 maxEigenvalue,
      double* __restrict__                          QHull[Dimensions*2],
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  t,
      const double                                  dt,
      double                                        eigenvalues[Dimensions*2]
      );

  /*
   * Computes the numerical value of the rusanov flux (or lax-friedrich)
   *  along each node of each face.
   *
   * @param QHullIn see above
   *
   * @param QHullOut is where the computed values are put, this has a similar structure
   *  to QHullIn but since we only require one value for the flux instead of a value on
   *  the left and right sides of the face this is only half as large
   *
   * @todo das getMaxEigenvalue sollte nicht separat sein (oder in internal) und direkt
   * hier verwendet werden, so dass die Signatur einfacher wird.
   */
  void computeRusanovOnHull(
      Flux                                          flux,
      NonConservativeProduct                        nonConservativeProduct,
      double*                                       QHullIn[Dimensions*2],
      double* __restrict__                          QHullOut,
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  dx,
      const double                                  t,
      const double                                  dt,
      const double                                  maxEigenvalueAlongFace[Dimensions*2],
      const bool                                    useFlux,
      const bool                                    useNCP
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
