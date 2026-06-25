// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FusedTasks.h"

#include "celldata/AderSolverQ.h"
#include "celldata/AderSolverQ_old.h"
#include "facedata/AderSolverQEstimates.h"
#include "facedata/AderSolverQFluxEstimates.h"
#include "kernels/AderSolver/FaceIntegral.h"
#include "kernels/AderSolver/FusedSpaceTimePredictorVolumeIntegral.h"
#include "kernels/AderSolver/MaxScaledEigenvalue.h"
#include "kernels/AderSolver/RiemannSolver.h"
#include "peano4/Loop.h"

void benchmarks::exahype2::kernelbenchmarks::handleBoundaryFused(
  benchmarks::exahype2::kernelbenchmarks::AderSolver& solver,
  SolverCorrectorPrecision*                           faceEstimatesFlattened,
  SolverCorrectorPrecision*                           faceFluxesFlattened,
  const uint64_t*                                     faceIndices,
  const uint8_t*                                      faceNumbers,
  const uint64_t*                                     cellIndices,
  const tarch::la::Vector<DIMENSIONS, double>*        cellCentres,
  const tarch::la::Vector<DIMENSIONS, double>*        cellSizes,
  const double                                        t,
  const double                                        dt,
  const int                                           numberOfFaces,
  const int                                           numberOfCells,
  const int                                           targetDevice
) {

  // Check whether boundary condition should even run
  if (not solver.isLastGridSweepOfTimeStep()) {
    return;
  }

#pragma omp parallel for schedule(static)

  for (int boundaryFaceIndex = 0; boundaryFaceIndex < numberOfFaces; ++boundaryFaceIndex) {

    const uint64_t faceIndex  = faceIndices[boundaryFaceIndex];
    const int      faceNumber = faceNumbers[boundaryFaceIndex];
    const uint64_t cellIndex  = cellIndices[boundaryFaceIndex];

    tarch::la::Vector<DIMENSIONS, double> patchSize   = cellSizes[cellIndex];
    tarch::la::Vector<DIMENSIONS, double> patchCentre = cellCentres[cellIndex];
    tarch::la::Vector<DIMENSIONS, double> faceOffset  = patchCentre - 0.5 * patchSize;
    if (faceNumber >= DIMENSIONS) {
      faceOffset(faceNumber % DIMENSIONS) += patchSize(faceNumber % DIMENSIONS);
    }

    constexpr int faceEstimatesSize = facedata::AderSolverQEstimates::Cardinality;
    constexpr int faceFluxesSize    = facedata::AderSolverQFluxEstimates::Cardinality;

    const int  normal     = faceNumber % DIMENSIONS;
    const bool isLeftFace = faceNumber < DIMENSIONS;

    auto* QIn  = &faceEstimatesFlattened[faceIndex * faceEstimatesSize + (!isLeftFace ? 0 : faceEstimatesSize / 2)];
    auto* QOut = &faceEstimatesFlattened[faceIndex * faceEstimatesSize + (!isLeftFace ? faceEstimatesSize / 2 : 0)];
    auto* FOut = &faceFluxesFlattened[faceIndex * faceFluxesSize + (!isLeftFace ? faceFluxesSize / 2 : 0)];

    dfore(dof, AbstractAderSolver::Order + 1, normal, 0) {
      tarch::la::Vector<DIMENSIONS, double> nodePosition;
      int                                   dofSerialised = 0;
      int                                   basis         = 1;
      for (int d = 0; d < DIMENSIONS; d++) {
        nodePosition(d) = faceOffset(d);
        nodePosition(
          d
        ) += (d == normal) ? 0.0 : kernels::AderSolver::Quadrature<SolverStoragePrecision>::nodes[dof(d)] * patchSize(d);
        dofSerialised += (d == normal) ? 0 : dof(d) * basis;
        basis *= (d == normal) ? 1 : (AbstractAderSolver::Order + 1);
      }

      repositories::instanceOfAderSolver.boundaryConditions(
        QIn + dofSerialised * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables),
        QOut + dofSerialised * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables),
        nodePosition,
        patchSize,
        t,
        normal
      );

      repositories::instanceOfAderSolver.flux(
        QOut + dofSerialised * (AbstractAderSolver::NumberOfUnknowns + AbstractAderSolver::NumberOfAuxiliaryVariables),
        nodePosition,
        patchSize,
        t,
        dt,
        normal,
        FOut + dofSerialised * AbstractAderSolver::NumberOfUnknowns
      );
    }
  }
}

