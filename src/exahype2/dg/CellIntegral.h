// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_CELL_INTEGRAL_H_
#define _EXAHYPE2_DG_CELL_INTEGRAL_H_

/*
 * @page DG Discontinous Galerkin methods in ExaHyPE
 *
 *
 * For a comprehensive explanation of the Discontinuous galerkin method, check out
 * appropriate literature, but very briefly:
 *
 * We approximate the solution of an equation on a volume by a polynomial of order p,
 * which we interpolate by Lagrangian polynomials. As we solely support Lagrangian
 * polynomials, we have the simple property that the shape functions are 1 in the
 * associated sample point, but 0 in all other points.
 *
 *  This lets us simply use the values of the function at the points as coefficients of the lagrange polynomials.
 *  As support nodes of the lagrange polynomial we use gaussian interpolation points, which lets us approximate an integral in a dimension
 *  by summing up the values of the values of the function at the quadrature points along this dimension.
 *
 *  These two properties let us construct an approximation of the integral of the function, which we divide into two parts: one purely local
 *  volume part and one riemann part which approximates the flux over the boundary.
 *
 *  In this file is the implementation of the approximation using discontinuous galerkin of the volume part, the flux over the boundary is then
 *  in Riemann.h
 *
 *
 * ## Runge-Kutta vs DG
 *
 * I have thought quite a lot whether I call the C++ namespace rkdg or dg. Eventually,
 * I decided to stick to DG. After all, RK is basically a sequence of Euler
 * steps with different starting points and time step sizes, where the all
 * intermediate and the final solution are constructed through a linear combination
 * of all previous Euler outcomes. These Euler steps then employ Discontinous Galerkin
 * operators, so it seemed to be safer to stick to DG which is then used by RK.
 *
 * This is in line with the Finite Volume naming convention. Here, FV is the overarching
 * namespace, and the Python script then discriminates MUSCL vs Rusanov, e.g.
 *
 *
 *
 *
 *
 *  ## Cell integral
 *
 *  This header file contains only routines computing the cell integrals resulting
 *  from an explicit (Euler) time step. An Euler step is the fundamental building
 *  block behind any Runge-Kutta scheme, as
 *
 *
 *
 *
 * We exploit the fact that we can update all the updates within a cell independently:
 * All the unknowns within a patch can (and do) affect every single unknown in the
 * output, but the output variables do not interact. After all, it is an explicit scheme.
  * Consequently, we can loop over all the unknowns in parallel (if we want).
  *
     * This function computes the change of the solution in the cell if it were an isolated
     * system. That is to say it ignores any contribution over the surface of the cell.
     * Given this, the solution at the next timestep is the solution at the current timestep
     * plus disturbances caused by:
     *
     *  - sources, which are purely local. At every node, there may be an increase or a
     *    decrease caused by sources or sinks at that location.
     *
     *  - the flux, which is not local. Every node's value may increase or decrease according
     *    to some flux coming from or going into its neighbouring nodes. This flux exists in
     *    every dimension and therefore in total is the sum of the flux contributions going
     *    into this node from every neighbouring node along each dimension.
     *    Since the flux at each node is required for each neighbouring node, we can precompute
     *    these fluxes only once and reuse their values, but this increases the total memory
     *    footprint.
     *
     *  - the non-conservative product (ncp), which represents non-conservative effects on the
     *    solution. One example of this is friction in fluids, which causes energy to be lost.
     *    This is local but unlike the source has a direction (there will be a contribution of
     *    the friction on your overall energy for both your velocity in x and y direction for
     *    example) and in general requires the gradient of the solution in each of these directions.
 *
 *
 */


#include <vector>
#include <algorithm>

