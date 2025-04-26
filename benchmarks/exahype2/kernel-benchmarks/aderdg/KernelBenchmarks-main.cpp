// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarks-main.h"

#include "config.h"
#include "Constants.h"

#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"

#include "exahype2/CellData.h"

#include "exahype2/dg/DGUtils.h"

//#include "tasks/FVRusanovEnclaveTask.h"
#include "kernels/AderSolver/FusedSpaceTimePredictorVolumeIntegral.h"
#include "kernels/AderSolver/FaceIntegral.h"
#include "kernels/AderSolver/MaxScaledEigenvalue.h"
#include "kernels/AderSolver/Quadrature.h"
#include "kernels/AderSolver/DGMatrices.h"
#include "kernels/AderSolver/RiemannSolver.h"
#include "kernels/AderSolver/CellData.h"

#include "peano4/peano4.h"

#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/logging/Log.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

#include <cstring>

#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON

using namespace benchmarks::exahype2::kernelbenchmarks;

tarch::logging::Log _log("::");

constexpr double TimeStamp    = 0.0;
constexpr double TimeStepSize = 1e-6;
const tarch::la::Vector<Dimensions,double> CellCenter = benchmarks::exahype2::kernelbenchmarks::DomainOffset + 0.5*benchmarks::exahype2::kernelbenchmarks::DomainSize;
const tarch::la::Vector<Dimensions,double> CellSize = benchmarks::exahype2::kernelbenchmarks::DomainSize / 81.0;
// constexpr int    HaloSize     = 1;

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

constexpr int NumberOfInputEntriesPerCell
  = (AderSolver::Order + 1)
  * (AderSolver::Order + 1)
#if Dimensions == 3
  * (AderSolver::Order + 1)
#endif
  * (AderSolver::NumberOfUnknowns + AderSolver::NumberOfAuxiliaryVariables);

constexpr int NumberOfOutputEntriesPerCell
  = 0;
//   = AderSolver::Order
//   * AderSolver::Order
// #if Dimensions == 3
//   * AderSolver::Order
// #endif
//   * (AderSolver::NumberOfUnknowns + AderSolver::NumberOfAuxiliaryVariables);

constexpr int NumberOfFiniteVolumesPerCell
  = AderSolver::Order
  * AderSolver::Order
#if Dimensions == 3
  * AderSolver::Order
#endif
  ;

// Check the outcomes of each kernel
double** validQ = nullptr;
double* validMaxEigenvalue = nullptr;
bool outcomeIsInvalid = false;

tarch::timing::Measurement timingComputeKernel;

/**
 * Set input data
 *
 * We use the values from the centralMost Cell in the domain
 * with an arbitrary cell size of 1/81 of the domain size.
 * 
 * TODO: should we update this to actual initial conditions?
 */
void initInputData(double* Q) {
  // for (int i = 0; i < NumberOfInputEntriesPerCell; i++) {
  //   Q[i] = std::sin(1.0 * i / (NumberOfInputEntriesPerCell) * tarch::la::PI);
  // }

  int linearisedIndex = 0; 
  dfor(index, AderSolver::Order + 1) {
    repositories::instanceOfAderSolver.initialCondition(
      Q + linearisedIndex,
      ::exahype2::dg::getQuadraturePoint(
        CellCenter,
        CellSize,
        index,
        repositories::instanceOfAderSolver.Order + 1,
        kernels::AderSolver::Quadrature<double>::nodes
      ),
      CellSize,
      //index,
      true
    );
    linearisedIndex += AderSolver::NumberOfUnknowns + AderSolver::NumberOfAuxiliaryVariables;
  }

}

/**
 * Allocates and stores outcome of one compute kernel
 *
 * Make a persistent snapshot of a solution and assume, from hereon, that
 * this snapshot is the valid data. You can call this routine as often as
 * you want. Only the very first call will trigger a snapshot.
 *
 * Note: needs to freed using the freeOutcome() routine.
 */