void benchmarks::exahype2::kernelbenchmarks::predictorFused(
  benchmarks::exahype2::kernelbenchmarks::AderSolver& solver,
  SolverStoragePrecision* NOALIAS const               QInFlattened,
  SolverStoragePrecision* NOALIAS const               QOutFlattened,
  SolverCorrectorPrecision*                           faceEstimatesFlattened,
  SolverCorrectorPrecision*                           faceFluxesFlattened,
  const std::uint64_t*                                faceIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfCells,
  const int                                           targetDevice
) {

  // Check whether the predictor should even run
  if (not solver.isFirstGridSweepOfTimeStep()) {
    return;
  }

#pragma omp parallel for schedule(static)
  for (int cellIndex = 0; cellIndex < numberOfCells; ++cellIndex) {

    constexpr int cellCardinality = celldata::AderSolverQ::Cardinality;
    auto* const   QNew            = &QInFlattened[cellIndex * cellCardinality];
    auto* const   QOld            = &QOutFlattened[cellIndex * cellCardinality];

    std::copy_n(QNew, cellCardinality, QOld);

    constexpr int BasisElementsPerFace = facedata::AderSolverQEstimates::Cardinality / 2;
    constexpr int FluxElementsPerFace  = facedata::AderSolverQFluxEstimates::Cardinality / 2;

    const tarch::la::Vector<DIMENSIONS, double> patchCentre{
      patchCentres[cellIndex * DIMENSIONS],
      patchCentres[cellIndex * DIMENSIONS + 1]
#if DIMENSIONS == 3
      ,
      patchCentres[cellIndex * DIMENSIONS + 2]
#endif
    };
    const tarch::la::Vector<DIMENSIONS, double> patchSize{
      patchSizes[cellIndex * DIMENSIONS],
      patchSizes[cellIndex * DIMENSIONS + 1]
#if DIMENSIONS == 3
      ,
      patchSizes[cellIndex * DIMENSIONS + 2]
#endif
    };

    SolverCorrectorPrecision* lQhbnd[TWO_TIMES_D] = {
      &faceEstimatesFlattened
        [faceIndices[(cellIndex * TWO_TIMES_D + 0)] * 2 * BasisElementsPerFace + BasisElementsPerFace],
      &faceEstimatesFlattened
        [faceIndices[(cellIndex * TWO_TIMES_D + 1)] * 2 * BasisElementsPerFace + BasisElementsPerFace],
#if DIMENSIONS == 3
      &faceEstimatesFlattened
        [faceIndices[(cellIndex * TWO_TIMES_D + 2)] * 2 * BasisElementsPerFace + BasisElementsPerFace],
#endif
      &faceEstimatesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 0 + DIMENSIONS)] * 2 * BasisElementsPerFace],
      &faceEstimatesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 1 + DIMENSIONS)] * 2 * BasisElementsPerFace],
#if DIMENSIONS == 3
      &faceEstimatesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 2 + DIMENSIONS)] * 2 * BasisElementsPerFace],
#endif
    };

    SolverCorrectorPrecision* lFhbnd[TWO_TIMES_D] = {
      &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 0)] * 2 * FluxElementsPerFace + FluxElementsPerFace],
      &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 1)] * 2 * FluxElementsPerFace + FluxElementsPerFace],
#if DIMENSIONS == 3
      &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 2)] * 2 * FluxElementsPerFace + FluxElementsPerFace],
#endif
      &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 0 + DIMENSIONS)] * 2 * FluxElementsPerFace],
      &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 1 + DIMENSIONS)] * 2 * FluxElementsPerFace],
#if DIMENSIONS == 3
      &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + 2 + DIMENSIONS)] * 2 * FluxElementsPerFace],
#endif
    };

    kernels::AderSolver::fusedSpaceTimePredictorVolumeIntegral<
      SolverStoragePrecision,
      SolverPredictorPrecision,
      SolverCorrectorPrecision>(solver, lQhbnd, lFhbnd, QNew, patchCentre, patchSize, timeStamp, timeStepSize);
  }
}

