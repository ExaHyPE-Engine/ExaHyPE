// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelBenchmarksFVRusanov-main.h"

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

// Check the outcomes of each kernel
double** validQ = nullptr;
double* validMaxEigenvalue = nullptr;
bool outcomeIsInvalid = false;

tarch::timing::Measurement timingComputeKernel;

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
    Q[i] = std::sin(1.0 * i / (NumberOfInputEntriesPerPatch) * tarch::la::PI);
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
                             const int            numberOfPatches
) {
  if constexpr (Accuracy <= 0.0) return;
  if (validQ == nullptr and validMaxEigenvalue == nullptr) {
    validQ = new double*[numberOfPatches];
    for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
      validQ[patchIndex] = new double[NumberOfOutputEntriesPerPatch];
      std::memcpy(validQ[patchIndex], Q[patchIndex], sizeof(double) * NumberOfOutputEntriesPerPatch);
    }
    validMaxEigenvalue = new double[numberOfPatches];
    std::memcpy(validMaxEigenvalue, maxEigenvalue, sizeof(double) * numberOfPatches);
    logInfo("storeOutcome(...)", "bookmarked reference solution");
  }
}

void freeOutcome(const int numberOfPatches) {
  if constexpr (Accuracy <= 0.0) return;
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    delete[] validQ[patchIndex];
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
  const int            numberOfPatches
) {
  if constexpr (Accuracy <= 0.0) return;
  int errors = 0;
  double maxDifference = 0.0;

  std::cerr.precision(16);
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    for (int i = 0; i < NumberOfOutputEntriesPerPatch; i++) {
      if (not tarch::la::equals(Q[patchIndex][i], validQ[patchIndex][i], Accuracy)) {
        if (!errors) { // Only print once
          logError("validateOutcome(...)",
            std::fixed
              << "patch " << patchIndex << ": "
              << "Q[" << i << "]!=validQ[" << i << "] ("
              << Q[patchIndex][i]
              << "!="
              << validQ[patchIndex][i]
              << ")"
          );
        }
        errors++;
        maxDifference = std::max(maxDifference, std::abs(Q[patchIndex][i] - validQ[patchIndex][i]));
    }
  }

    if (not tarch::la::equals(maxEigenvalue[patchIndex], validMaxEigenvalue[patchIndex], Accuracy)) {
      if (!errors) {
        logError("validateOutcome(...)",
          std::fixed
            << "maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue[" << patchIndex << "] ("
            << maxEigenvalue[patchIndex] << "!=" << validMaxEigenvalue[patchIndex]
            << ")";
        );
      }
      errors++;
      maxDifference = std::max(maxDifference, std::abs(maxEigenvalue[patchIndex] - validMaxEigenvalue[patchIndex]));
    }
  }

  if (errors > 0) {
    outcomeIsInvalid = true;
    logError("validateOutcome(...)",
      "max difference of outcome from all patches is "
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
  int                                 numberOfPatches
) {
  std::stringstream ss;
  ss << "\n";
  ss << kernelIdentificator << ":\n\t";
  ss << timingComputeKernel.getValue() << " |\n\t";
  ss << (timingComputeKernel.getValue() / numberOfPatches / NumberOfFiniteVolumesPerPatch) << " |\n\t";
  ss << timingComputeKernel.toString() << " |\n\t";
  ss << timingKernelLaunch.getValue() << " |\n\t";
  ss << (timingKernelLaunch.getValue() / numberOfPatches / NumberOfFiniteVolumesPerPatch);
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
 * @param numberOfPatches Number of patches to study
 */
void runBenchmarks(int numberOfPatches) {
  exahype2::CellData<double, double> patchData(numberOfPatches);
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    patchData.QIn[patchIndex]           = tarch::allocateMemory<double>(NumberOfInputEntriesPerPatch, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    patchData.t[patchIndex]             = TimeStamp;
    patchData.dt[patchIndex]            = TimeStepSize;
    patchData.QOut[patchIndex]          = tarch::allocateMemory<double>(NumberOfOutputEntriesPerPatch, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    patchData.cellCentre[patchIndex]    = tarch::la::Vector<Dimensions, double>(CellOffset + 0.5 * CellSize);
    patchData.cellSize[patchIndex]      = tarch::la::Vector<Dimensions, double>(CellSize);
    patchData.maxEigenvalue[patchIndex] = 0.0;
    initInputData(patchData.QIn[patchIndex]);
    std::memset(patchData.QOut[patchIndex], 0.0, NumberOfOutputEntriesPerPatch * sizeof(double));
  }

  auto assessKernel = [&](
    std::function<void(int device, exahype2::CellData<double, double>& patchData)> kernelWrapper,
    const std::string& markerName,
    const int          device
  ) -> void {
    timingComputeKernel.erase();
    tarch::timing::Measurement timingKernelLaunch;

    int sample = 0;
    while (sample <= NumberOfSamples) {
      // Reset output data
      for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
        patchData.maxEigenvalue[patchIndex] = 0.0;
        std::memset(patchData.QOut[patchIndex], 0.0, NumberOfOutputEntriesPerPatch * sizeof(double));
      }

      parallelFor(launchingThread, NumberOfLaunchingThreads) {
        tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
        kernelWrapper(device, patchData);
        watchKernelLaunch.stop();
        timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());
      } endParallelFor

      sample++;
    }

    reportRuntime(markerName, timingKernelLaunch, numberOfPatches);
    allocateAndStoreOutcome(patchData.QOut, patchData.maxEigenvalue, numberOfPatches);
    validateOutcome(patchData.QOut, patchData.maxEigenvalue, numberOfPatches);
  };

  if constexpr (AssessHostKernels) {
#if 1 // Batched host kernels
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>,
      peano4::utils::LoopPlacement::Serial>,
      "host, stateless, batched, AoS, serial",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>,
      peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, batched, AoS, spread-out",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::Serial>,
      "host, stateless, batched, SoA, serial",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, batched, SoA, spread-out",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::Serial>,
      "host, stateless, batched, AoSoA, serial",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, batched, AoSoA, spread-out",
      tarch::accelerator::Device::HostDevice
    );
#endif // Batched host kernels

#if 1 // Patch-wise host kernels
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>,
      peano4::utils::LoopPlacement::Serial>,
      "host, stateless, patch-wise, AoS, serial",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>,
      peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, patch-wise, AoS, spread-out",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::Serial>,
      "host, stateless, patch-wise, SoA, serial",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, patch-wise, SoA, spread-out",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::Serial>,
      "host, stateless, patch-wise, AoSoA, serial",
      tarch::accelerator::Device::HostDevice
    );
    assessKernel(wrapHostKernel<
      exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>,
      peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, patch-wise, AoSoA, spread-out",
      tarch::accelerator::Device::HostDevice
    );
#endif // Patch-wise host kernels
  } // AssessHostKernels

  if constexpr (AssessDeviceKernels) {
#if defined(GPUOffloadingOMP)
#if 1 // Batched device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, batched, AoS, copy",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, batched, SoA, copy",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, batched, AoSoA, copy",
      0
    );
