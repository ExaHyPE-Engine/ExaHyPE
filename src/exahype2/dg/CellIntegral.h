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

#include "exahype2/CellData.h"


namespace exahype2 {
  namespace dg {
    /*
     * Volumetric kernel for Gauss-Legendre shape functions using functors
     *
     * Computes the @f$ \hat Q @f$ from the generic Discontinuous Galerkin
     * explanation. That is the volumetric part of the weak formulation. It
     * does not include the previous time step's solution, and it is not (yet)
     * multiplied with the time step size or the inverse of the mass matrix.
     *
     * ## Algorithmic steps
     *
     * This version of the volumetric kernel works purely dof-wisely. That is,
     * we iterate over the @f$ (p+1)^d @f$ quadrature points within the cell
     * and compute the entries from @f$ \hat Q @f$. Per dof, we run through
     * the following steps:
     *
     * - Set the outcome to zero. This is important, as we accumulate from
     *   hereon.
     * - If you want to use a flux function: We know that the contribution of
     *   the flux term for one point only depends on the other dofs in the
     *   same line/row/column of the Cartesian dof layout. This is due to
     *   the fact that we use Legendre polynomials. Here, the polynomials are
     *   orthogonal, which simplifies things. So we don't have to loop over
     *   all dofs, but we can only loop in all directions once.
     *
     *
     *
     * Copy over in into out. We run an explicit Euler, so the value is updated
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
     *
     *
     * ## Arguments
     *
     * @param cellData The routine computes the DG cell contributions over
     *   multiple cells in one go. cellData is the wrapper around these cells,
     *   i.e. holds the pointers to input and ouput data, time step sizes, ...
     *   However, all cells carry the same polynomial degree and solve the
     *   same PDE.
     *
     * @param order Order of underlying dg polynomials. This specifies the number
     *   of nodes in each dimension, which is order+1 (you need two points to specify
     *   a linear polynomial for example). The order has to be the same for all cells
     *   handed over via cellData.
     *
     * @param unknowns Mumber of unknowns whose values must be updated. If you
     *   solve a PDE with n eqations over a function Q with n+m entries, then n
     *   is the number of unknonws that evolve according to the hyperbolic system
     *   of PDEs, and m are auxiliary (material) parameters. They do not change due
     *   to the PDE (though any user function might want to alter them).
     *
     * @param auxiliaryVariables. Auxiliary variables. See description of unknowns.
     *
     * @param quadratureNodes Location of quadrature nodes along one dimension in a
     *   reference element (length). We usually expect the nodes to be
     *   Gauss-Legendre or Gauss-Lobatto quadrature points, but we don't really
     *   bake these considerations into the present routine. However, we do
     *   hardcode the information that we work with a Cartesian tensor product
     *   layout of the dofs, i.e. the (x,y,z) position can be computed per
     *   entry with y and z being independent of x. See getQuadraturePoint()
     *   for some documentation.
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
        const double* __restrict__                     quadWeights,
     *
     *
     *    since we implicitly integrate by summing over these using their weights.
     *
     * @param: MassMatrix: MassMatrix contains the quadrature weights on the diagonal.
     *
     * @param: StiffnessMatrix contains the derivatives of the basis functions evaluated
     *  at the quadrature nodes multiplied by the quadrature weight.
     *
     * @param: DerivativeOperator contains the derivative of the basis function evaluated
     *  at the quadrature node.
     *
     */
    void cellIntegral_patchwise_GaussLegendre_functors(
      ::exahype2::CellData&                          cellData,
      const int                                      order,
      const int                                      unknowns,
      const int                                      auxiliaryVariables,
      Flux                                           flux,
      NonConservativeProduct                         nonConservativeProduct,
      Source                                         source,
      PointSources                                   pointSources,
      const double* __restrict__                     QuadratureNodes1d,
      const double* __restrict__                     MassMatrixDiagonal1d,
      const double* __restrict__                     StiffnessMatrix1d,
      const double* __restrict__                     DerivativeOperator1d,
      bool                                           evaluateFlux,
      bool                                           evaluateNonconservativeProduct,
      bool                                           evaluateSource,
      bool                                           evaluatePointSources
    );


    /**
     * This is the final step of any DG algorithm. Consult dg.h for a
     * clearer explanation.
     *
     * This routine assumes that the mass matrix is a diagonal. Therefore,
     * the inversion is trivial.
     *
     * This routine also copies over the material parameters.
     */
    void multiplyWithInvertedMassMatrixAndAddToSolution_GaussLegendre(
      ::exahype2::CellData&                          cellData,
      const int                                      order,
      const int                                      unknowns,
      const int                                      auxiliaryVariables,
      const double* __restrict__                     MassMatrixDiagonal1d
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
       *
       * @todo raus?
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