void benchmarks::exahype2::kernelbenchmarks::riemannFused(
  benchmarks::exahype2::kernelbenchmarks::AderSolver& solver,
  SolverCorrectorPrecision*                           faceEstimatesFlattened,
  SolverCorrectorPrecision*                           faceFluxesFlattened,
  const std::uint64_t*                                faceIndices,
  const uint8_t*                                      faceNumbers,
  const uint64_t*                                     cellIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 cellCentres,
  const double* const                                 cellSizes,
  const int                                           numberOfFaces,
  const int                                           targetDevice
) {
  // Check whether the predictor should even run
  if (not solver.isLastGridSweepOfTimeStep()) {
    return;
  }

#pragma omp parallel for schedule(static)
  for (int f = 0; f < numberOfFaces; ++f) {

    const uint64_t faceIndex  = faceIndices[f];
    const int      faceNumber = faceNumbers[f];
    const uint64_t cellIndex  = cellIndices[f];

    tarch::la::Vector<DIMENSIONS, double> patchSize   = cellSizes[cellIndex];
    tarch::la::Vector<DIMENSIONS, double> patchCentre = cellCentres[cellIndex];
    tarch::la::Vector<DIMENSIONS, double> faceOffset  = patchCentre;
    faceOffset[faceNumber % DIMENSIONS] -= 0.5 * patchSize(faceNumber % DIMENSIONS);
    if (faceNumber >= DIMENSIONS) {
      faceOffset(faceNumber % DIMENSIONS) += patchSize(faceNumber % DIMENSIONS);
    }

    constexpr int faceEstimatesSize = facedata::AderSolverQEstimates::Cardinality;
    constexpr int faceFluxesSize    = facedata::AderSolverQFluxEstimates::Cardinality;

    const int normal = faceNumber % DIMENSIONS;

    auto* QL = &faceEstimatesFlattened[faceIndex * faceEstimatesSize + 0];
    auto* QR = &faceEstimatesFlattened[faceIndex * faceEstimatesSize + faceEstimatesSize / 2];
    auto* FL = &faceFluxesFlattened[faceIndex * faceFluxesSize + 0];
    auto* FR = &faceFluxesFlattened[faceIndex * faceFluxesSize + faceFluxesSize / 2];

    bool isBoundary = false;
    for (int d = 0; d < DIMENSIONS; d++) {
      isBoundary |= tarch::la::equals(faceOffset(normal), DomainOffset(normal));
      isBoundary |= tarch::la::equals(faceOffset(normal), DomainOffset(normal) + DomainSize(normal));
    }

    kernels::AderSolver::riemannSolver<SolverCorrectorPrecision>(
      solver,
      FL,
      FR,
      QL,
      QR,
      timeStamp,
      timeStepSize,
      faceOffset,
      patchSize,
      normal,
      isBoundary,
      faceNumber
    );
  }
}

void benchmarks::exahype2::kernelbenchmarks::correctorFused(
  benchmarks::exahype2::kernelbenchmarks::AderSolver& solver,
  SolverStoragePrecision* NOALIAS const               QInFlattened,
  SolverCorrectorPrecision*                           faceFluxesFlattened,
  const std::uint64_t*                                faceIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfCells,
  const int                                           targetDevice
) {

  // Check whether the predictor should even run
  if (not solver.isLastGridSweepOfTimeStep()) {
    return;
  }

  double maxEigenvalue = 0.;
#define OMP_REDUCTION_CLAUSE reduction(max : maxEigenvalue)

#pragma omp parallel for schedule(static) OMP_REDUCTION_CLAUSE
  for (int cellIndex = 0; cellIndex < numberOfCells; ++cellIndex) {

    constexpr int cellCardinality = celldata::AderSolverQ::Cardinality;
    auto* const   QNew            = &QInFlattened[cellIndex * cellCardinality];

    constexpr int FluxElementsPerFace = facedata::AderSolverQFluxEstimates::Cardinality / 2;

    const tarch::la::Vector<DIMENSIONS, double> patchCentre{
      patchCentres[cellIndex * DIMENSIONS],
      patchCentres[cellIndex * DIMENSIONS + 1]
#if DIMENSIONS == 3
      ,
      patchCentres[cellIndex * DIMENSIONS + 2]
#endif
    };
    const tarch::la::Vector<DIMENSIONS, double> patchSize{
      patchSizes[cellIndex * DIMENSIONS],
      patchSizes[cellIndex * DIMENSIONS + 1]
#if DIMENSIONS == 3
      ,
      patchSizes[cellIndex * DIMENSIONS + 2]
#endif
    };

    for (int d = 0; d < DIMENSIONS; d++) {
      const int direction = d;

      // Negative face
      kernels::AderSolver::faceIntegral(
        QNew,
        &faceFluxesFlattened
          [faceIndices[(cellIndex * TWO_TIMES_D + d)] * 2 * FluxElementsPerFace + FluxElementsPerFace],
        d,
        0., // orientation,
        1. / patchSize[d],
        timeStepSize
      );

      // Positive face
      kernels::AderSolver::faceIntegral(
        QNew,
        &faceFluxesFlattened[faceIndices[(cellIndex * TWO_TIMES_D + d + DIMENSIONS)] * 2 * FluxElementsPerFace],
        d,
        1., // orientation,
        1. / patchSize[d],
        timeStepSize
      );
    }

    maxEigenvalue = kernels::AderSolver::
      maxScaledEigenvalue(repositories::instanceOfAderSolver, QNew, patchCentre, patchSize, timeStamp, timeStepSize);
  }

  solver.setMaxEigenvalue(maxEigenvalue);
}
