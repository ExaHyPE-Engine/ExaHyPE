/**
 * This is variant 3 of the fused kernels.
 * In this variant, we will assume that faceData is constructed per cell, and that
 * it contains data both from one's own cell and one's neighbour, e.g. it has access
 * to the side of the faces that belong to the neighbour.
 * We assume that this data IS shared with the neighbour, i.e. there always exist two
 * instances of a faceData object which share a pointer.
 *
 * This means that the Riemann kernel can be performed only once per face, but it also
 * means that these need to be performed before any cells are started, as otherwise the
 * cells cannot know whether any given face has already been solved by its neighbour.
 *
 * This can be implemented as two enclave tasks, with the first being solving the
 * pre-required Riemann problems, and the second being solving the problems inside of
 * all of the cells in order.
 *
 * Upsides:
 *    only one dependency which occurs between timeSteps
 *    Riemann kernels are only performed once per face
 *    No copying or duplicating of data whatsoever
 *
 * Downsides:
 *    Costly data dependency at the beginning of the step due to the initial Riemann solves
 *
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

tarch::logging::Log variant3::_log("variants3::");

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

inline void firstTask(
  exahype2::CellFaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                               faceId,
  const int                               faceDirection,
  tarch::timing::Measurement&             measurement
) {

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  tarch::la::Vector<Dimensions, double> faceCenter = myFaceData->cellCentre[faceId];
  faceCenter[faceDirection % Dimensions] += 0.5 * myFaceData->cellSize[faceId][faceDirection % Dimensions];

  // We are assuming that there are no boundaries okay
  // In a real ExaHyPE application boundaries will exist
  // but we can still assume that the corresponding faces
  // will have received data
  kernels::AderSolver::riemannSolver<SolverPrecision>(
    repositories::instanceOfAderSolver,
    &myFaceData->QOut[faceId][faceDirection][0],
    &myFaceData->QOut[faceId][faceDirection][kernels::AderSolver::getBndFluxSize()],
    &myFaceData->QIn[faceId][faceDirection][0],
    &myFaceData->QIn[faceId][faceDirection][kernels::AderSolver::getBndFaceSize()],
    myFaceData->t[faceId],
    myFaceData->dt[faceId],
    faceCenter,
    myFaceData->cellSize[faceId],
    faceDirection
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


void variant3::runBenchmarks(
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

    for (int i = 0; i < Dimensions; i++) {
      cellFaceData.QIn[cellIndex][i] = tarch::allocateMemory<SolverPrecision>(
        2 * kernels::AderSolver::getBndFaceSize(),
        tarch::MemoryLocation::Heap
      );
      cellFaceData.QOut[cellIndex][i] = tarch::allocateMemory<SolverPrecision>(
        2 * kernels::AderSolver::getBndFluxSize(),
        tarch::MemoryLocation::Heap
      );
    }
    // Stitch faces together by connecting the left and right faces of any given cell
    for (int i = 0; i < Dimensions; i++) {
      cellFaceData.QIn[cellIndex][i + Dimensions]  = cellFaceData.QIn[cellIndex][i];
      cellFaceData.QOut[cellIndex][i + Dimensions] = cellFaceData.QOut[cellIndex][i];
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
      #pragma omp parallel num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        // We're not actually simulating a real grid, instead
        // we are making each cell it's own neighbour, as though
        // it were a whole periodic domain
        firstTask(&cellFaceData, cellIndex, 0, timingComputeKernel);
        firstTask(&cellFaceData, cellIndex, 1, timingComputeKernel);
      }
      #if defined(SharedOMP)
      #pragma omp parallel num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        cellData.maxEigenvalue[cellIndex] = 0.0;
        secondTask(&cellData, &cellFaceData, cellIndex, cellIndex, timingComputeKernel);
      }

      watchKernelLaunch.stop();
      timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
    }


    reportRuntime("variant 3", timingComputeKernel, timingKernelLaunch, numberOfCells, numberOfThreads, _log);
    // allocateAndStoreOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
    // validateOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);

  #if defined(SharedOMP)
  }//threadIndex
  #endif

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    tarch::freeMemory(cellData.QIn[cellIndex], tarch::MemoryLocation::Heap);
    tarch::freeMemory(cellData.QOut[cellIndex], tarch::MemoryLocation::Heap);

    for (int i = 0; i < Dimensions; i++) {
      tarch::freeMemory(cellFaceData.QIn[cellIndex][i], tarch::MemoryLocation::Heap);
      tarch::freeMemory(cellFaceData.QOut[cellIndex][i], tarch::MemoryLocation::Heap);
    }
  }
}
