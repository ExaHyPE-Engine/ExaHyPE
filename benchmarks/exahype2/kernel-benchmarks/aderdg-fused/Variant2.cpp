/**
 * This is variant 2 of the fused kernels.
 * In this variant, we will assume that faceData is constructed per cell, and that
 * it contains data both from one's own cell and one's neighbour, e.g. it has access
 * to the side of the faces that belong to the neighbour.
 * We assume that this data is not shared with the neighbour however, but copied from
 * the neighbour before the start of the kernels.
 *
 * This means that the Riemann kernel can and must be performed within any given cell,
 * and will therefore be performed twice for any given face.
 *
 * This can be implemented efficiently as one single enclave task, but requires the
 * data from the faces to be copied over between tasks, which requires some fiddly
 * management of the faces. It also means that each (potentially costly) Riemann
 * kernel must be performed twice.
 * You will not see these prices for the cheap Rusanov kernels, but believe me this
 * can become quite costly.
 *
 * Upsides:
 *    Each cell can be performed independently
 *    only one dependency which occurs between timeSteps
 *
 * Downsides:
 *    somewhat fiddly and bug-prone management of faces for copying
 *    Requires copying data between faces of adjacent cells
 *    Each Riemann solver is computed twice, which can become costly
 */

#include "CellFaceData.h"
#include "exahype2/CellData.h"
#include "kernels/AderSolver/CellData.h"
#include "kernels/AderSolver/FaceIntegral.h"
#include "kernels/AderSolver/FusedSpaceTimePredictorVolumeIntegral.h"
#include "kernels/AderSolver/MaxScaledEigenvalue.h"
#include "kernels/AderSolver/RiemannSolver.h"
#include "repositories/SolverRepository.h"
#include "Utils.h"
#include "Variants.h"

#include <cstring>

tarch::logging::Log variant2::_log("variants3::");

using namespace benchmarks::exahype2::kernelbenchmarks;

inline void initialTask(
  exahype2::CellData<SolverPrecision, SolverPrecision>*     myCellData,
  exahype2::CellFaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                               cellId,
  const int                               faceId,
  tarch::timing::Measurement&             measurement
) {

  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  SolverPrecision* lQhbnd[2 * Dimensions] = {
    &myFaceData->QIn[faceId][0][kernels::AderSolver::getBndFaceSize()],
    &myFaceData->QIn[faceId][1][kernels::AderSolver::getBndFaceSize()],
#if Dimensions == 3
    &myFaceData->QIn[faceId][2][kernels::AderSolver::getBndFaceSize()],
#endif
    &myFaceData->QIn[faceId][Dimensions + 0][0],
    &myFaceData->QIn[faceId][Dimensions + 1][0]
#if Dimensions == 3
    ,
    &myFaceData->QIn[faceId][Dimensions + 2][0]
#endif
  };

  SolverPrecision* lFhbnd[2 * Dimensions] = {
    &myFaceData->QOut[faceId][0][kernels::AderSolver::getBndFluxSize()],
    &myFaceData->QOut[faceId][1][kernels::AderSolver::getBndFluxSize()],
#if Dimensions == 3
    &myFaceData->QOut[faceId][2][kernels::AderSolver::getBndFluxSize()],
#endif
    &myFaceData->QOut[faceId][Dimensions + 0][0],
    &myFaceData->QOut[faceId][Dimensions + 1][0]
#if Dimensions == 3
    ,
    &myFaceData->QOut[faceId][Dimensions + 2][0]
#endif
  };

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  int numberOfIterations = kernels::AderSolver::fusedSpaceTimePredictorVolumeIntegral<SolverPrecision, SolverPrecision, SolverPrecision>(
    repositories::instanceOfAderSolver,
    lQhbnd,
    lFhbnd,
    myCellData->QIn[cellId],
    myCellData->cellCentre[cellId],
    myCellData->cellSize[cellId],
    myCellData->t,
    myCellData->dt
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());
}


