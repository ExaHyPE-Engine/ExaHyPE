// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellIntegral.h"

#include "peano4/Loop.h"

/**
 * Volumetric kernel for Gauss-Legendre shape functions using functors
 *
 * Computes the @f$ \hat Q @f$ from the generic Discontinuous Galerkin
 * explanation. That is the volumetric part of the weak formulation. It
 * does not include the previous time step's solution, and it is not (yet)
 * multiplied with the time step size or the inverse of the mass matrix.
 *
 * The whole implementation works cell-wisely, i.e. runs through cell by
 * cell. Within each cell, it works dof-wisely, i.e. it runs through each
 * individual dof and accumulates all the PDE contributions to this dof.
 * The second loop runs over the degrees of freedom, and dof holds the
 * d-dimensional index of the one currently handled.
 *
 * ## Solver flavour
 *
 * We rely on Gauss-Legendre quadrature nodes on tensor product spaces, i.e.
 * we exploit the fact that the mass matrix has diagonal form. Consequently,
 * we can construct all involved cell-local operators on-the-fly by
 * multiplying their 1d counterparts.
 *
 * We run over the patches one by one (patchwise). We do not exploit that we
 * have to do the same operation for all dofs of all patches.
 *
 * We evaluate all operators in-situ, i.e. get their output and apply it
 * immediately. See the discussion below that each dof's flux evaluation
 * needs all the fluxes in that row of dofs. Consequently, we evaluate
 * each flux p times if we have p quadrature points per axis. This is
 * redundant work. In return, in-situ implies that we don't have to store
 * data temporarily.
 *
 * ## Flux evaluation
 *
 * This version of the volumetric kernel uses the Gauss Legendre property,
 * i.e. the orthogonality of the polynomials, plus the tensor product style
 * of the shape functions aggressively.
 *
 * For the flux, we first loop over the dimensions (dim) as we have
 * fluxes in all d directions. We are interested in how the dof under
 * consideration changes due to all the fluxes around. So we want to
 * find out what happens if we test against the test function @f$ \phi @f$
 * in that dof.
 *
 * For the flux, tensor products and orthogonality means that the flux
 * in x-direction in any point really only interferes with the unknowns
 * along the x-direction along this same line: We test @f$ F(Q) \partial_x \phi @f$
 * and assume that @f$ \phi = \phi _{1d}(x)\phi _{1d}(y)\phi _{1d}(z) @f$.
 * Furthermore, let us represent @f$ F(Q) @f$ with the same polynomials,
 * too. In the product of the flux with the test function (which now
 * consists of 2d ingredients), all the polynomials besides the @f$ \partial _x \phi(x) @f$
 * are orthogonal - the fact that the polynomials have been chosen as
 * orthogonal ones does not make their derivatives orthogonal, too.
 *
 * So all the shape functions spanning F where the y and z coordinates do
 * not match cancel out in the example above if the test functions don't
 * have the same coordinates. We are left with all the shapes along the
 * x-axis within the cubic cell. Analogous arguments hold for y and z.
 *
 * ## Algebraic source
 *
 * The treatment of the source term is close to trivial: We assume that
 * we can represent the source @f$ S(Q) @f$ in the shape space. Again,
 * we exploit the orthogonality. If we want to test what the impact of
 * the source is if we test against the dof's test function, the
 * orthogonality implies that we only have to take S(Q) in that dof
 * multiplied with its shape function. We get a mass matrix entry.
 *
 * ## Non-conservative product
 *
 * The non-conservative product once again relies on a very crude numerical
 * approximation: We assume that @f$ B_x(Q) \partial _x Q @f$ can be
 * represented within our orthogonal shape space. Consequently, it is
 * tried exactly in the same way as the algebraic source.
 *
 * This approach seems to be inaccurate, but we have to keep in mind
 * that B can be highly non-linear. So we don't know anything about the
 * analytical shape of @f$ B_x(Q) \partial _x Q @f$ anyway: It could be
 * any type of polynomial. So it is convenient to assume that it is once
 * again from the same space as @f$ \phi @f$.
 *
 * ## Signs in front of the individual terms
 *
 * We rely on the formulation as discussed on the page Discontinuous
 * Galerkin. See dg.h or the doxygen html pages for details. This routine
 * computes @f$ \partial _t Q @f$. However, all PDE terms besides the source
 * arise on the left-hand side of the PDE, i.e. on the same side as the time
 * derivative. We therefore have to bring these terms to the right which
 * gives us an additional minus sign.
 *
 * Here's the rationale behind the volumetric terms:
 *
 * - The source term is already on the right-hand side in our PDE
 *   formulation. Therefore, we can simply add it.
 * - The flux is on the left-hand side of the PDE formulation where we find
 *   a term @f$ div F(Q) @f$. We bring the derivative
 *   over to the test function and then bring the whole term to the
 *   right-hand side. Both steps yield a minus sign, which means that the
 *   minus signs cancel out.
 * - The ncp enters the volumetric term as it is. No partial derivative is
 *   exploited. It hence enters the solution with its original sign.
 *   However, we have to bring it to the right-hand side for the time
 *   derivatives. Therefore, we need a minus sign here.
 *
 * ## Arguments
 *
 * @param cellData The routine computes the DG cell contributions over
 *   multiple cells in one go. cellData is the wrapper around these cells,
 *   i.e. holds the pointers to input and output data, time step sizes, ...
 *   However, all cells carry the same polynomial degree and solve the
 *   same PDE.
 *
 * @param order Order of underlying dg polynomials. This specifies the number
 *   of nodes in each dimension, which is order+1 (you need two points to specify
 *   a linear polynomial for example). The order has to be the same for all cells
 *   handed over via cellData.
 *
 * @param unknowns Number of unknowns whose values must be updated. If you
 *   solve a PDE with n equations over a function Q with n+m entries, then n
 *   is the number of unknowns that evolve according to the hyperbolic system
 *   of PDEs, and m are auxiliary (material) parameters. They do not change due
 *   to the PDE (though any user function might want to alter them).
 *
 * @param auxiliaryVariables Auxiliary variables. See description of unknowns.
 *
 * @param quadratureNodes Location of quadrature nodes along one dimension in a
 *   reference element (length). We usually expect the nodes to be
 *   Gauss-Legendre or Gauss-Lobatto quadrature points, but we don't really
 *   bake these considerations into the present routine. However, we do
 *   hardcode the information that we work with a Cartesian tensor product
 *   layout of the dofs, i.e. the (x,y,z) position can be computed per
 *   entry with y and z being independent of x. See getQuadraturePoint()
 *   for some documentation.
 */
