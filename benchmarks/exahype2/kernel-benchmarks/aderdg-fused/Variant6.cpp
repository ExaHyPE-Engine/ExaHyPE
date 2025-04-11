/**
 * This is variant 6 of the fused kernels.
 * In this variant, we will assume that faceData is constructed per FACE, and that
 * it contains data from both from sides of said face, i.e. the "left" and "right" sides.
 * We assume that this data IS shared with the neighbour, i.e. there always exist two
 * instances of a faceData object which share a pointer.
 *
 * This means that the Riemann kernel can be performed only once per face, but it also
 * means that we need to be careful about the order in which the cells are computed, as
 * we have to assume that certain Riemann solvers have already been computed.
 * The simplest solution to this is to assume a grid of cells, precompute the topmost,
 * leftmost (,frontmost if 3D) Riemann solvers.
 * After this we can iterate over the grid front-to-back in all three dimensions, first
 * solving the Riemann problems on the negative side of the current cell while assuming
 * that the problems on the positive side of the cell have already been solved.
 *
 * This can be implemented as two enclave tasks, with the first being solving the
 * pre-required Riemann problems, and the second being solving the problems inside of
 * all of the cells.
 *
 * Upsides:
 *    only one dependency which occurs between timesteps
 *    Riemann kernels are only performed once per face
 *    No copying or duplicating of data whatsoever
 *
 * Downsides:
 *    Cells cannot be performed independently, they require specific orders
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

tarch::logging::Log variant6::_log("variants6::");

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
  const int                           faceId,
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
    myFaceData->QOut[faceId][1],
    myFaceData->QOut[faceId][0],
    myFaceData->QIn[faceId][1],
    myFaceData->QIn[faceId][0],
    myFaceData->t[faceId],
    myFaceData->dt[faceId],
    myFaceData->faceCentre[faceId],
    myFaceData->faceSize[faceId],
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


void variant6::runBenchmarks(
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

  /**
   * The construction here is a bit peculiar.
   * Since we want to simulate there being 2*Dimensions*numberOfCells faces, but each should share a pointer with
   * one neighbouring cells' face Pointer, we have each face allocate the inner part of its data, i.e. the right side
   * on left faces and the left side on right faces.
   *
   * Then, in a second step, we have the non-allocated side point to the allocated side of the other face.
   * This results in face having two valid pointers, but the total amount of allocated data fitting the model.
   *
   * We also could have allocated all data on the lefts and have all the rights point, but I was slightly less confident
   * about this being a good model for the actual data movements.
   */
  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    const int faceIndex = 2 * Dimensions * cellIndex;
    for (int d = 0; d < Dimensions; d++) {
      // Allocating data for the right side of the left face and the left side of the right face
      faceData.QIn[faceIndex + d][1] = tarch::allocateMemory<double>(
        kernels::AderSolver::getBndFaceSize(),
        tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
      );
      faceData.QIn[faceIndex + d + Dimensions][0] = tarch::allocateMemory<double>(
        kernels::AderSolver::getBndFaceSize(),
        tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
      );

      faceData.QOut[faceIndex + d][1] = tarch::allocateMemory<double>(
        kernels::AderSolver::getBndFluxSize(),
        tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
      );
      faceData.QOut[faceIndex + d + Dimensions][0] = tarch::allocateMemory<double>(
        kernels::AderSolver::getBndFluxSize(),
        tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
      );

      // Stitching these faces together
      faceData.QIn[faceIndex + d][0]              = faceData.QIn[faceIndex + d + Dimensions][0];
      faceData.QIn[faceIndex + d + Dimensions][1] = faceData.QIn[faceIndex + d][1];

      faceData.QOut[faceIndex + d][0]              = faceData.QOut[faceIndex + d + Dimensions][0];
      faceData.QOut[faceIndex + d + Dimensions][1] = faceData.QOut[faceIndex + d][1];

      faceData.t[faceIndex + d]          = timeStamp;
      faceData.dt[faceIndex + d]         = timeStepSize;
      faceData.faceCentre[faceIndex + d] = cellCenter;
      faceData.faceSize[faceIndex + d]   = cellSize;

      faceData.t[faceIndex + d + Dimensions]          = timeStamp;
      faceData.dt[faceIndex + d + Dimensions]         = timeStepSize;
      faceData.faceCentre[faceIndex + d + Dimensions] = cellCenter;
      faceData.faceSize[faceIndex + d + Dimensions]   = cellSize;
    }
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
        // We have stitched the negative and positive faces together and therefore
        // only need to perform the Riemann kernel on one of them, as the other points to the same data.
        firstTask(&faceData, 2 * Dimensions * cellIndex + d, d, timingComputeKernel);
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


  reportRuntime("variant 6", timingComputeKernel, timingKernelLaunch, numberOfCells, _log);
  // allocateAndStoreOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
  // validateOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    tarch::freeMemory(cellData.QIn[cellIndex], tarch::MemoryLocation::Heap);
    tarch::freeMemory(cellData.QOut[cellIndex], tarch::MemoryLocation::Heap);

    const int faceIndex = 2 * Dimensions * cellIndex;
    for (int d = 0; d < Dimensions; d++) {
      // Freeing data for the right side of the left face and the left side of the right face
      tarch::freeMemory(faceData.QIn[faceIndex + d][1], tarch::MemoryLocation::Heap);
      tarch::freeMemory(faceData.QIn[faceIndex + d + Dimensions][0], tarch::MemoryLocation::Heap);
      tarch::freeMemory(faceData.QOut[faceIndex + d][1], tarch::MemoryLocation::Heap);
      tarch::freeMemory(faceData.QOut[faceIndex + d + Dimensions][0], tarch::MemoryLocation::Heap);
    }
  }
}