void allocateAndStoreOutcome(const double* const* Q,
                             const double* const  maxEigenvalue,
                             const int            numberOfCells
) {
  if constexpr (Accuracy <= 0.0) return;
  if (validQ == nullptr and validMaxEigenvalue == nullptr) {
    validQ = new double*[numberOfCells];
    for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
      validQ[cellIndex] = new double[NumberOfOutputEntriesPerCell];
      std::memcpy(validQ[cellIndex], Q[cellIndex], sizeof(double) * NumberOfOutputEntriesPerCell);
    }
    validMaxEigenvalue = new double[numberOfCells];
    std::memcpy(validMaxEigenvalue, maxEigenvalue, sizeof(double) * numberOfCells);
    logInfo("storeOutcome(...)", "bookmarked reference solution");
  }
}

void freeOutcome(const int numberOfCells) {
  if constexpr (Accuracy <= 0.0) return;
  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    delete[] validQ[cellIndex];
  }
  delete[] validQ;
  delete[] validMaxEigenvalue;
  validQ = nullptr;
  validMaxEigenvalue = nullptr;
}

/**
 * Validate data against pre-stored simulation outcome
 *
 * Works if and only if storeOutcome has been performed before.
 * Does not abort in case there are errors.
 * If there are errors, we return a status code != 0.
 *
 */
void validateOutcome(
  const double* const* Q,
  const double* const  maxEigenvalue,
  const int            numberOfCells
) {
  if constexpr (Accuracy <= 0.0) return;
  int errors = 0;
  double maxDifference = 0.0;

  std::cerr.precision(16);
  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    for (int i = 0; i < NumberOfOutputEntriesPerCell; i++) {
      if (not tarch::la::equals(Q[cellIndex][i], validQ[cellIndex][i], Accuracy)) {
        if (!errors) { // Only print once
          logError("validateOutcome(...)",
            std::fixed
              << "cell " << cellIndex << ": "
              << "Q[" << i << "]!=validQ[" << i << "] ("
              << Q[cellIndex][i]
              << "!="
              << validQ[cellIndex][i]
              << ")"
          );
        }
        errors++;
        maxDifference = std::max(maxDifference, std::abs(Q[cellIndex][i] - validQ[cellIndex][i]));
    }
  }

    if (not tarch::la::equals(maxEigenvalue[cellIndex], validMaxEigenvalue[cellIndex], Accuracy)) {
      if (!errors) {
        logError("validateOutcome(...)",
          std::fixed
            << "maxEigenvalue[" << cellIndex << "]!=validMaxEigenvalue[" << cellIndex << "] ("
            << maxEigenvalue[cellIndex] << "!=" << validMaxEigenvalue[cellIndex]
            << ")";
        );
      }
      errors++;
      maxDifference = std::max(maxDifference, std::abs(maxEigenvalue[cellIndex] - validMaxEigenvalue[cellIndex]));
    }
  }

  if (errors > 0) {
    outcomeIsInvalid = true;
    logError("validateOutcome(...)",
      "max difference of outcome from all cells is "
      << maxDifference
      << " (admissible accuracy="
      << Accuracy << ")"
      << " for " << errors << " entries"
    );
  }
}

/**
 * Reports the runtime and throughput of the benchmarks
 *
 * First set of outputs refers to the time of the compute kernel itself.
 * Second set of outputs refers to the time including the kernel launch overhead.
 * The throughput is measured in the updates of volumes per second (dof/s).
 *
 */
void reportRuntime(
  const std::string&                  kernelIdentificator,
  const tarch::timing::Measurement&   timingKernelLaunch,
  int                                 numberOfCells
) {
  std::stringstream ss;
  ss << "\n";
  ss << kernelIdentificator << ":\n\t";
  ss << timingComputeKernel.getValue() << " |\n\t";
  ss << (timingComputeKernel.getValue() / numberOfCells ) << " |\n\t";
  ss << timingComputeKernel.toString() << " |\n\t";
  ss << timingKernelLaunch.getValue() << " |\n\t";
  ss << (timingKernelLaunch.getValue() / numberOfCells );
  ss << " |\n\t" << timingKernelLaunch.toString();
  logInfo("reportRuntime()", ss.str());
}

