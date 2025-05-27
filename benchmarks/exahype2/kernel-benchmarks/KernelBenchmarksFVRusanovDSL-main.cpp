// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "KernelBenchmarksFVRusanovDSL-main.h"

#include "config.h"
#include "Constants.h"

#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"

#include "tasks/FVRusanovEnclaveTask.h"

#include "peano4/peano.h"

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
::tarch::timing::Measurement         timingComputeKernel;
::tarch::multicore::BooleanSemaphore validateOutcomeSemaphore;

double                               validMaxEigenvalue = 0.0;
double*                              validOutcome       = nullptr;
constexpr double TimeStamp    = 0.5;
constexpr double TimeStepSize = 1e-6;
constexpr double CellSize     = 0.1;
constexpr double CellOffset   = 4.0;
constexpr int    HaloSize     = 1;

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

constexpr int NumberOfInputEntriesPerPatch
  = (FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
  * (FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
#if Dimensions == 3
  * (FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
#endif
  * (FVRusanov::NumberOfUnknowns + FVRusanov::NumberOfAuxiliaryVariables);

constexpr int NumberOfOutputEntriesPerPatch
  = (FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch + 0)
  * (FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch + 0)
#if Dimensions == 3
  * (FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch + 0)
#endif
  * (FVRusanov::NumberOfUnknowns + FVRusanov::NumberOfAuxiliaryVariables);

constexpr int NumberOfFiniteVolumesPerPatch
  = FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch
  * FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch
#if Dimensions == 3
  * FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch
#endif
  ;

/**
 * Set input data
 *
 * We really don't care if this makes any sense. Just make the data reasonably
 * smooth and ensure that all is positive, as there are density and energy
 * among the unknowns. If they become negative or zero, the compute kernels do
 * not make any sense anymore.
 */
void initInputData(double* Q) {
  for (int i = 0; i < NumberOfInputEntriesPerPatch; i++) {
    Q[i] = std::sin(1.0 * i / (NumberOfInputEntriesPerPatch) * ::tarch::la::PI);
  }
}


/**
 * Store outcome of one compute kernel
 *
 * Make a persistent snapshot of a solution and assume, from hereon, that
 * this snapshot is the valid data. You can call this routine as often as
 * you want. Only the very first call will trigger a snapshot.
 */
void storeOutcome(const double* Q, const double maxEigenvalue) {
  ::tarch::multicore::Lock lock(validateOutcomeSemaphore);
  if (validOutcome == nullptr) {
    validOutcome = new double[NumberOfOutputEntriesPerPatch]{0.0};
    std::memcpy(validOutcome, Q, sizeof(double) * NumberOfOutputEntriesPerPatch);
    validMaxEigenvalue = maxEigenvalue;
    logInfo("storeOutcome()", "Bookmarked reference solution");
  }
}


/**
 * Validate data against pre-stored simulation outcome
 *
 * Works if and only if storeOutcome has been performed before.
 *
 * @return Tuple of maximum difference and total number of errors.
 */
std::tuple<double, int> validateOutcome(
  double*      Q,
  int          patchIndex,
  const double maxEigenvalue
) {
  ::tarch::multicore::Lock lock(validateOutcomeSemaphore);
  int                      index         = 0;
  int                      errors        = 0;
  double                   maxDifference = 0.0;

  for (int i = 0; i < NumberOfOutputEntriesPerPatch; i++) {
    if (not ::tarch::la::equals(Q[i], validOutcome[i], Accuracy)) {
      errors++;
      std::cerr.precision(16);
      logError(
        "validateOutcome()",
        std::fixed
          << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i]
          << "!=" << validOutcome[i]
      );
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i] - validOutcome[i]));
    index++;
  }

  if (not tarch::la::equals(maxEigenvalue, validMaxEigenvalue, Accuracy)) {
    std::cerr.precision(16);
    logError(
      "validateOutcome()",
      std::fixed
        << " maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue["
        << patchIndex << "]: " << maxEigenvalue << "!=" << validMaxEigenvalue
    );
    errors++;
  }

  return {maxDifference, errors};
}


void reportRuntime(
  const std::string&                  kernelIdentificator,
  const ::tarch::timing::Measurement& timingKernelLaunch,
  int                                 patches
) {
  std::stringstream ss;
  ss << "\n";
  ss << kernelIdentificator << ":\n\t";

  ss << timingComputeKernel.getValue() << " |\n\t";
  ss
    << (timingComputeKernel.getValue() / patches / NumberOfFiniteVolumesPerPatch
       )
    << " |\n\t";
  ss << timingComputeKernel.toString() << " |\n\t";

  ss << timingKernelLaunch.getValue() << " |\n\t";
  ss
    << (timingKernelLaunch.getValue() / patches / NumberOfFiniteVolumesPerPatch
       );
  ss << " |\n\t" << timingKernelLaunch.toString();

  logInfo("reportRuntime()", ss.str());
}


