/**
 * This is variant 4 of the fused kernels.
 * In this variant, we will assume that faceData is constructed per FACE, and that
 * it contains data from both from sides of said face, i.e. the "left" and "right" sides.
 * We assume that this data is NOT shared with the neighbour, i.e. the faceData exists
 * once per Cell and data must be copied between two instances of the same face between
 * timeSteps.
 *
 * This means that the data in the faces must be copied from one face to the other in
 * between timeSteps, at which point the Riemann kernels can be performed as well.
 *
 * This can be implemented as two enclave tasks, with the first consisting of exchanging
 * face data and solving the pre-required Riemann problems, and the second being solving
 * the problems inside of all of the cells.
 *
 * Upsides:
 *    only requires each Riemann Kernel to be performed once
 *    only one dependency between two groups of tasks
 *
 * Downsides:
 *    Requires copying of data between faces
 *    Costly data dependency at the beginning of the step due to the initial Riemann solves
 */

#include "exahype2/CellData.h"
#include "kernels/AderSolver/CellData.h"
#include "kernels/AderSolver/FaceIntegral.h"
#include "kernels/AderSolver/FusedSpaceTimePredictorVolumeIntegral.h"
#include "kernels/AderSolver/MaxScaledEigenvalue.h"
#include "kernels/AderSolver/RiemannSolver.h"
#include "LRFaceData.h"
#include "repositories/SolverRepository.h"
#include "Utils.h"
#include "Variants.h"

#include <cstring>

tarch::logging::Log variant4::_log("variants4::");

using namespace benchmarks::exahype2::kernelbenchmarks;