/*
 * Executes one full run of all of the kernels
*/
double runKernels(int device, exahype2::CellData<double, double>& cellData, int cellIndex, tarch::timing::Measurement& measurement){
  double boundaryData[kernels::AderSolver::getBndFaceTotalSize()];
  double* lQhbnd[2*Dimensions] = {
    boundaryData,
    boundaryData  +   kernels::AderSolver::getBndFaceSize(),
    boundaryData  + 2*kernels::AderSolver::getBndFaceSize(),
    boundaryData  + 3*kernels::AderSolver::getBndFaceSize()
#if Dimensions==3
    ,boundaryData + 4*kernels::AderSolver::getBndFaceSize(),
     boundaryData + 5*kernels::AderSolver::getBndFaceSize()
#endif
  };

  double boundaryFlux[kernels::AderSolver::getBndFluxTotalSize()];
  double* lFhbnd[2*Dimensions] = {
      boundaryFlux,
      boundaryFlux  +   kernels::AderSolver::getBndFluxSize(),
      boundaryFlux  + 2*kernels::AderSolver::getBndFluxSize(),
      boundaryFlux  + 3*kernels::AderSolver::getBndFluxSize()
#if Dimensions==3
      ,boundaryFlux + 4*kernels::AderSolver::getBndFluxSize(),
       boundaryFlux + 5*kernels::AderSolver::getBndFluxSize()
#endif
  };

  tarch::timing::Watch watchKernelCompute("::runBenchmarks", "assessKernel(...)", false);

  int numberOfIterations = kernels::AderSolver::fusedSpaceTimePredictorVolumeIntegral<double, double, double>(
    repositories::instanceOfAderSolver,
    lQhbnd, lFhbnd, cellData.QIn[cellIndex],
    CellCenter, CellSize, TimeStamp, TimeStepSize
  );

  for (int d = 0; d < Dimensions; d++) {
    const int direction = d;

    //For the Riemann solver, we wrap the domain as though it had periodic boundary
    // conditions and solve the problem between the left and right faces of the cell.
    tarch::la::Vector<Dimensions, double> faceCentre = CellCenter;
//    faceCentre[d] -= 0.5 * CellSize[d];

    kernels::AderSolver::riemannSolver<double>(
      repositories::instanceOfAderSolver,
      lFhbnd[d+Dimensions],
      lFhbnd[d],
      lQhbnd[d+Dimensions],
      lQhbnd[d],
      TimeStamp,
      TimeStepSize,
      faceCentre,
      CellSize,
      direction
    );

    const double inverseDxDirection = 1.0 / CellSize[d];
    // Negative face
    kernels::AderSolver::faceIntegral(
      cellData.QIn[cellIndex],
      lFhbnd[d],
      direction,
      0,
      inverseDxDirection,
      TimeStepSize
    );

    // Positive face
    faceCentre[d] += CellSize[d];
    kernels::AderSolver::faceIntegral(
      cellData.QIn[cellIndex],
      lFhbnd[d+Dimensions],
      direction,
      1,
      inverseDxDirection,
      TimeStepSize
    );

  } // for d

  double maxEigenvalue = kernels::AderSolver::maxScaledEigenvalue(
    repositories::instanceOfAderSolver,
    cellData.QIn[cellIndex],
    CellCenter,
    CellSize,
    TimeStamp,
    TimeStepSize
  );

  watchKernelCompute.stop();
  measurement.setValue(watchKernelCompute.getCalendarTime());

  return maxEigenvalue;

}

/**
 * Run the benchmark for one particular number of cells
 *
 * @param numberOfCells Number of cells to study
 */