void exahype2::dg::cellIntegral_patchwise_in_situ_GaussLegendre_functors(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  const int                             auxiliaryVariables,
  Flux                                  flux,
  NonConservativeProduct                nonconservativeProduct,
  Source                                source,
  PointSources /*pointSources*/,
  const double* NOALIAS quadratureNodes,
  const double* NOALIAS massMatrixDiagonal,
  const double* NOALIAS stiffnessMatrix,
  const double* NOALIAS derivativeOperator,
  bool                  evaluateFlux,
  bool                  evaluateNonconservativeProduct,
  bool                  evaluateSource,
  bool /*evaluatePointSources*/
) {
  const int inputStride    = unknowns + auxiliaryVariables;
  const int outputStride   = unknowns;
  double*   fluxValues     = evaluateFlux ? new double[unknowns] : nullptr;
  double*   sourceValues   = evaluateSource ? new double[unknowns] : nullptr;
  double*   gradientValues = evaluateNonconservativeProduct ? new double[(unknowns + auxiliaryVariables)] : nullptr;
  double*   ncpValues      = evaluateNonconservativeProduct ? new double[unknowns] : nullptr;

  for (int currentCell = 0; currentCell < cellData.numberOfCells; currentCell++) {
    const double* NOALIAS                       cellQin  = cellData.QIn[currentCell];
    double* NOALIAS                             cellQout = cellData.QOut[currentCell];
    const tarch::la::Vector<DIMENSIONS, double> x        = cellData.cellCentre[currentCell];
    const tarch::la::Vector<DIMENSIONS, double> h        = cellData.cellSize[currentCell];
    const double                                t        = cellData.t;
    const double                                dt       = cellData.dt;

    const int nodesPerAxis = order + 1;
    const int strideQ      = unknowns + auxiliaryVariables;

    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      const int dofSerialised = peano4::dLinearised(dof, nodesPerAxis);
      for (int var = 0; var < unknowns; var++) {
        cellQout[dofSerialised * outputStride + var] = 0.0;
      }

      if (evaluateFlux) {
        for (int dim = 0; dim < DIMENSIONS; dim++)
          for (int i = 0; i < nodesPerAxis; i++) {
            tarch::la::Vector<DIMENSIONS, int> contributingNodeIndex = dof;
            contributingNodeIndex[dim]                               = i;
            tarch::la::Vector<DIMENSIONS, double> position =
              getQuadraturePoint(x, h, contributingNodeIndex, order, quadratureNodes);
            const int contributingNodeSerialised = peano4::dLinearised(contributingNodeIndex, nodesPerAxis);

            flux(
              &cellQin[contributingNodeSerialised * inputStride],
              position,
              t,
              dt,
              dim, // normal
              fluxValues
            );

            // Coefficient within matrix is tensor product over dimensions, too.
            // So we take the stiffness matrix in the direction that we are
            // evaluating and multiply it for the d-1 remaining directions with
            // the mass matrix. We integrate over the whole cell, but the h-terms
            // are eliminated along the derivative direction.
            double coeff = stiffnessMatrix[i + dof(dim) * nodesPerAxis];
            for (int dd = 0; dd < DIMENSIONS; dd++) {
              coeff *= (dd == dim) ? 1.0 : massMatrixDiagonal[dof(dd)] * h(dd);
            }

            for (int var = 0; var < unknowns; var++) {
              cellQout[dofSerialised * outputStride + var] += coeff * fluxValues[var];
            }
          }
      }

      if (evaluateSource) {
        tarch::la::Vector<DIMENSIONS, double> position = getQuadraturePoint(x, h, dof, order, quadratureNodes);

        source(&cellQin[dofSerialised * inputStride], position, t, dt, sourceValues);

#if DIMENSIONS == 2
        double coeff = massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * tarch::la::volume(h);
#else
        double coeff =
          massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * massMatrixDiagonal[dof(2)] * tarch::la::volume(h);
#endif

        for (int var = 0; var < unknowns; var++) {
          cellQout[dofSerialised * outputStride + var] += coeff * sourceValues[var];
        }
      }

      // @todo Point Source missing

      if (evaluateNonconservativeProduct) {
        tarch::la::Vector<DIMENSIONS, double> position = getQuadraturePoint(x, h, dof, order, quadratureNodes);

#if DIMENSIONS == 2
        double coeff = massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * tarch::la::volume(h);
#else
        double coeff =
          massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * massMatrixDiagonal[dof(2)] * tarch::la::volume(h);
#endif

        for (int dim = 0; dim < DIMENSIONS; dim++) {
          std::fill_n(gradientValues, strideQ, 0.0);

          const int invDx = 1 / h(dim);

          tarch::la::Vector<DIMENSIONS, int> contributingNodeIndex = dof;
          for (int node = 0; node < nodesPerAxis; node++) {
            contributingNodeIndex[dim]              = node;
            const double coeffDerivative            = invDx * derivativeOperator[node + nodesPerAxis * dof(dim)];
            const int    contributingNodeSerialised = peano4::dLinearised(contributingNodeIndex, nodesPerAxis);

            for (int var = 0; var < strideQ; var++) {
              gradientValues[var] += coeffDerivative * cellQin[contributingNodeSerialised * inputStride + var];
              assertion(gradientValues[var] == gradientValues[var]);
            }
          }

          nonconservativeProduct(
            &cellQin[dofSerialised * inputStride],
            gradientValues,
            position,
            t,
            dt,
            dim,
            ncpValues
          );

          for (int var = 0; var < unknowns; var++) {
            assertion(ncpValues[var] == ncpValues[var]);
            cellQout[dofSerialised * outputStride + var] -= coeff * ncpValues[var];
          }
        }
      }
    });
  }

  if (evaluateFlux) {
    delete[] fluxValues;
  }
  if (evaluateSource) {
    delete[] sourceValues;
  }
  if (evaluateNonconservativeProduct) {
    delete[] gradientValues;
    delete[] ncpValues;
  }
}

