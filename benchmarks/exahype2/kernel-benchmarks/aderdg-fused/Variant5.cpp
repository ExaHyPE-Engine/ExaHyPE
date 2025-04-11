/**
 * This is variant 5 of the fused kernels.
 * In this variant, we will assume that faceData is constructed per FACE, and that
 * it contains data from both from sides of said face, i.e. the "left" and "right" sides.
 * We assume that this data IS NOT shared with the neighbour, i.e. there always exist two
 * instances of a faceData object between which data must be copied between timesteps.
 *
 * This means that the data in the faces must be copied from one face to the other in
 * between timesteps, but we assume that the Riemann solver cannot be performed at this time.
 * Therefore, the Riemann problem must be solved on each face twice, once per adjacent cell.
 *
 * This can be implemented as once enclave task, in which every kernel is solved in order for
 * every cell.
 *
 * Upsides:
 *    only one dependency which occurs between timesteps
 *    only one enclave task which corresponds to that of FV
 *
 * Downsides:
 *    Each Riemann solver must be performed twice, once per adjacent cell
 *    Data needs to be copied between faces of adjacent cells
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

tarch::logging::Log variant5::_log("variants5::");

using namespace benchmarks::exahype2::kernelbenchmarks;

inline void initialTask(
  exahype2::CellData<double, double>* myCellData,
  exahype2::FaceData<double, double>* myFaceData,
  const int                           cellId,
  const int                           faceId,
  tarch::timing::Measurement&         measurement
) {

  // Assume that the faces for a given cell are one after another in FaceData
  // in typical Peano order, e.g. left, lower, right, upper
  //  so faceId is the left, lower (front) face,
  //  faceId+1 is the one after this, and so on.
  double* lQhbnd[2 * Dimensions] = {
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

  double* lFhbnd[2 * Dimensions] = {
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

  int numberOfIterations = kernels::AderSolver::fusedSpaceTimePredictorVolumeIntegral<double, double, double>(
    repositories::instanceOfAderSolver,
    lQhbnd,
    lFhbnd,
    myCellData->QIn[cellId],
    myCellData->cellCentre[cellId],
    myCellData->cellSize[cellId],
    myCellData->t[cellId],
    myCellData->dt[cellId]
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());
}

inline void firstTask(
  exahype2::FaceData<double, double>* myFaceData,
  const int                           leftFaceId,
  const int                           rightFaceId,
  const int                           faceDirection,
  tarch::timing::Measurement&         measurement
) {

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  // We are assuming that there are no boundaries okay
  // In a real ExaHyPE application boundaries will exist
  // but we can still assume that the corresponding faces
  // will have received data
  kernels::AderSolver::riemannSolver<double>(
    repositories::instanceOfAderSolver,
    myFaceData->QOut[leftFaceId][1], // lFhbnd[d+Dimensions],
    myFaceData->QOut[rightFaceId][0],
    myFaceData->QIn[leftFaceId][1], // lFhbnd[d+Dimensions],
    myFaceData->QIn[rightFaceId][0],
    0.5 * (myFaceData->t[leftFaceId] + myFaceData->t[rightFaceId]),
    0.5 * (myFaceData->dt[leftFaceId] + myFaceData->dt[rightFaceId]),
    0.5 * (myFaceData->faceCentre[leftFaceId] + myFaceData->faceCentre[rightFaceId]),
    0.5 * (myFaceData->faceSize[leftFaceId] + myFaceData->faceSize[rightFaceId]),
    faceDirection
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());
}


inline void secondTask(
  exahype2::CellData<double, double>* myCellData,
  exahype2::FaceData<double, double>* myFaceData,
  const int                           cellId,
  const int                           faceId,
  tarch::timing::Measurement&         measurement
) {

  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  // Assume that the faces for a given cell are one after another
  // in typical Peano order, e.g. left, lower, right, upper
  double* lQhbnd[2 * Dimensions] = {
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

  double* lFhbnd[2 * Dimensions] = {
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

    const double inverseDxDirection = 1.0 / myCellData->cellSize[cellId][d];
    // Negative face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      myFaceData->QOut[faceId + d][1],
      direction,
      0,
      inverseDxDirection,
      myCellData->dt[cellId]
    );

    // Positive face
    kernels::AderSolver::faceIntegral(
      myCellData->QIn[cellId],
      myFaceData->QOut[faceId + Dimensions][0],
      direction,
      1,
      inverseDxDirection,
      myCellData->dt[cellId]
    );

  } // for d

  myCellData->maxEigenvalue[cellId] = kernels::AderSolver::maxScaledEigenvalue(
    repositories::instanceOfAderSolver,
    myCellData->QIn[cellId],
    myCellData->cellCentre[cellId],
    myCellData->cellSize[cellId],
    myCellData->t[cellId],
    myCellData->dt[cellId]
  );

  int numberOfIterations = kernels::AderSolver::fusedSpaceTimePredictorVolumeIntegral<double, double, double>(
    repositories::instanceOfAderSolver,
    lQhbnd,
    lFhbnd,
    myCellData->QIn[cellId],
    myCellData->cellCentre[cellId],
    myCellData->cellSize[cellId],
    myCellData->t[cellId],
    myCellData->dt[cellId]
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());
}


void variant5::runBenchmarks(
  int                                         numberOfCells,
  double                                      timeStamp,
  double                                      timeStepSize,
  const tarch::la::Vector<Dimensions, double> cellCenter,
  const tarch::la::Vector<Dimensions, double> cellSize
) {

  tarch::timing::Measurement timingComputeKernel;

  exahype2::CellData<double, double> cellData(numberOfCells);
  exahype2::FaceData<double, double> faceData(2 * Dimensions * numberOfCells);

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    cellData.QIn[cellIndex] = tarch::allocateMemory<double>(
      NumberOfInputEntriesPerCell,
      tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
    );
    cellData.t[cellIndex]             = timeStamp;
    cellData.dt[cellIndex]            = timeStepSize;
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
    faceData.QIn[faceIndex][0] = tarch::allocateMemory<double>(
      kernels::AderSolver::getBndFaceSize(),
      tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
    );
    faceData.QIn[faceIndex][1] = tarch::allocateMemory<double>(
      kernels::AderSolver::getBndFaceSize(),
      tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
    );
    faceData.QOut[faceIndex][0] = tarch::allocateMemory<double>(
      kernels::AderSolver::getBndFluxSize(),
      tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
    );
    faceData.QOut[faceIndex][1] = tarch::allocateMemory<double>(
      kernels::AderSolver::getBndFluxSize(),
      tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
    );

    faceData.t[faceIndex]          = timeStamp;
    faceData.dt[faceIndex]         = timeStepSize;
    faceData.faceCentre[faceIndex] = cellCenter;
    faceData.faceSize[faceIndex]   = cellSize;
  }

  timingComputeKernel.erase();
  tarch::timing::Measurement timingKernelLaunch;

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

  for (int sample = 0; sample <= NumberOfSamples; sample++) {
#if defined(SharedOMP)
#pragma omp parallel num_threads(NumberOfLaunchingThreads)
#endif
    for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {

      for (int d = 0; d < Dimensions; d++) {
        tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
        // Stitch negative and positive face together and just
        //  perform the Riemann kernel on that.
        firstTask(
          &faceData,
          2 * Dimensions * cellIndex + d + Dimensions,
          2 * Dimensions * cellIndex + d,
          d,
          timingComputeKernel
        );
        watchKernelLaunch.stop();
        timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
      }
    }

#if defined(SharedOMP)
#pragma omp parallel num_threads(NumberOfLaunchingThreads)
#endif
    for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
      cellData.maxEigenvalue[cellIndex] = 0.0;
      tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
      secondTask(
        &cellData,
        &faceData,
        cellIndex,
        cellIndex * 2 * Dimensions, // just passing the first of a cells faces, it can get the others by pointer
                                    // arithmetic
        timingComputeKernel
      );
      watchKernelLaunch.stop();
      timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
    }
  }


  reportRuntime("variant 5", timingComputeKernel, timingKernelLaunch, numberOfCells, _log);
  // allocateAndStoreOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
  // validateOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);

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