#endif // Batched device kernels

#if 1 // Patch-wise device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, patch-wise, AoS, copy",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, patch-wise, SoA, copy",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, patch-wise, AoSoA, copy",
      0
    );
#endif // Patch-wise device kernels
#endif // GPUOffloadingOMP

#if defined(GPUOffloadingSYCL)
#if 1 // Batched device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, batched, AoS, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, batched, SoA, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, batched, AoSoA, usm",
      0
    );
#endif // Batched device kernels

#if 1 // Patch-wise device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, patch-wise, AoS, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, patch-wise, SoA, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, patch-wise, AoSoA, usm",
      0
    );
#endif // Patch-wise device kernels

#if 1 // Task-graph device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, task-graph, AoS, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, task-graph, SoA, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, task-graph, AoSoA, usm",
      0
    );
#endif // Task-graph device kernels
#endif // GPUOffloadingSYCL

#if defined(GPUOffloadingCPP)
#if 1 // Batched device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, batched, AoS, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, batched, SoA, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, batched, AoSoA, usm",
      0
    );
#endif // Batched device kernels

#if 1 // Patch-wise device kernels
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device, stateless, patch-wise, AoS, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::SoALexicographicEnumerator>>,
      "device, stateless, patch-wise, SoA, usm",
      0
    );
    assessKernel(wrapDeviceKernel<
      exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanov,
        FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanov::NumberOfUnknowns,
        FVRusanov::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device, stateless, patch-wise, AoSoA, usm",
      0
    );
#endif // Patch-wise device kernels
#endif // GPUOffloadingCPP
  }

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    tarch::freeMemory(patchData.QIn[patchIndex], tarch::MemoryLocation::Heap);
    tarch::freeMemory(patchData.QOut[patchIndex], tarch::MemoryLocation::Heap);
  }
}

int main(int argc, char** argv) {
  peano4::initParallelEnvironment(&argc, &argv);
  // Do this early, so people can use logInfo properly.
  repositories::initLogFilters();
  tarch::initNonCriticalAssertionEnvironment();
  tarch::multicore::initSmartMPI();
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
    << FVRusanov::NumberOfUnknowns
  );
  logInfo(
    "main()",
    "number of auxiliary variables: "
    << FVRusanov::NumberOfAuxiliaryVariables
  );
  logInfo(
    "main()",
    "number of finite volumes per axis per patch: "
    << FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch
  );
  logInfo(
    "main()",
    "number of samples per measurement: "
    << NumberOfSamples
  );
  logInfo(
    "main()",
    "evaluate max. eigenvalue (reduction step): "
    << std::boolalpha << EvaluateMaximumEigenvalueAfterTimeStep
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

#if defined(SharedOMP)
  #pragma omp parallel
  {
    #pragma omp master
    {
#endif
      for (int i = 0; i < NumberOfPatchesToStudy.size(); i++) {
        logInfo("main()", "number of patches: " << NumberOfPatchesToStudy[i]);
        runBenchmarks(NumberOfPatchesToStudy[i]);
        freeOutcome(NumberOfPatchesToStudy[i]);
      }
#if defined(SharedOMP)
    }
  }
#endif

  tarch::multicore::shutdownSmartMPI();
  tarch::shutdownNonCriticalAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  if (outcomeIsInvalid) {
    return EXIT_FAILURE; // Make sure the CI pipeline reports an error
  }

  return EXIT_SUCCESS;
}
