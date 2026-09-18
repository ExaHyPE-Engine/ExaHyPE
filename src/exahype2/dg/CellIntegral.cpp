// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellIntegral.h"

#include "peano4/Loop.h"

/**
 * Writes the volumetric part of the weak form into cellData.QOut, one cell and one degree of
 * freedom at a time.
 *
 * The output carries neither the previous time step's solution nor a multiplication with the
 * time step size or the inverse mass matrix. Point sources are not implemented by this kernel,
 * and all cells carry the same polynomial order and solve the same PDE.
 *
 * @param order Order of the polynomials, so each cell holds order+1 nodes per axis.
 * @param unknowns Number of entries of Q that evolve according to the hyperbolic system.
 * @param auxiliaryVariables Number of entries of Q that the PDE leaves unchanged.
 * @param quadratureNodes Positions of the quadrature nodes along one axis of the reference cell.
 */
void exahype2::dg::cellIntegralPatchwiseInSituGaussLegendreFunctors(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  const int                             auxiliaryVariables,
  const Flux                            flux,
  const NonConservativeProduct          nonconservativeProduct,
  const Source                          source,
  [[maybe_unused]] const PointSources&  pointSources,
  const double* NOALIAS                 quadratureNodes,
  const double* NOALIAS                 massMatrixDiagonal,
  const double* NOALIAS                 stiffnessMatrix,
  const double* NOALIAS                 derivativeOperator,
  const bool                            evaluateFlux,
  const bool                            evaluateNonconservativeProduct,
  const bool                            evaluateSource,
  [[maybe_unused]] const bool           evaluatePointSources
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
        for (int dim = 0; dim < DIMENSIONS; dim++) {
          for (int i = 0; i < nodesPerAxis; i++) {
            tarch::la::Vector<DIMENSIONS, int> contributingNodeIndex = dof;
            contributingNodeIndex[dim]                               = i;
            const tarch::la::Vector<DIMENSIONS, double> position =
              getQuadraturePoint(x, h, contributingNodeIndex, quadratureNodes);
            const int contributingNodeSerialised = peano4::dLinearised(contributingNodeIndex, nodesPerAxis);

            flux(
              &cellQin[static_cast<std::ptrdiff_t>(contributingNodeSerialised * inputStride)],
              position,
              t,
              dt,
              dim, // normal
              fluxValues
            );

            // The coefficient is a tensor product of the stiffness matrix along the derivative
            // direction and the mass matrix along the other directions. The h-terms cancel
            // along the derivative direction.
            double coeff = stiffnessMatrix[i + dof(dim) * nodesPerAxis];
            for (int dd = 0; dd < DIMENSIONS; dd++) {
              coeff *= (dd == dim) ? 1.0 : massMatrixDiagonal[dof(dd)] * h(dd);
            }

            for (int var = 0; var < unknowns; var++) {
              cellQout[dofSerialised * outputStride + var] += coeff * fluxValues[var];
            }
          }
        }
      }

      if (evaluateSource) {
        const tarch::la::Vector<DIMENSIONS, double> position = getQuadraturePoint(x, h, dof, quadratureNodes);

        source(&cellQin[static_cast<std::ptrdiff_t>(dofSerialised * inputStride)], position, t, dt, sourceValues);

#if DIMENSIONS == 2
        const double coeff = massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * tarch::la::volume(h);
#else
        const double coeff =
          massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * massMatrixDiagonal[dof(2)] * tarch::la::volume(h);
#endif

        for (int var = 0; var < unknowns; var++) {
          cellQout[dofSerialised * outputStride + var] += coeff * sourceValues[var];
        }
      }

      // @todo Point Source missing

      if (evaluateNonconservativeProduct) {
        const tarch::la::Vector<DIMENSIONS, double> position = getQuadraturePoint(x, h, dof, quadratureNodes);

#if DIMENSIONS == 2
        const double coeff = massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * tarch::la::volume(h);
#else
        const double coeff =
          massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * massMatrixDiagonal[dof(2)] * tarch::la::volume(h);
#endif

        for (int dim = 0; dim < DIMENSIONS; dim++) {
          std::fill_n(gradientValues, strideQ, 0.0);

          const double invDx = 1.0 / h(dim);

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
            &cellQin[static_cast<std::ptrdiff_t>(dofSerialised * inputStride)],
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
 * Computes the maximum scaled eigenvalue over a sequence of cells.
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
double exahype2::dg::reduceMaxScaledEigenvaluePatchwiseFunctors(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  const int                             auxiliaryVariables,
  const MaxEigenvalue                   maxEigenvalue,
  const double* NOALIAS                 quadratureNodes1D
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
          cellData.QOut[cellIndex] + static_cast<std::ptrdiff_t>(dofSerialised * stride),
          getQuadraturePoint(cellData.cellCentre[cellIndex], cellData.cellSize[cellIndex], k, quadratureNodes1D),
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
 * Copies the unknowns and auxiliary variables of one node from cellQin to cellQout.
 */
void exahype2::dg::copySolution(
  const double* NOALIAS                     cellQin,
  const int                                 order,
  const int                                 unknowns,
  const int                                 auxiliaryVariables,
  const tarch::la::Vector<DIMENSIONS, int>& node,
  double* NOALIAS                           cellQout
) {
  const int strideQ        = unknowns + auxiliaryVariables;
  const int nodeSerialised = peano4::dLinearised(node, order + 1);
  for (int var = 0; var < strideQ; var++) {
    cellQout[nodeSerialised * strideQ + var] = cellQin[nodeSerialised * strideQ + var];
  }
}

/**
 * Divides cellData.QOut by the diagonal of the mass matrix, which closes one Runge-Kutta stage.
 *
 * The input holds the flux, source and Riemann contributions in the weak image space. The
 * routine requires a diagonal mass matrix.
 */
void exahype2::dg::multiplyWithInvertedMassMatrixGaussLegendre(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  [[maybe_unused]] const int            auxiliaryVariables,
  const double* NOALIAS                 massMatrixDiagonal1D
) {
  const int stride = unknowns;

  for (int currentCell = 0; currentCell < cellData.numberOfCells; currentCell++) {
    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      const int       dofSerialised = peano4::dLinearised(dof, order + 1);
      double* NOALIAS cellQInOut    = cellData.QOut[currentCell];

#if DIMENSIONS == 2
      const double diagonal = massMatrixDiagonal1D[dof(0)] * massMatrixDiagonal1D[dof(1)]
        * (cellData.cellSize[currentCell])(0) * (cellData.cellSize[currentCell])(1);
#else
      const double diagonal = massMatrixDiagonal1D[dof(0)] * massMatrixDiagonal1D[dof(1)] * massMatrixDiagonal1D[dof(2)]
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
