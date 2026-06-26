// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AderSolver.h"

namespace benchmarks::exahype2::kernelbenchmarks {
  void handleBoundaryFused(
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
  );

  void predictorFused(
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
    const int                                           numberOfPatches,
    const int                                           targetDevice
  );

  void riemannFused(
    benchmarks::exahype2::kernelbenchmarks::AderSolver& solver,
    SolverCorrectorPrecision*                           faceEstimatesFlattened,
    SolverCorrectorPrecision*                           faceFluxesFlattened,
    const std::uint64_t*                                faceIndices,
    const uint8_t*                                      faceNumbers,
    const uint64_t*                                     cellIndices,
    const double                                        timeStamp,
    const double                                        timeStepSize,
    const double* const                                 patchCentres,
    const double* const                                 patchSizes,
    const int                                           numberOfFaces,
    const int                                           targetDevice
  );

  void correctorFused(
    benchmarks::exahype2::kernelbenchmarks::AderSolver& solver,
    SolverStoragePrecision* NOALIAS const               QInFlattened,
    SolverCorrectorPrecision*                           faceFluxesFlattened,
    const std::uint64_t*                                faceIndices,
    const double                                        timeStamp,
    const double                                        timeStepSize,
    const double* const                                 patchCentres,
    const double* const                                 patchSizes,
    const int                                           numberOfPatches,
    const int                                           targetDevice
  );

} // namespace benchmarks::exahype2::kernelbenchmarks