/**
 * Compute the maximum scaled eigenvalue over a sequence of cells
 *
 * The result per cell is stored in the respective CellData entry. The scaled
 * eigenvalue of a quadrature node is the sum of its directional eigenvalues
 * divided by the cell width, so it carries the cell size and the number of
 * space dimensions, and the admissible time step size is a fraction of its
 * inverse.
 *
 * @return The maximum physical (unscaled) eigenvalue over all cells, which the
 *   solver statistics report.
 */
double exahype2::dg::reduceMaxScaledEigenvalue_patchwise_functors(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  const int                             auxiliaryVariables,
  MaxEigenvalue                         maxEigenvalue,
  const double* NOALIAS                 QuadratureNodes1d
) {
  const int stride = unknowns + auxiliaryVariables;

  double maxPhysicalEigenvalue = 0.0;
  for (int cellIndex = 0; cellIndex < cellData.numberOfCells; cellIndex++) {
    const double inverseCellWidth = 1.0 / tarch::la::min(cellData.cellSize[cellIndex]);
    double       scaledEigenvalue = 0.0;

    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      const int dofSerialised  = peano4::dLinearised(k, order + 1);
      double    nodeEigenvalue = 0.0;
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        const double directionalEigenvalue = maxEigenvalue(
          cellData.QOut[cellIndex] + dofSerialised * stride,
          getQuadraturePoint(cellData.cellCentre[cellIndex], cellData.cellSize[cellIndex], k, order, QuadratureNodes1d),
          cellData.t,
          cellData.dt,
          direction
        );

        assertion(directionalEigenvalue >= 0.0);

        maxPhysicalEigenvalue = std::max(maxPhysicalEigenvalue, directionalEigenvalue);
        nodeEigenvalue += directionalEigenvalue * inverseCellWidth;
      }

      scaledEigenvalue = std::max(scaledEigenvalue, nodeEigenvalue);
    });

    cellData.maxEigenvalue[cellIndex] = scaledEigenvalue;
  }

  return maxPhysicalEigenvalue;
}

