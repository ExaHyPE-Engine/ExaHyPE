/**
 * This is variant 1 of the fused kernels.
 * In this variant, we will assume that faceData is constructed per cell, and that
 * it only contains the data from one's own cell, e.g. it cannot access the side
 * of the faces that belong to the neighbour.
 *
 * This means that the Riemann kernel cannot be performed within a given cell, but must
 * be performed outside of the cells.
 *
 * Each cell therefore only perform the "cell" kernels and "face" kernels are expected
 * to be performed before any of the cell kernels are accessed.
 *
 * This can be implemented efficiently as two enclave tasks, with one being dependent
 * on the other, but requires a lot of complicated and fiddly management of faces to
 * ensure that the Riemann kernels are correctly performed on each neighbouring pair
 * of faces, which is very fiddly and uncomfortable.
 *
 * Upsides:
 *    only requires each Riemann Kernel to be performed once
 *    does not require any copying of data whatsoever
 *    only one dependency between two groups of tasks
 *
 * Downsides:
 *    very fiddly and bug-prone management of faces
 *    task dependency is within a timeStep instead of between timeSteps
 */

#include "CellFaceData.h"
#include "exahype2/CellData.h"
#include "kernels/AderSolver/BufferSizes.h"
#include "kernels/AderSolver/FaceIntegral.h"
#include "kernels/AderSolver/FusedSpaceTimePredictorVolumeIntegral.h"
#include "kernels/AderSolver/MaxScaledEigenvalue.h"
#include "kernels/AderSolver/RiemannSolver.h"
#include "repositories/SolverRepository.h"
#include "Utils.h"
#include "Variants.h"

#include <cstring>

tarch::logging::Log variant1::_log("variants1::");

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
    myFaceData->QIn[faceId][0],
    myFaceData->QIn[faceId][1],
#if Dimensions == 3
    myFaceData->QIn[faceId][2],
#endif
    myFaceData->QIn[faceId][Dimensions + 0],
    myFaceData->QIn[faceId][Dimensions + 1]
#if Dimensions == 3
    ,
    myFaceData->QIn[faceId][Dimensions + 2]
#endif
  };

  SolverPrecision* lFhbnd[2 * Dimensions] = {
    myFaceData->QOut[faceId][0],
    myFaceData->QOut[faceId][1],
#if Dimensions == 3
    myFaceData->QOut[faceId][2],
#endif
    myFaceData->QOut[faceId][Dimensions + 0],
    myFaceData->QOut[faceId][Dimensions + 1]
#if Dimensions == 3
    ,
    myFaceData->QOut[faceId][Dimensions + 2]
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

inline void firstTask(
  exahype2::CellFaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                               leftFaceId,
  const int                               rightFaceId,
  const int                               faceDirection,
  tarch::timing::Measurement&             measurement
) {

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  // We are assuming that there are no boundaries okay
  // In a real ExaHyPE application boundaries will exist
  // but we can still assume that the corresponding faces
  // will have received data
  kernels::AderSolver::riemannSolver<SolverPrecision>(
    repositories::instanceOfAderSolver,
    myFaceData->QOut[leftFaceId][faceDirection + Dimensions], // lFhbnd[d+Dimensions],
    myFaceData->QOut[rightFaceId][faceDirection],
    myFaceData->QIn[leftFaceId][faceDirection + Dimensions], // lFhbnd[d+Dimensions],
    myFaceData->QIn[rightFaceId][faceDirection],
    0.5 * (myFaceData->t[leftFaceId] + myFaceData->t[rightFaceId]),
    0.5 * (myFaceData->dt[leftFaceId] + myFaceData->dt[rightFaceId]),
    0.5 * (myFaceData->cellCentre[leftFaceId] + myFaceData->cellCentre[rightFaceId]),
    0.5 * (myFaceData->cellSize[leftFaceId] + myFaceData->cellSize[rightFaceId]),
    faceDirection,
    false,
    0
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());
}

inline void secondTask(
  exahype2::CellData<SolverPrecision, SolverPrecision>*     myCellData,
  exahype2::CellFaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                               cellId,
  const int                               faceId,
  tarch::timing::Measurement&             measurement
) {

  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  SolverPrecision* lQhbnd[2 * Dimensions] = {
    myFaceData->QIn[faceId][0],
    myFaceData->QIn[faceId][1],
#if Dimensions == 3
    myFaceData->QIn[faceId][2],
#endif
    myFaceData->QIn[faceId][Dimensions + 0],
    myFaceData->QIn[faceId][Dimensions + 1]
#if Dimensions == 3
    ,
    myFaceData->QIn[faceId][Dimensions + 2]
#endif
  };

  SolverPrecision* lFhbnd[2 * Dimensions] = {
    myFaceData->QOut[faceId][0],
    myFaceData->QOut[faceId][1],
#if Dimensions == 3
    myFaceData->QOut[faceId][2],
#endif
    myFaceData->QOut[faceId][Dimensions + 0],
    myFaceData->QOut[faceId][Dimensions + 1]
#if Dimensions == 3
    ,
    myFaceData->QOut[faceId][Dimensions + 2]
#endif
  };

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  for (int d = 0; d < Dimensions; d++) {
    const int direction = d;

    const double inverseDxDirection = 1.0 / myCellData->cellSize[cellId][d];
    // Negative face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      myFaceData->QOut[faceId][d],
      direction,
      0,
      inverseDxDirection,
      myCellData->dt
    );

    // Positive face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      myFaceData->QOut[faceId][d + Dimensions],
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


void variant1::runBenchmarks(
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
    cellData.t               = timeStamp;
    cellData.dt              = timeStepSize;
    cellData.QOut[cellIndex] = nullptr; // tarch::allocateMemory<SolverPrecision>(NumberOfOutputEntriesPerCell,
                                        // tarch::MemoryLocation::Heap);
    cellData.cellCentre[cellIndex]    = cellCenter;
    cellData.cellSize[cellIndex]      = cellSize;
    cellData.maxEigenvalue[cellIndex] = 0.0;

    initInputData(cellData.QIn[cellIndex], cellCenter, cellSize);
    std::memset(cellData.QOut[cellIndex], 0.0, NumberOfOutputEntriesPerCell * sizeof(SolverPrecision));

    for (int i = 0; i < 2 * Dimensions; i++) {
      cellFaceData.QIn[cellIndex][i] = tarch::allocateMemory<SolverPrecision>(
        kernels::AderSolver::getBndFaceSize(),
        tarch::MemoryLocation::Heap
      );
      cellFaceData.QOut[cellIndex][i] = tarch::allocateMemory<SolverPrecision>(
        kernels::AderSolver::getBndFluxSize(),
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

      tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);

      #if defined(SharedOMP)
      #pragma omp parallel for num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        // We're not actually simulating a real grid, instead
        // we are making each cell it's own neighbour, as though
        // it were a whole periodic domain
        firstTask(&cellFaceData, cellIndex, cellIndex, 0, timingComputeKernel);
        firstTask(&cellFaceData, cellIndex, cellIndex, 1, timingComputeKernel);
      }
      #if defined(SharedOMP)
      #pragma omp parallel for num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        cellData.maxEigenvalue[cellIndex] = 0.0;
        secondTask(&cellData, &cellFaceData, cellIndex, cellIndex, timingComputeKernel);
      }

      watchKernelLaunch.stop();
      timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
    }    

    reportRuntime("variant 1", timingComputeKernel, timingKernelLaunch, numberOfCells, numberOfThreads, _log);
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