void runKernels(
  exahype2::CellData<SolverPrecision, SolverPrecision>*     myCellData,
  exahype2::CellFaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                               cellId,
  const int                               faceId,
  tarch::timing::Measurement&             measurement
) {

  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  SolverPrecision* lQhbnd[2 * Dimensions] = {
    &myFaceData->QIn[faceId][0][kernels::AderSolver::getBndFaceSize()],
    &myFaceData->QIn[faceId][1][kernels::AderSolver::getBndFaceSize()],
#if Dimensions == 3
    &myFaceData->QIn[faceId][2][kernels::AderSolver::getBndFaceSize()],
#endif
    &myFaceData->QIn[faceId][Dimensions + 0][0],
    &myFaceData->QIn[faceId][Dimensions + 1][0]
#if Dimensions == 3
    ,
    &myFaceData->QIn[faceId][Dimensions + 2][0]
#endif
  };

  SolverPrecision* lFhbnd[2 * Dimensions] = {
    &myFaceData->QOut[faceId][0][kernels::AderSolver::getBndFluxSize()],
    &myFaceData->QOut[faceId][1][kernels::AderSolver::getBndFluxSize()],
#if Dimensions == 3
    &myFaceData->QOut[faceId][2][kernels::AderSolver::getBndFluxSize()],
#endif
    &myFaceData->QOut[faceId][Dimensions + 0][0],
    &myFaceData->QOut[faceId][Dimensions + 1][0]
#if Dimensions == 3
    ,
    &myFaceData->QOut[faceId][Dimensions + 2][0]
#endif
  };

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  for (int d = 0; d < Dimensions; d++) {
    const int direction = d;

    kernels::AderSolver::riemannSolver<SolverPrecision>(
      repositories::instanceOfAderSolver,
      &myFaceData->QOut[faceId][d][0], // lFhbnd[d+Dimensions],
      &myFaceData->QOut[faceId][d][kernels::AderSolver::getBndFluxSize()],
      &myFaceData->QIn[faceId][d][0], // lFhbnd[d+Dimensions],
      &myFaceData->QIn[faceId][d][kernels::AderSolver::getBndFaceSize()],
      0.5 * (myFaceData->t[faceId] + myFaceData->t[faceId]),
      0.5 * (myFaceData->dt[faceId] + myFaceData->dt[faceId]),
      0.5 * (myFaceData->cellCentre[faceId] + myFaceData->cellCentre[faceId]),
      0.5 * (myFaceData->cellSize[faceId] + myFaceData->cellSize[faceId]),
      direction
    );

    const double inverseDxDirection = 1.0 / myCellData->cellSize[cellId][d];
    // Negative face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      &myFaceData->QOut[faceId][d][kernels::AderSolver::getBndFluxSize()],
      direction,
      0,
      inverseDxDirection,
      myCellData->dt
    );

    tarch::la::Vector<Dimensions, double> faceCentre = myFaceData->cellCentre[faceId];
    faceCentre[d] += 0.5 * myFaceData->cellSize[faceId][d];

    kernels::AderSolver::riemannSolver<SolverPrecision>(
      repositories::instanceOfAderSolver,
      &myFaceData->QOut[faceId][d + Dimensions][0], // lFhbnd[d+Dimensions],
      &myFaceData->QOut[faceId][d + Dimensions][kernels::AderSolver::getBndFluxSize()],
      &myFaceData->QIn[faceId][d + Dimensions][0], // lFhbnd[d+Dimensions],
      &myFaceData->QIn[faceId][d + Dimensions][kernels::AderSolver::getBndFaceSize()],
      0.5 * (myFaceData->t[faceId] + myFaceData->t[faceId]),
      0.5 * (myFaceData->dt[faceId] + myFaceData->dt[faceId]),
      faceCentre,
      myFaceData->cellSize[faceId],
      direction
    );

    faceCentre[d] -= myFaceData->cellSize[faceId][d];
    // Positive face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      &myFaceData->QOut[faceId][d + Dimensions][0],
      direction,
      1,
      inverseDxDirection,
      myCellData->dt
    );

  } // for d

  myCellData->maxEigenvalue[cellId] = kernels::AderSolver::maxScaledEigenvalue(
    repositories::instanceOfAderSolver,
    myCellData->QIn[cellId],
    myCellData->cellCentre[cellId],
    myCellData->cellSize[cellId],
    myCellData->t,
    myCellData->dt
  );

  int numberOfIterations = kernels::AderSolver::fusedSpaceTimePredictorVolumeIntegral<SolverPrecision, SolverPrecision, SolverPrecision>(
    repositories::instanceOfAderSolver,
    lQhbnd,
    lFhbnd,
    myCellData->QIn[cellId],
    myCellData->cellCentre[cellId],
    myCellData->cellSize[cellId],
    myCellData->t,
    myCellData->dt
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());
}


