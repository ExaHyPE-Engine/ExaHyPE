// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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
     * The whole implementation works cell-wisely, i.e. runs through cell by
     * cell. Within each cell, it works dof-wisely, i.e. it runs through each
     * individual dof and accumulates all the PDE contriutions to this dof.
     * This second loop is for loop over node, and the d-dimensional index
     * of node if given by index.
     *
     *
     * ## Solver flavour
     *
     * We rely on Gauss-Legendre quadrature nodes on tensor product spaces, i.e.
     * we exploit the fact that the mass matrix has diagonal form. Consequently,
     * we can construct all involved cell-local operators on-the-fly by
     * multiplying their 1d counterparts.
     *
     * We run over the patches on by one (patchwise). We do not exploit that we
     * have to do the same operation for all dofs of all patches.
     *
     * We evaluate all operators in-situ, i.e. get their output and apply it
     * immediately. See the discussion below that each dof's flux evaluation
     * needs all the fluxes in that row of dofs. Consequently, we evaluate
     * each flux p times if we have p quadrature points per axis. This is
     * redundant work. In return, in-situ implies that we don't have to store
     * data temporarily.
     *
     *
     * ## Flux evaluation
     *
     * This version of the volumetric kernel uses the Gauss Legendre property,
     * i.e. the orthogonality of the polynomials, plus the tensor product style
     * of the shape functions aggressively.
     *
     * For the flux, we first loop over the dimensions (dim) as we have
     * fluxes in all d directions. We are interested how the dof in the
     * position index changes due to all the fluxes around. So we want to
     * find out what happens if we test against the test function @f$ \phi @f$
     * in the index dof.
     *
     * For the flux, tensor products and orthogonality means that the flux
     * in x-direction in any point really only interferes with the unknowns
     * along the x-direction along this same line: We test @f$ F(Q) \partial_x \phi @f$
     * and assume that @f$ \phi = \phi _{1d}(x)\phi _{1d}(y)\phi _{1d}(z) @f$.
     * Furthermore, lets represent @f$ F(Q) @f$ with the same polynomials,
     * too. In the product of the flux with the test function (which now
     * consists of 2d ingredients), all the polynomials besides the @f$ \partial _x \phi(x) @f$
     * are orthogonal - the fact that the polynomials have been chosen as
     * orthogonal ones means that their derivatives are obviously not automatically
     * orthogonal.
     *
     * So all the shape functions spanning F where the y and z coordinates do
     * not match cancel out in the example above if the test functions don't
     * have the same coordinates. We are left with all the shapes along the
     * x-axis within the cubic cell. Analogous arguments hold for y and z.
     *
     *
     * ## Algebraic source
     *
     * The treatment of the source term is close to trivial: We assume that
     * we can represent the source @f$ S(Q) @f$ in the shape space. Again,
     * we exploit the orthogonality. If we want to test what the impact of
     * the source is if we test against the index-th test function, the
     * orthogonality implies that we only have take S(Q) in index multiplied
     * with its shape function. We get a mass matrix entry.
     *
     *
     * ## Non-conservative product
     *
     * The non-conservative product once again relies on a very crude numerical
     * approximation: We assume that @f$ B_x(Q) \partial _x Q @f$ can be
     * represented within our orthogonal shape space. Consequently, it is
     * tried exactly in the same way as the algebraic source.
     *
     * This approach seems to be super inaccurate, but we have to keep in mind
     * that B can be highly non-linear. So we don't know anything about the
     * analytical shape of @f$ B_x(Q) \partial _x Q @f$ anyway: It could be
     * any type of polynomial. So it is convenient to assume that it is once
     * again from the same space as @f$ \phi @f$.
     *
     *
     * ## Signs in front of the individual terms
     *
     * We rely on the formulation as discussed on the page Discontinuous
     * Galerkin. See dg.h or the doxygen html pages for details. One important
     * fact to keep in mind is that this routine computes $\partial _t Q$.
     * However, all PDE terms besides the source arise on the left-hand side of
     * the PDE, i.e. on the same side as the time derivative. We therefore have
     * to bring these terms to the right which gives us an additional minus
     * sign.
     *
     * Here's the rationale behind the volumetric terms:
     *
     * - The source term is already on the right-hand side in our PDE
     *   formulation. Therefore, we can simply add it.
     * - The flux is on the left-hand side of the PDE formulation where we find
     *   a term @f$ div F(Q) @f$. We bring the derivative
     *   over to the test function and then bring the whole term to the
     *   right-hand side Both steps yield a minus sign, which means that the
     *   minus signs cancel out.
     * - The ncp enters the volumetric term as it is. No partial derivative is
     *   exploited. It hence enters the solution with its original sign.
     *   However, we have to bring it to the right-hand side for the time
     *   derivatives. Therefore, we need a minus sign here.
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
     */
    void cellIntegral_patchwise_in_situ_GaussLegendre_functors(
      ::exahype2::CellData&                          cellData,
      const int                                      order,
      const int                                      unknowns,
      const int                                      auxiliaryVariables,
      Flux                                           flux,
      NonConservativeProduct                         nonconservativeProduct,
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


    template <
      typename Solver,
      int      order,
      int      unknowns,
      int      auxiliaryVariables
    >
    void cellIntegral_patchwise_in_situ_GaussLegendre(
      ::exahype2::CellData&                          cellData,
      bool                                           evaluateFlux,
      bool                                           evaluateNonconservativeProduct,
      bool                                           evaluateSource,
      bool                                           evaluatePointSources
    );


    /**
     * Final step of DG algorithm
     *
     * This is the final step of any DG algorithm step, i.e. any Runge-Kutta
     * step. Consult dg.h for a
     * clearer explanation.
     *
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
     * This particular routine assumes that the mass matrix is a diagonal. Therefore,
     * the inversion is trivial: We iterate over all degrees of freedom. This is a
     * d-dimensional for loop (dfor). Per dof, we compute the diagonal, and every
     * unknown then is scaled by the inverse of this diagonal.
     */
    void multiplyWithInvertedMassMatrix_GaussLegendre(
      ::exahype2::CellData&                          cellData,
      const int                                      order,
      const int                                      unknowns,
      const int                                      auxiliaryVariables,
      const double* __restrict__                     MassMatrixDiagonal1d
    );


    /**
     * Compute the maximum eigenvalues over a sequence of cells and store the
     * result in the respective CellData entry.
     */
    void reduceMaxEigenvalue_patchwise_functors(
      ::exahype2::CellData&   cellData,
      const int               order,
      const int               unknowns,
      const int               auxiliaryVariables,
      MaxEigenvalue           maxEigenvalue,
      const double* __restrict__   QuadratureNodes1d
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


#include "CellIntegral.cpph"