/**
 * We want to use all kernels exactly the same way. However, the various
 * kernels all have slightly different signatures. So we write small helper
 * functions (wrappers) which map the generic test signature onto the
 * specific kernel.
 *
 * To make this possible, all parameters which are not part of the generic
 * interface, i.e., which are not patch data or the boolean, have to be
 * mapped onto template arguments.
 */
template <void (*Kernel)(exahype2::CellData<double, double>&, tarch::timing::Measurement&)>
void wrapHostKernel(int device, exahype2::CellData<double, double>& patchData) {
  assertionEquals(device, tarch::accelerator::Device::HostDevice);
  Kernel(patchData, timingComputeKernel);
}

 template <void (*Kernel)(exahype2::CellData<double, double>&, tarch::timing::Measurement&, peano4::utils::LoopPlacement),
          peano4::utils::LoopPlacement loopPlacement>
void wrapHostKernel(int device, exahype2::CellData<double, double>& patchData) {
  assertionEquals(device, tarch::accelerator::Device::HostDevice);
  Kernel(patchData, timingComputeKernel, loopPlacement);
}

template <void (*Kernel)(int, exahype2::CellData<double, double>&, tarch::timing::Measurement&)>
void wrapDeviceKernel(int device, exahype2::CellData<double, double>& patchData) {
  assertion(device != tarch::accelerator::Device::HostDevice);
  Kernel(device, patchData, timingComputeKernel);
}

/**
 * Run the benchmark for one particular number of patches
 *
 * This operation benchmarks exclusively the host performance. This happens in
 * two steps: We first assess the baseline performance, i.e., the serial kernel
 * call, and then we look if the multithreaded stateless optimisation pays off
 * and what speedup we obtain.
 *
 * These two steps may run on multiple threads to mimic the spacetree tasking
 * and domain decomposition. In OpenMP for example, there is
 * one parallel loop around the two steps which invokes the same benchmark per
 * loop entry. However, only the first thread always will report its runtime.
 * If you are not bandwidth-bound, the data you obtain should thus be
 * independent of the number of threads used.
 *
 * @param numberOfPatches Number of patches to study
 */
void runBenchmarks(int numberOfPatches, int launchingThreads) {
  ::peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  ::peano4::datamanagement::CellMarker marker(event);

  auto assessKernel =
    [&](
      std::function<void(int device, ::exahype2::CellData<double, double>& patchData)> kernel,
      const std::string& markerName,
      int                launchingThreads,
      int                device,
      int                patches
    ) -> void {
    timingComputeKernel.erase();
    ::tarch::timing::Measurement timingKernelLaunch;

    // TODO: Does the number of samples change the outcome of the solution?
    for (int j = 0; j < NumberOfSamples; j++) {
      parallelFor(launchingThread, launchingThreads) {
        ::exahype2::CellData<double, double> patchData(patches);
        for (int i = 0; i < patches; i++) {
          patchData.QIn[i] = ::tarch::allocateMemory<double>(
            NumberOfInputEntriesPerPatch,
            ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
          );
          patchData.t[i]    = TimeStamp;
          patchData.dt[i]   = TimeStepSize;
          patchData.QOut[i] = ::tarch::allocateMemory<double>(
            NumberOfOutputEntriesPerPatch,
            ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
          );
          patchData.cellCentre[i] = ::tarch::la::Vector<Dimensions, double>(
            CellOffset + 0.5 * CellSize
          );
          patchData.cellSize[i] = ::tarch::la::Vector<Dimensions, double>(
            CellSize
          );
          patchData.maxEigenvalue[i] = 0.0;
          initInputData(patchData.QIn[i]);
          std::memset(
            patchData.QOut[i],
            0.0,
            NumberOfOutputEntriesPerPatch * sizeof(double)
          );
        }

        ::tarch::timing::Watch
          watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
        kernel(device, patchData);
        watchKernelLaunch.stop();
        timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());

        if constexpr (Accuracy > 0.0) {
          int    errors        = 0;
          double maxDifference = 0.0;
          for (int i = 0; i < patches; i++) {
            storeOutcome(patchData.QOut[i], patchData.maxEigenvalue[i]);
            auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome(
              patchData.QOut[i],
              i,
              patchData.maxEigenvalue[i]
            );
            errors += errorsPerPatch;
            maxDifference = std::max(maxDifference, maxDifferencePerPatch);
          }

          if (errors > 0) {
            logError(
              "runBenchmarks()",
              "Max difference of outcome from \""
                << markerName << "\" for all patches is " << maxDifference
                << " (admissible accuracy=" << Accuracy << ")"
                << " for " << errors << " entries"
            );
            std::abort();
          }
        }

        for (int i = 0; i < patches; i++) {
          ::tarch::freeMemory(
            patchData.QIn[i],
            ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
          );
          ::tarch::freeMemory(
            patchData.QOut[i],
            ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
          );
        }
      }
      endParallelFor
    }

    if (::tarch::mpi::Rank::getInstance().getRank() == ::tarch::mpi::Rank::getGlobalMasterRank()) {
      reportRuntime(markerName, timingKernelLaunch, patches);
    }
  };

  // Kernel launches
  const int device = 0;
  const int rank          = ::tarch::mpi::Rank::getInstance().getRank();
  const int numberOfRanks = ::tarch::mpi::Rank::getInstance().getNumberOfRanks(
  );
  const int patchesPerProcess = numberOfPatches / numberOfRanks;
  const int remainder         = numberOfPatches % numberOfRanks;
  const int startPatch        = rank * patchesPerProcess;
  const int endPatch          = startPatch + patchesPerProcess
                       + (rank == numberOfRanks - 1 ? remainder : 0);
  const int localPatches = endPatch - startPatch;

  // Headers
  std::stringstream ss;
  ss << std::left;
  ss << "\n";
  ss << "Kernel ID:\n\t";
  ss << "Compute Kernel Time |\n\t";
  ss << "Compute Kernel Time (Normalised) |\n\t";
  ss << "Compute Kernel String |\n\t";
  ss << "Kernel Launch Time |\n\t";
  ss << "Kernel Launch Time (Normalised) |\n\t";
  ss << "Kernel Launch String";

  if (::tarch::mpi::Rank::getInstance().getRank() == ::tarch::mpi::Rank::getGlobalMasterRank()) {
    logInfo("runBenchmarks()", "Number of patches per rank: " << localPatches);
    logInfo("runBenchmarks()", ss.str());
  }

  std::string deviceString;