inline void initialTask(
  exahype2::CellData<SolverPrecision, SolverPrecision>* myCellData,
  exahype2::FaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                           cellId,
  const int                           faceId,
  tarch::timing::Measurement&         measurement
) {

  // Assume that the faces for a given cell are one after another in FaceData
  // in typical Peano order, e.g. left, lower, right, upper
  //  so faceId is the left, lower (front) face,
  //  faceId+1 is the one after this, and so on.
  SolverPrecision* lQhbnd[2 * Dimensions] = {
    myFaceData->QIn[faceId + 0][1],
    myFaceData->QIn[faceId + 1][1],
#if Dimensions == 3
    myFaceData->QIn[faceId + 2][1],
#endif
    myFaceData->QIn[faceId + Dimensions + 0][0],
    myFaceData->QIn[faceId + Dimensions + 1][0]
#if Dimensions == 3
    ,
    myFaceData->QIn[faceId + Dimensions + 2][0]
#endif
  };

  SolverPrecision* lFhbnd[2 * Dimensions] = {
    myFaceData->QOut[faceId + 0][1],
    myFaceData->QOut[faceId + 1][1],
#if Dimensions == 3
    myFaceData->QOut[faceId + 2][1],
#endif
    myFaceData->QOut[faceId + Dimensions + 0][0],
    myFaceData->QOut[faceId + Dimensions + 1][0]
#if Dimensions == 3
    ,
    myFaceData->QOut[faceId + Dimensions + 2][0]
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
  exahype2::CellData<SolverPrecision, SolverPrecision>* myCellData,
  exahype2::FaceData<SolverPrecision, SolverPrecision>* myFaceData,
  const int                           cellId,
  const int                           faceId,
  tarch::timing::Measurement&         measurement
) {

  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  SolverPrecision* lQhbnd[2 * Dimensions] = {
    myFaceData->QIn[faceId + 0][1],
    myFaceData->QIn[faceId + 1][1],
#if Dimensions == 3
    myFaceData->QIn[faceId + 2][1],
#endif
    myFaceData->QIn[faceId + Dimensions + 0][0],
    myFaceData->QIn[faceId + Dimensions + 1][0]
#if Dimensions == 3
    ,
    myFaceData->QIn[faceId + Dimensions + 2][0]
#endif
  };

  SolverPrecision* lFhbnd[2 * Dimensions] = {
    myFaceData->QOut[faceId + 0][1],
    myFaceData->QOut[faceId + 1][1],
#if Dimensions == 3
    myFaceData->QOut[faceId + 2][1],
#endif
    myFaceData->QOut[faceId + Dimensions + 0][0],
    myFaceData->QOut[faceId + Dimensions + 1][0]
#if Dimensions == 3
    ,
    myFaceData->QOut[faceId + Dimensions + 2][0]
#endif
  };

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  for (int d = 0; d < Dimensions; d++) {
    const int direction = d;

    kernels::AderSolver::riemannSolver<SolverPrecision>(
      repositories::instanceOfAderSolver,
      myFaceData->QOut[faceId + d][0],
      myFaceData->QOut[faceId + d][1],
      myFaceData->QIn[faceId + d][0],
      myFaceData->QIn[faceId + d][1],
      myFaceData->t[faceId + d],
      myFaceData->dt[faceId + d],
      myFaceData->faceCentre[faceId + d],
      myFaceData->faceSize[faceId + d],
      direction
    );

    const double inverseDxDirection = 1.0 / myCellData->cellSize[cellId][d];
    // Negative face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      myFaceData->QOut[faceId + d][1],
      direction,
      0,
      inverseDxDirection,
      myCellData->dt
    );

    kernels::AderSolver::riemannSolver<SolverPrecision>(
      repositories::instanceOfAderSolver,
      myFaceData->QOut[faceId + d + Dimensions][0],
      myFaceData->QOut[faceId + d + Dimensions][1],
      myFaceData->QIn[faceId + d + Dimensions][0],
      myFaceData->QIn[faceId + d + Dimensions][1],
      myFaceData->t[faceId + d + Dimensions],
      myFaceData->dt[faceId + d + Dimensions],
      myFaceData->faceCentre[faceId + d + Dimensions],
      myFaceData->faceSize[faceId + d + Dimensions],
      direction
    );

    // Positive face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      myFaceData->QOut[faceId + Dimensions][0],
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


void variant4::runBenchmarks(
  int                                         numberOfCells,
  double                                      timeStamp,
  double                                      timeStepSize,
  const tarch::la::Vector<Dimensions, double> cellCenter,
  const tarch::la::Vector<Dimensions, double> cellSize
) {

  tarch::timing::Measurement timingComputeKernel;

  exahype2::CellData<SolverPrecision, SolverPrecision> cellData(numberOfCells);
  exahype2::FaceData<SolverPrecision, SolverPrecision> faceData(2 * Dimensions * numberOfCells);

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
  }

  // We need one instance of face per face of a cell, e.g.
  //  2*Dimensions*numberOfCells faces.
  //  Here we just number them in the Peano order for each cell,
  //  so face (2*Dimensions*i+j) will be the jth face of the ith cell
  for (int faceIndex = 0; faceIndex < 2 * Dimensions * numberOfCells; faceIndex++) {
    // Allocating data for the left and right sides of a given face
    faceData.QIn[faceIndex][0] = tarch::allocateMemory<SolverPrecision>(
      kernels::AderSolver::getBndFaceSize(),
      tarch::MemoryLocation::Heap
    );
    faceData.QIn[faceIndex][1] = tarch::allocateMemory<SolverPrecision>(
      kernels::AderSolver::getBndFaceSize(),
      tarch::MemoryLocation::Heap
    );
    faceData.QOut[faceIndex][0] = tarch::allocateMemory<SolverPrecision>(
      kernels::AderSolver::getBndFluxSize(),
      tarch::MemoryLocation::Heap
    );
    faceData.QOut[faceIndex][1] = tarch::allocateMemory<SolverPrecision>(
      kernels::AderSolver::getBndFluxSize(),
      tarch::MemoryLocation::Heap
    );

    faceData.t[faceIndex]          = timeStamp;
    faceData.dt[faceIndex]         = timeStepSize;
    faceData.faceCentre[faceIndex] = cellCenter;
    faceData.faceSize[faceIndex]   = cellSize;
  }

  int numberOfThreads = 1;

  #if defined(SharedOMP)
  for (int threadIndex = 0; threadIndex < NumberOfLaunchingThreads.size(); threadIndex++) {
    numberOfThreads = NumberOfLaunchingThreads[threadIndex];
  #endif

    // Initial task
    for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
      initialTask(
        &cellData,
        &faceData,
        cellIndex,
        cellIndex * 2 * Dimensions, // just passing the first of a cells faces, it can get the others by pointer
                                    // arithmetic
        timingComputeKernel
      );
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
            faceData.QIn[2 * Dimensions * cellIndex + d][1],
            kernels::AderSolver::getBndFaceSize(),
            faceData.QIn[2 * Dimensions * cellIndex + d + Dimensions][1]
          );
          std::copy_n(
            faceData.QOut[2 * Dimensions * cellIndex + d][1],
            kernels::AderSolver::getBndFluxSize(),
            faceData.QOut[2 * Dimensions * cellIndex + d + Dimensions][1]
          );

          // Copy inner positive face into outer negative face
          std::copy_n(
            faceData.QIn[2 * Dimensions * cellIndex + d + Dimensions][0],
            kernels::AderSolver::getBndFaceSize(),
            faceData.QIn[2 * Dimensions * cellIndex + d][0]
          );
          std::copy_n(
            faceData.QOut[2 * Dimensions * cellIndex + d + Dimensions][0],
            kernels::AderSolver::getBndFluxSize(),
            faceData.QOut[2 * Dimensions * cellIndex + d][0]
          );
        }
      }

      tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);

      #if defined(SharedOMP)
      #pragma omp parallel for num_threads(NumberOfLaunchingThreads[threadIndex])
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        cellData.maxEigenvalue[cellIndex] = 0.0;
        runKernels(
          &cellData,
          &faceData,
          cellIndex,
          cellIndex * 2 * Dimensions, // just passing the first of a cells faces, it can get the others by pointer
                                      // arithmetic
          timingComputeKernel
        );
      }

      watchKernelLaunch.stop();
      timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
    }


    reportRuntime("variant 4", timingComputeKernel, timingKernelLaunch, numberOfCells, numberOfThreads, _log);
    // allocateAndStoreOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
    // validateOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);

  #if defined(SharedOMP)
  }//threadIndex
  #endif

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    tarch::freeMemory(cellData.QIn[cellIndex], tarch::MemoryLocation::Heap);
    tarch::freeMemory(cellData.QOut[cellIndex], tarch::MemoryLocation::Heap);
  }
  for (int faceIndex = 0; faceIndex < 2 * Dimensions * numberOfCells; faceIndex++) {
    tarch::freeMemory(faceData.QIn[faceIndex][0], tarch::MemoryLocation::Heap);
    tarch::freeMemory(faceData.QIn[faceIndex][1], tarch::MemoryLocation::Heap);
    tarch::freeMemory(faceData.QOut[faceIndex][0], tarch::MemoryLocation::Heap);
    tarch::freeMemory(faceData.QOut[faceIndex][1], tarch::MemoryLocation::Heap);
  }
}