#include "DGUtils.h"
#include "Functors.h"

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace dg {
    /*
     *
     *
     * ## Visibility/realisation
     *
     * The Euler variant is static and all in the header. As I discuss in Rusanov.h, this is
     * not nice. For DG, we have way heavier routines, to it is reasonable to model the kernels
     * as proper functions with implementation files.
     *
     *
     * ## Algorithmic steps
     *
     * - Copy over in into out. We run an explicit Euler, so the value is updated
     *   according to a formula @f$ Q(t+\Delta t) = Q(t) + \Delta t \cdot (F + NCP) + S @f$
     *   ,the first part of which are the values at the last timestep
     *
     * - Precompute value of the flux function at each node
     *
     * - For each node in the cell, add the change (Δt \cdot (F + NCP) + S essentially)
     *  -> Add purely local source contribution
     *  -> For each direction, compute the gradient in that direction, then using this
     *     compute the non-conservative product in that direction and add it
     *  -> For each dimension and each node in that dimension (including oneself), use
     *     the precomputed flux at that node to compute its contribution to the current node.
     *
     *  @param cellQin: solution of the function at each node for the previous timestep.
     *    a cell will have (order+1) nodes along each direction, each with
     *    (unknowns+auxiliaryVariables) values for a total of
     *    (unknowns+auxiliaryVariables)*(order+1)^d values in total
     *
     *  @param quadNodes: location of quadrature nodes along a dimension in a reference
     *   element we expect the nodes to be gauss-legendre or gauss-lobatto quadrature
     *   nodes since we implicitly integrate by summing over these using their weights.
     *
     *  @param quadWeights: corresponding weight of each node
     *
     *  @param cellQout: is where the computed solution of the function at next timestep
     *    are put
     *
     * @param cellQin, out: input and output cells
     *
     * @param order: order of underlying dg polynomial, this specifies the number
     *  of nodes in each dimension, which is order+1 (you need two points to specify
     *  a linear polynomial for example)
     *
     * @param unknowns: number of unknowns whose values must be updated
     *
     * @param auxiliaryVariables: variables which still have local values but do not
     *  change over time.
     *
     * @param quadNodes, quadWeights: used both for gaussian interpolation and are assumed
     *  to be the support points of underlying lagrange polynomials
     *
     * @param: MassMatrix: MassMatrix contains the quadrature weights on the diagonal.
     *
     * @param: StiffnessMatrix contains the derivatives of the basis functions evaluated
     *  at the quadrature nodes multiplied by the quadrature weight.
     *
     * @param: DerivativeOperator contains the derivative of the basis function evaluated
     *  at the quadrature node.
     */
/*
    void timeStepEuler_patchwise_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfVolumesPerAxisInPatch,
        int                     haloSize,
        int                     unknowns,

        @todo CellDescription hier rein
        @todo Normalise signature re other solvers

*/
    // @todo Massenmatrix ist 1d? Oder lumped oder was?
    void cellIntegral_patchwise_computeAndUpdate_functors(
        const double* __restrict__                     cellQin,
        double* __restrict__                           cellQout,
        const int                                      order,
        const int                                      unknowns,
        const int                                      auxiliaryVariables,
        const tarch::la::Vector<Dimensions,double>&    x,
        const double                                   dx,
        const double                                   t,
        const double                                   dt,
        Flux                                           flux,
        NonConservativeProduct                         nonConservativeProduct,
        Source                                         source,
        PointSources                                   pointSources,
        const double* __restrict__                     quadNodes,
        const double* __restrict__                     quadWeights,
        const double* __restrict__                     MassMatrix,
        const double* __restrict__                     StiffnessMatrix,
        const double* __restrict__                     DerivativeOperator,
        bool                                           evaluateFlux,
        bool                                           evaluateNonconservativeProduct,
        bool                                           evaluateSource,
        bool                                           evaluatePointSources
    );


    /*
     * Computes the greatest eigenvalue over all nodes in a cell
     * @param strideQ: number of elements per node per cell, usually number of
     *    unknowns + number of auxiliary variables
     * @param dt: timestep
     */
    double getMaxEigenvalue(
        MaxEigenvalue                                  maxEigenvalue,
        const double* __restrict__                     Q,
        const int                                      order,
        const int                                      strideQ,
        const tarch::la::Vector<Dimensions,double>&    x,
        const double                                   t,
        const double                                   dt
        );


    namespace internal {
      /**
       * Copy solution over for one node
       *
       * This method signature is kind of an overkill (we don't need all of the
       * variables), but I decided to keep the internal routines as close to
       * each other as possible.
       */
      void copySolution(
        const double* __restrict__                    cellQin,
        const int                                     order,
        const int                                     unknowns,
        const int                                     auxiliaryVariables,
        const tarch::la::Vector<Dimensions,int>&      node,
        double* __restrict__                          cellQout
      );
    }
  }
}

#endif
