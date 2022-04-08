// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_RIEMANN_H_
#define _EXAHYPE2_DG_RIEMANN_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#include "DGUtils.h"

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
   * @param atBoundary specifies whether a given face is at the boundary, in which case the
   *  outer values are ignored and instead the function boundaryConditions() is evaluated in
   *  order to compute outerValues which are then used instead.
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
      std::function< double(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                         normal
          ) >   maxEigenvalue,
      std::function< void(
        const double * __restrict__                  Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal
      ) >   boundaryConditions,
      double* __restrict__                          QHull[Dimensions*2],
      bool                                          atBoundary[Dimensions*2],
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  t,
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
   * @param atBoundary boolean array which specifies whether a face is at the boundary or
   *  not. If a face is at the boundary the outside values (e.g. left of the face if the
   *  face is at the left of the cell) will be ignored and the flux is instead computed
   *  with the values given by the boundaryConditions() function
   */
  void computeRusanovOnHull(
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   flux,
      std::function< void(
        const double * __restrict__                  Q,
        const double * __restrict__                  dQdx,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   nonConservativeProduct,
      std::function< void(
        const double * __restrict__                  Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal
      ) >   boundaryConditions,
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
      const bool                                    atBoundary[Dimensions*2],
      const bool                                    useFlux,
      const bool                                    useNCP
      );

  /*
   * Given a numerical flux at the surface from QHullIn, this applies this flux to
   * the values of Q to compute the next solution.
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
