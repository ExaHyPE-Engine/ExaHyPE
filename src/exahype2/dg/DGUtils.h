// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_DG_UTILS_H_
#define _EXAHYPE2_DG_DG_UTILS_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#include "Functors.h"



/**
 * @todo
 *
 * - More overviews documentation here which links to the correct Python files
 * - Find out which routines are not used anymore and remove them
 * - Clean-up/update hte unit tests (Tobias has broken some)
 * - BasisFunctionValues are symmetric and should be removed
 * - Mass matrix is not used correctly. Not sure if the inverted one is always symmetric (it is definitely not)
 */


namespace exahype2 {
  namespace dg {
    /**
     * In ExaHyPE's Finite Volume setup, a cell hosts a patch of Finite Volumes.
     * When we iterate over these volumes, we typically have to know the centre
     * of the volume.
     *
     * Given the centre of a cell as well as the size of this cell, this function
     * computes the spatial position of a quadrature node specified by its index
     *
     * For example if there are 2 points in each dimension, located at [0.25,0.75]
     * in 1d, then for a 2d cell with center 0.5 and size 1 [i.e. ranging from [0,0]
     * to [1,1], there would be four nodes specified with indexes 0 to 3 and respective
     * positions [0.25,0.25], [0.25,0.75], [0.75,0.25] and [0.75,0.75]
     *
     * @param polynomialOrder order of the underlying polynomial, this specifies
     * the number of quadraturePoints in each direction as order+1
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
    **
    ** This is used to iterate over the nodes of a cell in a given dimension,
    ** since the cells are all in the same array one after another advancing by
    ** n*strides[i] will move you to the cell which is n steps in direction i
    ** from the current cell.
    ** i.e. node+1 would be the "right" neighbour, node+nodesPerAxis would be the
    ** "upper" neighbour, and so on.
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
     * Ordering on the surface is the same as for the previous functions, but only
     * considering one "side" on each face
     *
     * So with the same notation as the previous function, if given a 2*2 cell
     * in 2 dimensions, there are a total of 4 faces (left,right,down,up), each
     * with 2 nodes.
     * If given index which corresponds to [0,1],
     *   direction 0, orientation 0 would return node 1 on the left face
     *   direction 1, orientation 1 would return node 0 on the upper face
     *       6 7
     *   1 [ o o ] 3
     *   0 [ o o ] 2
     *       4 5
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
     *
     * @param invDx: the inverse of the size of the cell in any one dimension, this
     * assumes that the cells are cubic
     *
     * @param derivativeOperator: array containing the value of the derivatives of
     * the n base functions of the dg polynomial evaluated at the n quadrature points
     * (in 1 dimension)
     *
     * @param scalarIndex: index of the node at which the gradient should be evaluated
     *
     * @param gradQ: where the gradient values should be placed, has size unknowns*Dimensions
     * since there is a gradient in each direction and the gradient is only nonzero for the
     * unknowns
     *
     * @param strideQ: total number of variables over which the gradient should be computed
     */
    void computeGradient(
        const double* __restrict__ const QCell,
        const double* __restrict__ const derivativeOperator,
        const double                     invDx,
        const int                        nodesPerAxis,
        const int                        strideQ,
        const int                        scalarIndex,
        double* __restrict__             gradQ
        );

    /*
     * Substracts contents of one cell array from another.
     */
    void subtractCell(
        double* __restrict__        QOut,
        const double* __restrict__  Qsubstract,
        const int                   order,
        const int                   unknowns,
        const int                   auxiliaryVariables
        );


    /*
     * Applies boundary conditions to a given face on the boundary, this
     * fills in the values on the "outside" of the face with values computed from the
     * boundaryConditions() function.
     *
     * @param strideQ: number of elements per node, typically this is the number of
     *  unknowns + the number of auxiliary variables
     *
     * @param lr: int which denotes whether the face is on the left of the cell or on the
     * right of the self (values respectively 0 and 1)
     */
    void applyBoundaryConditions(
        BoundaryConditions                          boundaryConditions,
        double*                                     QFace,
        const int                                   order,
        const int                                   strideQ,
        const tarch::la::Vector<Dimensions,double>& x,
        const double                                t,
        const int                                   faceNormal,
        const int                                   lr
        );


    /*
     * Plot patch
     *
     * usually used for debugging
     */

    std::string plotPatch(
        const double* __restrict__ Q,
        const int order,
        const int unknowns,
        const int auxiliaryVariables
        );

    /*
     * Plot Face
     *
     * Usually used for debugging. We plot all unknowns, i.e. both the unknowns
     * form the left and the right adjacent side. The order is linear, i.e. you
     * have to take the normal into account when you want to find out how the
     * unknowns are spatially arranged.
     *
     * See the discussion around ExaHyPE's lexicographic ordering.
     */
    std::string plotFace(
        const double* __restrict__ Q,
        const int order,
        const int unknowns,
        const int auxiliaryVariables
        );

  }
}

#endif