void runBenchmarks(int numberOfCells) {
  exahype2::CellData<double, double> cellData(numberOfCells);
  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    cellData.QIn[cellIndex]           = tarch::allocateMemory<double>(NumberOfInputEntriesPerCell, tarch::MemoryLocation::Heap);
    cellData.QOut[cellIndex]          = nullptr; //tarch::allocateMemory<double>(NumberOfOutputEntriesPerCell, tarch::MemoryLocation::Heap);
    cellData.cellCentre[cellIndex]    = CellCenter;
    cellData.cellSize[cellIndex]      = CellSize;
    cellData.maxEigenvalue[cellIndex] = 0.0;
    initInputData(cellData.QIn[cellIndex]);
    std::memset(cellData.QOut[cellIndex], 0.0, NumberOfOutputEntriesPerCell * sizeof(double));
  }

  cellData.t = TimeStamp;
  cellData.dt = TimeStepSize;

  auto assessKernel = [&](
    std::function<double(int device, exahype2::CellData<double, double>& cellData, int cellIndex, tarch::timing::Measurement& measurement)> executeKernels,
    const std::string& markerName,
    const int          device
  ) -> void {
    timingComputeKernel.erase();
    tarch::timing::Measurement timingKernelLaunch;

    int sample = 0;
    while (sample <= NumberOfSamples) {
      // Reset output data
      #if defined(SharedOMP)
      #pragma omp parallel num_threads(NumberOfLaunchingThreads)
      #endif
      for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
        cellData.maxEigenvalue[cellIndex] = 0.0;
        // std::memset(cellData.QOut[cellIndex], 0.0, NumberOfOutputEntriesPerCell * sizeof(double));

      // parallelFor(launchingThread, NumberOfLaunchingThreads) {
      // for (int launchingThread = 0; launchingThread < NumberOfLaunchingThreads; launchingThread++) {
        tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
        executeKernels(device, cellData, cellIndex, timingComputeKernel);
        watchKernelLaunch.stop();
        timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
      // } endParallelFor
      // }
      // #if defined(SharedOMP)
      // }
      // #endif
      }
      sample++;
    }

    reportRuntime(markerName, timingKernelLaunch, numberOfCells);
    allocateAndStoreOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
    validateOutcome(cellData.QOut, cellData.maxEigenvalue, numberOfCells);
  };

  if constexpr (AssessHostKernels) {
    assessKernel(runKernels,
      "host, stateless, batched, AoS, serial",
      tarch::accelerator::Device::DefaultDevice
    );
  } // AssessHostKernels

//   if constexpr (AssessDeviceKernels) {
// #if defined(GPUOffloadingOMP)

// #endif // GPUOffloadingOMP

// #if defined(GPUOffloadingSYCL)

// #endif // GPUOffloadingSYCL

// #if defined(GPUOffloadingCPP)

// #endif // GPUOffloadingCPP
//   }

  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    tarch::freeMemory(cellData.QIn[cellIndex], tarch::MemoryLocation::Heap);
    tarch::freeMemory(cellData.QOut[cellIndex], tarch::MemoryLocation::Heap);
  }
}

int main(int argc, char** argv) {
  peano4::initParallelEnvironment(&argc, &argv);
  // Do this early, so people can use logInfo properly.
  repositories::initLogFilters();
  tarch::initNonCriticalAssertionEnvironment();
  peano4::fillLookupTables();
  repositories::initSharedMemoryAndGPUEnvironment();

  if constexpr (EnableFPE) {
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
  }

  logInfo(
    "main()",
    "number of compute threads: "
    << tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  logInfo(
    "main()",
    "number of threads launching compute kernels: "
    << NumberOfLaunchingThreads
  );
  logInfo(
    "main()",
    "number of unknowns: "
    << AderSolver::NumberOfUnknowns
  );
  logInfo(
    "main()",
    "number of auxiliary variables: "
    << AderSolver::NumberOfAuxiliaryVariables
  );
  logInfo(
    "main()",
    "number of finite volumes per axis per cell: "
    << AderSolver::Order
  );
  logInfo(
    "main()",
    "number of samples per measurement: "
    << NumberOfSamples
  );
  logInfo(
    "main()",
    "floating-point exception handler enabled: "
    << std::boolalpha << EnableFPE
  );
  logInfo(
    "main()",
    "performing accuracy checks with precision: "
    << Accuracy
  );
#if defined(GPUOffloadingSYCL)
  logInfo(
    "main()",
    "set SYCL_DEVICE_FILTER=gpu or ONEAPI_DEVICE_SELECTOR=cuda:0 when using SYCL on the device"
  );
  logInfo(
    "main()",
    "set SYCL_PI_TRACE=2 in case of runtime errors"
  );
#endif

// #if defined(SharedOMP)
//   #pragma omp parallel
//   {
//     #pragma omp master
//     {
// #endif
      for (int i = 0; i < NumberOfCellsToStudy.size(); i++) {
        logInfo("main()", "number of cells: " << NumberOfCellsToStudy[i]);
        runBenchmarks(NumberOfCellsToStudy[i]);
        freeOutcome(NumberOfCellsToStudy[i]);
      }
// #if defined(SharedOMP)
//     }
//   }
// #endif

  tarch::shutdownNonCriticalAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  if (outcomeIsInvalid) {
    return EXIT_FAILURE; // Make sure the CI pipeline reports an error
  }

  return EXIT_SUCCESS;
}