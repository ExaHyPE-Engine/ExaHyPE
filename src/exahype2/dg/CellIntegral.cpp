#include "CellIntegral.h"

#include "peano4/Loop.h"

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
    // const int nodesPerCell = getNodesPerCell(nodesPerAxis);

    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
      const int dofSerialised = peano4::dLinearised(dof, nodesPerAxis);
      for (int var = 0; var < unknowns; var++) {
        cellQout[dofSerialised * outputStride + var] = 0.0;
      }

      if (evaluateFlux) {
        // direction in which flux contributions are being computed
        for (int dim = 0; dim < DIMENSIONS; dim++)
          for (int i = 0; i < nodesPerAxis; i++) {
            // compute contributing node real position
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
            } // var
          } // dim/i combination
      } // if useFlux

      if (evaluateSource) {
        // compute real node position
        tarch::la::Vector<DIMENSIONS, double> position = getQuadraturePoint(x, h, dof, order, quadratureNodes);

        // compute source term contributions
        source(&cellQin[dofSerialised * inputStride], position, t, dt, sourceValues);

#if DIMENSIONS == 2
        double coeff = massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * tarch::la::volume(h);
#else
        double coeff =
          massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * massMatrixDiagonal[dof(2)] * tarch::la::volume(h);
#endif

        for (int var = 0; var < unknowns; var++) {
          cellQout[dofSerialised * outputStride + var] += coeff * sourceValues[var];
        } // var
      } // if useSource

      // @todo Point Source missing

      if (evaluateNonconservativeProduct) {
        // compute real node position
        tarch::la::Vector<DIMENSIONS, double> position = getQuadraturePoint(x, h, dof, order, quadratureNodes);

#if DIMENSIONS == 2
        double coeff = massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * tarch::la::volume(h);
#else
        double coeff =
          massMatrixDiagonal[dof(0)] * massMatrixDiagonal[dof(1)] * massMatrixDiagonal[dof(2)] * tarch::la::volume(h);
#endif

        for (int dim = 0; dim < DIMENSIONS; dim++) {
          // computes gradient values in all directions for the given node
          std::fill_n(gradientValues, strideQ, 0.0);

          const int invDx = 1 / h(dim);

          tarch::la::Vector<DIMENSIONS, int> contributingNodeIndex = dof;
          // computing gradients in direction dim
          for (int node = 0; node < nodesPerAxis; node++) {
            contributingNodeIndex[dim]              = node;
            const double coeffDerivative            = invDx * derivativeOperator[node + nodesPerAxis * dof(dim)];
            const int    contributingNodeSerialised = peano4::dLinearised(contributingNodeIndex, nodesPerAxis);

            for (int var = 0; var < strideQ; var++) {
              gradientValues[var] += coeffDerivative * cellQin[contributingNodeSerialised * inputStride + var];
              assertion(gradientValues[var] == gradientValues[var]);
            } // var
          } // node

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
          } // var
        } // dim
      } // if useNCP
    }); // dof
  } // currentCell

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

void exahype2::dg::reduceMaxEigenvalue_patchwise_functors(
  ::exahype2::CellData<double, double>& cellData,
  const int                             order,
  const int                             unknowns,
  const int                             auxiliaryVariables,
  MaxEigenvalue                         maxEigenvalue,
  const double* NOALIAS                 QuadratureNodes1d
) {
  const int stride = unknowns + auxiliaryVariables;

  for (int cellIndex = 0; cellIndex < cellData.numberOfCells; cellIndex++) {
    double eigenvalue = std::numeric_limits<double>::min();

    peano4::dForEach(order + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      const int dofSerialised = peano4::dLinearised(k, order + 1);
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        double nodeEigenvalue = maxEigenvalue(
          cellData.QOut[cellIndex] + dofSerialised * stride,
          getQuadraturePoint(cellData.cellCentre[cellIndex], cellData.cellSize[cellIndex], k, order, QuadratureNodes1d),
          cellData.t,
          cellData.dt,
          direction
        );

        assertion(nodeEigenvalue >= 0.0);

        eigenvalue = std::max(eigenvalue, nodeEigenvalue);
      }
    });

    cellData.maxEigenvalue[cellIndex] = eigenvalue;
  }
}

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
