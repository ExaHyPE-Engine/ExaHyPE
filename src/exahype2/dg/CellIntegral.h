// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_CELL_INTEGRAL_H_
#define _EXAHYPE2_DG_CELL_INTEGRAL_H_


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
     * Final step of DG algorithm
     *
     * This is the final step of any DG algorithm. Consult dg.h for a
     * clearer explanation.
     *
     * - I assume that the input points to the old time step's solution.
     *   This is typically
     *
     *        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination
     *
     *   in a Discontinuous Galerkin context.
     * - I assume that the output points to the new time step's data. This is
     *   usually
     *
     *        fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates
     *
     *   subject to some shifts.
     * - The output already holds the flux contributions, source terms, and
     *   so forth, but these contributions are stored in the weak image space,
     *   i.e. we still have to multiply them with the inverse of a mass matrix.
     *   The output also contains the Riemann solutions, back-propagated into
     *   the volume.
     * - The output shall holds the flux contributions in a Runge-Kutta sense,
     *   i.e. it shall not hold the new time step but the amount that's added.
     *
     *
     * Let
     *
     * @f$ \partial Q = F(Q) @f$
     *
     * be our ODE of interest. Runge-Kutta will construct new solutions according
     * to a rule
     *
     * @f$ Q^{(new)} = c_i \cdot dt \cdot (a_{i1} k_1 + a_{i2} k_2 + \dots )  @f$
     *
     * The linear combination field will holds the current @f$ Q^{(new)} @f$. The
     * estimates however have to be the individual @f$ k_j @f$ from above. That means,
     * they are not to be scaled by any time step size, and they do not incorporate
     * the original time step. They also do not carry any auxiliary variables. So
     * we don't need the input data
     *
     *        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination
     *
     * in this routine. We focus on the output.
     *
     * This particular routine assumes that the mass matrix is a diagonal. Therefore,
     * the inversion is trivial: We iterate over all degrees of freedom. This is a
     * d-dimensional for loop (dfor). Per dof, we compute the diagonal, and every
     * unknown then is scaled by the inverse of this diagonal.
     */
    void multiplyWithInvertedMassMatrixAndClearAuxiliaryVariables_GaussLegendre(
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
       * @todo Noch komplett auf diese Micro-Kernel-Notation umstellen, die Dominic im ADER eingefuehrt hat
       *  Das aber erst machen, nachdem wir die Enumeratoren da haben
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