/**
 * Copy solution over for one node
 *
 * The signature carries more variables than the body needs, so that all
 * internal routines stay as close to each other as possible.
 *
 * @todo Move this to the micro-kernel notation Dominic introduced for ADER,
 *   once the enumerators are available.
 */
void exahype2::dg::internal::copySolution(
  const double* NOALIAS                     cellQin,
  const int                                 order,
  const int                                 unknowns,
  const int                                 auxiliaryVariables,
  const tarch::la::Vector<DIMENSIONS, int>& node,
  double* NOALIAS                           cellQout
) {
  const int nodeSerialised = peano4::dLinearised(node, order + 1);
  for (int var = 0; var < unknowns + auxiliaryVariables; var++) {
    cellQout[nodeSerialised + var] = cellQin[nodeSerialised + var];
  }
}

/**
 * Final step of a DG algorithm step
 *
 * This is the final step of any DG algorithm step, i.e. any Runge-Kutta step.
 * Consult dg.h for a clearer explanation.
 *
 * - The output points to the new time step's data. This is usually
 *
 *        fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates
 *
 *   subject to some shifts.
 * - The output already holds the flux contributions, source terms, and
 *   so forth, but these contributions are stored in the weak image space,
 *   i.e. we still have to multiply them with the inverse of a mass matrix.
 *   The output also contains the Riemann solutions, back-propagated into
 *   the volume.
 * - The output holds the flux contributions in a Runge-Kutta sense, i.e. the
 *   increment that one Runge-Kutta stage contributes.
 *
 * This particular routine assumes that the mass matrix is a diagonal. Therefore,
 * the inversion is trivial: We iterate over all degrees of freedom. This is a
 * d-dimensional for loop (peano4::dForEach). Per dof, we compute the diagonal, and
 * every unknown then is scaled by the inverse of this diagonal.
 */
void exahype2::dg::multiplyWithInvertedMassMatrix_GaussLegendre(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  const int /*auxiliaryVariables*/,
  const double* NOALIAS massMatrixDiagonal1d
) {
  const int stride = unknowns;

  for (int currentCell = 0; currentCell < cellData.numberOfCells; currentCell++) {
    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      const int       dofSerialised = peano4::dLinearised(dof, order + 1);
      double* NOALIAS cellQInOut    = cellData.QOut[currentCell];

#if DIMENSIONS == 2
      double diagonal = massMatrixDiagonal1d[dof(0)] * massMatrixDiagonal1d[dof(1)]
        * (cellData.cellSize[currentCell])(0) * (cellData.cellSize[currentCell])(1);
#else
      double diagonal = massMatrixDiagonal1d[dof(0)] * massMatrixDiagonal1d[dof(1)] * massMatrixDiagonal1d[dof(2)]
        * (cellData.cellSize[currentCell])(0) * (cellData.cellSize[currentCell])(1)
        * (cellData.cellSize[currentCell])(2);
#endif

      assertion(diagonal > 0.0);

      for (int var = 0; var < unknowns; var++) {
        cellQInOut[dofSerialised * stride + var] = 1.0 / diagonal * cellQInOut[dofSerialised * stride + var];
      }
    });
  }
}
