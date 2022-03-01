// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_DG_UTILS_H_
#define _EXAHYPE2_DG_DG_UTILS_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace dg {
    /**
     * In ExaHyPE's Finite Volume setup, a cell hosts a patch of Finite Volumes.
     * When we iterate over these volumes, we typically have to know the centre
     * of the volume.
     *
     * @param quadraturePoints 1d quadrature points over unit interval. This array
     *                         has polynomialOrder+1 entries.
     */
    tarch::la::Vector<2,double>  getQuadraturePoint(
      const tarch::la::Vector<2,double>&  cellCentre,
      const tarch::la::Vector<2,double>&  cellSize,
      const tarch::la::Vector<2,int>&     index,
      int                                 polynomialOrder,
      const double* __restrict__          quadraturePoints
    );


    tarch::la::Vector<3,double>  getQuadraturePoint(
      const tarch::la::Vector<3,double>&  cellCentre,
      const tarch::la::Vector<3,double>&  cellSize,
      const tarch::la::Vector<3,int>&     index,
      int                                 polynomialOrder,
      const double* __restrict__          quadraturePoints
    );

    /**
     * Take polynomial within cell and project it onto the faces
     *
     * @param cellQ Pointer to cell data. The array has the size
     *   @f$ unknowns \times (order+1)^d@f$.
     *
     * @param unkonwns Integer greater or equal to 1. These number comprises both
     *   real unknowns in the hyperbolic sense and auxiliary variables.
     *
     * @param order Integer greater or equal to 0.
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
     *   i.e. the first unknowns entries are those ab the bottom of the vertical
     *   face from the left adjacent cell. The next unknowns entries are those
     *   at the bottom of the face from the right adjacent cell.
     *
     * @param faceQBottom Bottom unkonwns. See faceQLeft. Again, we order
     *   lexicographically. This time, the first half of the entries are those
     *   from the adjacent cell below. The remaining entries are the projections
     *   from above.
     */
    void projectVolumetricDataOntoFaces(
      const double* __restrict__          cellQ,
      int                                 order,
      int                                 unknowns,
      const double* const __restrict__    BasisFunctionValuesLeft,
      const double* const __restrict__    BasisFunctionValuesRight,
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
      const double* const __restrict__    BasisFunctionValuesRight,
      double* const __restrict__          faceQLeft,
      double* const __restrict__          faceQRight,
      double* const __restrict__          faceQBottom,
      double* const __restrict__          faceQUp,
      double* const __restrict__          faceQFront,
      double* const __restrict__          faceQBack
    );


    /**
     * The number of nodes in a cell is basically the input to the
     * power of d. The argument nodesPerAxis is typically the order
     * plus one.
     *
     * @return Number of nodes in a cell
     */
    int getNodesPerCell(int nodesPerAxis);


    /* returns a vector with the powers of nodesPerAxis up to the Dimension - 1
    ** e.g. for dims = 2, nodesPerAxis = 2 will return (1, 2)
    **      for dims = 3, nodesPerAxis = 4 will return (1, 4, 16)
    */
    tarch::la::Vector<Dimensions,int> getStrides(int nodesPerAxis);

    /*for a given linear node number, returns the index of that node
    **in cartesian coordinate system.
    **e.g. for dims = 2, nodesPerAxis = 4, there will be 16 total nodes
    **  so node 0 is [0,0], 1 is [1,0] and so on until 15 which is [3,3]
    **     for dims = 3, nodesPerAxis = 4, there will be 16*4 = 64 total nodes
    **     ranging from 0 which is [0,0,0] to 63 which is [3,3,3]
    */
    tarch::la::Vector<Dimensions,int> getIndex(
        int                                node,
        tarch::la::Vector<Dimensions,int>  strides
    );

    /*
     * for a given node index within the cell, returns the index of the neighbouring
     * node on the surface in a given direction with a given orientation.
     *
     */
    int cellIndexToHullIndex(
        const tarch::la::Vector<Dimensions,int>&  indexCell,
        const int                                 direction,
        const int                                 orientation,
        const int                                 nodesPerAxis
        );

    /*
     * computes the gradient of Q in a given direction for a given node
     * mostly used for the non-conservative product which requires this
     * Since the solution is represented by the sum of basis functions, its
     * derivative at a given node is the sum of the derivatives of the basis
     * functions evaluated at that node.
     * The derivative operator contains the value of the derivatives evaluated
     * at the quadrature nodes, therefore it only needs to be multiplied by
     * their coefficients.
     * u(x) = \sum_i \phi_i(x) * u_i => du(x)/dx = d\phi_i(x_i)/dx * u_i
     */
    void computeGradient(
        const double* __restrict__ const QCell,
        const double* __restrict__ const derivativeOperator,
        const double                     invDx,
        const int                        nodesPerAxis,
        const int                        unknowns,
        const int                        scalarIndex,
        double* __restrict__             gradQ
        );
  }
}

#endif