void variant2::runBenchmarks(
  int                                         numberOfCells,
  double                                      timeStamp,
  double                                      timeStepSize,
  const tarch::la::Vector<Dimensions, double> cellCenter,
  const tarch::la::Vector<Dimensions, double> cellSize
) {

  tarch::timing::Measurement timingComputeKernel;

  exahype2::CellData<SolverPrecision, SolverPrecision>     cellData(numberOfCells);
  exahype2::CellFaceData<SolverPrecision, SolverPrecision> cellFaceData(numberOfCells);

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    cellData.QIn[cellIndex] = tarch::allocateMemory<SolverPrecision>(
      NumberOfInputEntriesPerCell,
      tarch::MemoryLocation::Heap
    );
    cellData.t                        = timeStamp;
    cellData.dt                       = timeStepSize;
    cellData.QOut[cellIndex]          = nullptr;
    cellData.cellCentre[cellIndex]    = cellCenter;
    cellData.cellSize[cellIndex]      = cellSize;
    cellData.maxEigenvalue[cellIndex] = 0.0;

    initInputData(cellData.QIn[cellIndex], cellCenter, cellSize);
    std::memset(cellData.QOut[cellIndex], 0.0, NumberOfOutputEntriesPerCell * sizeof(SolverPrecision));

    for (int i = 0; i < 2 * Dimensions; i++) {
      cellFaceData.QIn[cellIndex][i] = tarch::allocateMemory<SolverPrecision>(
        2 * kernels::AderSolver::getBndFaceSize(),
        tarch::MemoryLocation::Heap
      );
      cellFaceData.QOut[cellIndex][i] = tarch::allocateMemory<SolverPrecision>(
        2 * kernels::AderSolver::getBndFluxSize(),
        tarch::MemoryLocation::Heap
      );
    }

    cellFaceData.t[cellIndex]          = timeStamp;
    cellFaceData.dt[cellIndex]         = timeStepSize;
    cellFaceData.cellCentre[cellIndex] = cellCenter;
    cellFaceData.cellSize[cellIndex]   = cellSize;

    std::memset(cellData.QOut[cellIndex], 0.0, NumberOfOutputEntriesPerCell * sizeof(SolverPrecision));
  }

  int numberOfThreads = 1;

  #if defined(SharedOMP)
  for (int threadIndex = 0; threadIndex < NumberOfLaunchingThreads.size(); threadIndex++) {
    numberOfThreads = NumberOfLaunchingThreads[threadIndex];
  #endif

    // Initial task
    for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
      initialTask(&cellData, &cellFaceData, cellIndex, cellIndex, timingComputeKernel);
    }

    timingComputeKernel.erase();
    tarch::timing::Measurement timingKernelLaunch;

    for (int sample = 0; sample <= NumberOfSamples; sample++) {

      #if defined(SharedOMP)
      #pragma omp parallel for num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {

        // Copy data from one side of each face to the other
        // Here we just copy data from one side of a given cell's face to the opposite face of the same cell
        for (int d = 0; d < Dimensions; d++) {
          // Copy inner negative face into outer positive face
          std::copy_n(
            &cellFaceData.QIn[cellIndex][d][kernels::AderSolver::getBndFaceSize()],
            kernels::AderSolver::getBndFaceSize(),
            &cellFaceData.QIn[cellIndex][d + Dimensions][kernels::AderSolver::getBndFaceSize()]
          );
          std::copy_n(
            &cellFaceData.QOut[cellIndex][d][kernels::AderSolver::getBndFluxSize()],
            kernels::AderSolver::getBndFluxSize(),
            &cellFaceData.QOut[cellIndex][d + Dimensions][kernels::AderSolver::getBndFluxSize()]
          );

          // Copy inner positive face into outer negative face
          std::copy_n(
            &cellFaceData.QIn[cellIndex][d + Dimensions][0],
            kernels::AderSolver::getBndFaceSize(),
            &cellFaceData.QIn[cellIndex][d][0]
          );
          std::copy_n(
            &cellFaceData.QOut[cellIndex][d + Dimensions][0],
            kernels::AderSolver::getBndFluxSize(),
            &cellFaceData.QOut[cellIndex][d][0]
          );
        }
      }

      tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);

      #if defined(SharedOMP)
      #pragma omp parallel for num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        cellData.maxEigenvalue[cellIndex] = 0.0;
        runKernels(&cellData, &cellFaceData, cellIndex, cellIndex, timingComputeKernel);
      }

      watchKernelLaunch.stop();
      timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());

    }

    reportRuntime("variant 2", timingComputeKernel, timingKernelLaunch, numberOfCells, numberOfThreads, _log);
    // allocateAndStoreOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
    // validateOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);

  #if defined(SharedOMP)
  }//threadIndex
  #endif

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    tarch::freeMemory(cellData.QIn[cellIndex], tarch::MemoryLocation::Heap);
    tarch::freeMemory(cellData.QOut[cellIndex], tarch::MemoryLocation::Heap);

    for (int i = 0; i < 2 * Dimensions; i++) {
      tarch::freeMemory(cellFaceData.QIn[cellIndex][i], tarch::MemoryLocation::Heap);
      tarch::freeMemory(cellFaceData.QOut[cellIndex][i], tarch::MemoryLocation::Heap);
    }
  }
}
