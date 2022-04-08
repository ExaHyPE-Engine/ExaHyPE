// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_EULER_H_
#define _EXAHYPE2_DG_EULER_H_

/*
 * For a full explanation of the Discontinuous galerkin method, check out appropriate literature, but very briefly:
 *
 *  we approximate the solution of an equation on a volume by a polynomial of order n, which we interpolate by lagrange polynomials.
 *  This lets us simply use the values of the function at the points as coefficients of the lagrange polynomials.
 *  As support nodes of the lagrange polynomial we use gaussian interpolation points, which lets us approximate an integral in a dimension
 *  by summing up the values of the values of the function at the quadrature points along this dimension.
 *
 *  These two properties let us construct an approximation of the integral of the function, which we divide into two parts: one purely local
 *  volume part and one riemann part which approximates the flux over the boundary.
 *
 *  In this file is the implementation of the approximation using discontinuous galerkin of the volume part, the flux over the boundary is then
 *  in Riemann.h
 */


#include <vector>
#include <algorithm>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#include "DGUtils.h"

namespace exahype2 {
  namespace dg {
    /*
     * Computes the update caused by an explicit Euler on one cell
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
     * Algorithm:
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
     */
    void explicitEulerOnCell_Flux_EvaluateTermsOnce(
      const double* __restrict__                    cellQin,
      double* __restrict__                          cellQout,
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  dx,
      const double                                  t,
      const double                                  dt,
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   flux,
      const double* __restrict__                     quadNodes,
      const double* __restrict__                     quadWeights,
      const double* __restrict__                     MassMatrix,
      const double* __restrict__                     StiffnessMatrix,
      const double* __restrict__                     DerivativeOperator,
      const bool                                     useFlux,
      const bool                                     useNCP,
      const bool                                     useSource
    );


    /*
     * Computes the update caused by an explicit Euler on one cell
     *
     * Generic implementation which implements fluxes redundantly.
     * Since the flux contributions are added over all of a node's (order+1)
     * neighbours along each dimension, one node's flux contribution is
     * required a total of (order+1)*d times. Here it is naively re-computed
     * each time it is required, which adds to the computational intensity of
     * the function but avoids the need to store these values and therefore
     * reduces the overall memory footprint.
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
     *
     * @todo Rename
     */
    void explicitEulerOnCell_FluxNCPSource_EvaluateTermsRedundantly(
        const double* __restrict__                     cellQin,
        double* __restrict__                           cellQout,
        const int                                      order,
        const int                                      unknowns,
        const int                                      auxiliaryVariables,
        const tarch::la::Vector<Dimensions,double>&    x,
        const double                                   dx,
        const double                                   t,
        const double                                   dt,
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
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double * __restrict__                        S
        ) >   source,
        const double* __restrict__                     quadNodes,
        const double* __restrict__                     quadWeights,
        const double* __restrict__                     MassMatrix,
        const double* __restrict__                     StiffnessMatrix,
        const double* __restrict__                     DerivativeOperator,
        bool                                           useFlux,
        bool                                           useNCP,
        bool                                           useSource
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