#ifdef Parallel
  std::vector<int> devices(numberOfRanks);
  MPI_Gather(&device, 1, MPI_INT, devices.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
  for (int i = 0; i < numberOfRanks; i++) {
    deviceString += std::to_string(devices[i]);
    if (i < numberOfRanks - 1) {
      deviceString += ",";
    }
  }
#else
  deviceString = std::to_string(device);
#endif

  if constexpr (AssessHostKernels) {
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep>
      >,
      "host, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    #if defined (SharedOMP)
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep>
      >,
      "host, AoS, omp",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    #endif
}
}


int main(int argc, char** argv) {
  ::peano4::initParallelEnvironment(&argc, &argv);
  // Do this early, so people can use logInfo properly.
  repositories::initLogFilters();
  ::tarch::initNonCriticalAssertionEnvironment();
  ::tarch::multicore::initSmartMPI();
  ::peano4::fillLookupTables();
  repositories::initSharedMemoryAndGPUEnvironment();

  if constexpr (EnableFPE) {
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
  }

  if (::tarch::mpi::Rank::getInstance().getRank() == ::tarch::mpi::Rank::getGlobalMasterRank()) {
    logInfo("main()", "Dimensions: " << Dimensions);
    logInfo(
      "main()",
      "Number of threads launching compute kernels: "
        << NumberOfLaunchingThreads
    );
    logInfo(
      "main()",
      "Number of Unknowns: "
        << FVRusanov::NumberOfUnknowns
    );
    logInfo(
      "main()",
      "Number of patches per thread/compute kernel launch to study: "
        << toString(NumberOfPatchesToStudy)
    );
    logInfo(
      "main()",
      "Number of compute threads: "
        << ::tarch::multicore::Core::getInstance().getNumberOfThreads()
    );
    logInfo(
      "main()",
      "Number of MPI ranks: "
        << ::tarch::mpi::Rank::getInstance().getNumberOfRanks()
    );
    logInfo(
      "main()",
      "Number of GPU devices: "
        << ::tarch::accelerator::Device::getInstance().getNumberOfDevices()
    );
    logInfo(
      "main()",
      "Number of finite volumes per axis per patch: "
        << FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch
    );
    logInfo("main()", "Number of samples per measurement: " << NumberOfSamples);
    logInfo(
      "main()",
      "Evaluate max. eigenvalue (reduction step): "
        << std::boolalpha << EvaluateMaximumEigenvalueAfterTimeStep
    );
    if constexpr (EnableFPE) {
      logInfo("main()", "Floating-point exception handler enabled");
    }
    if constexpr (Accuracy > 0.0) {
      logInfo(
        "main()",
        "Performing accuracy checks with precision: " << Accuracy
      );
    }
#if defined(GPUOffloadingSYCL)
    logInfo(
      "main()",
      "Set SYCL_DEVICE_FILTER=gpu or ONEAPI_DEVICE_SELECTOR=cuda:0 when using SYCL on the device"
    );
    logInfo("main()", "Set SYCL_PI_TRACE=2 in case of runtime errors");
#endif
  }

#if defined(SharedOMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      for (int n = 0; n < NumberOfPatchesToStudy.size(); n++) {
        runBenchmarks(NumberOfPatchesToStudy[n], NumberOfLaunchingThreads);
      }
#if defined(SharedOMP)
    }
  }
#endif

  if constexpr (Accuracy > 0.0) {
    logInfo(
      "main()",
      "All kernels yield the same outcome up to a accuracy of "
        << Accuracy << " unless reported otherwise"
    );
  } else {
    logInfo("main()", "No accuracy checks were performed");
  }

  delete[] validOutcome;
  ::tarch::multicore::shutdownSmartMPI();
  ::tarch::shutdownNonCriticalAssertionEnvironment();
  ::peano4::shutdownParallelEnvironment();
  return EXIT_SUCCESS;
}