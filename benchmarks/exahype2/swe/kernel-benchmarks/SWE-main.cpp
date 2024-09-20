// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE-main.h"

#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON

#include "config.h"
#include "Constants.h"
#include "exahype2/UserInterface.h"
#include "observers/CreateGrid.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"
#include "peano4/peano.h"
#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "tasks/SWEEnclaveTask.h"
#include "toolbox/loadbalancing/loadbalancing.h"

using namespace applications::exahype2::swe;
::tarch::logging::Log                _log("::");
double                               validMaxEigenvalue = 0.0;
double*                              validOutcome       = nullptr;
static constexpr double              TimeStamp          = 0.5;
static constexpr double              TimeStepSize       = 1e-6;
static constexpr double              CellSize           = 0.1;
static constexpr double              CellOffset         = 4.0;
static constexpr int                 HaloSize           = 1;
::tarch::timing::Measurement         timingComputeKernel;
::tarch::multicore::BooleanSemaphore validateOutcomeSemaphore;
static_assert(
  Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0
);

#if Dimensions == 2
static constexpr int NumberOfInputEntries
  = (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
    * (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
    * (SWE::NumberOfUnknowns + SWE::NumberOfAuxiliaryVariables);
static constexpr int NumberOfOutputEntries
  = (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (SWE::NumberOfUnknowns + SWE::NumberOfAuxiliaryVariables);
static constexpr int NumberOfFiniteVolumesPerPatch
  = SWE::NumberOfFiniteVolumesPerAxisPerPatch
    * SWE::NumberOfFiniteVolumesPerAxisPerPatch;
#else
static constexpr int NumberOfInputEntries
  = (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
    * (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
    * (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
    * (SWE::NumberOfUnknowns + SWE::NumberOfAuxiliaryVariables);
static constexpr int NumberOfOutputEntries
  = (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (SWE::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (SWE::NumberOfUnknowns + SWE::NumberOfAuxiliaryVariables);
static constexpr int NumberOfFiniteVolumesPerPatch
  = SWE::NumberOfFiniteVolumesPerAxisPerPatch
    * SWE::NumberOfFiniteVolumesPerAxisPerPatch
    * SWE::NumberOfFiniteVolumesPerAxisPerPatch;
#endif


/**
 * Set input data
 *
 * We really don't care if this makes any sense. Just make the data reasonably
 * smooth and ensure that all is positive, as there are density and energy
 * among the unknowns. If they become negative or zero, the compute kernels do
 * not make any sense anymore.
 */
void initInputData(double* Q) {
  for (int i = 0; i < NumberOfInputEntries; i++) {
    Q[i] = std::sin(1.0 * i / (NumberOfInputEntries) * ::tarch::la::PI);
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
    validOutcome = new double[NumberOfOutputEntries]{0.0};
    std::memcpy(validOutcome, Q, sizeof(double) * NumberOfOutputEntries);
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

  for (int i = 0; i < NumberOfOutputEntries; i++) {
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
 * This is a wrapper around the kernel call with the functors
 *
 * We want to use all kernels exactly the same way. However, the various
 * kernels all have slightly different signatures. So we write small helper
 * functions (wrappers) which map the generic test signature onto the
 * specific kernel.
 *
 * To make this possible, all parameters which are not part of the generic
 * interface, i.e., which are not patch data or the boolean, have to be
 * mapped onto template arguments.
 *
 * Here, we use the functor-based generic kernel implementation.
 */
template <
  class TempDataEnumerator,
  ::peano4::utils::LoopPlacement loopParallelism>
void wrapPatchwiseHeapFunctorsHostKernel(
  int                   device,
  ::exahype2::CellData& patchData
) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapFunctors<
    SWE::NumberOfFiniteVolumesPerAxisPerPatch,
    HaloSize,
    SWE::NumberOfUnknowns,
    SWE::NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumerator>(
    patchData,
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ F
    ) -> void {
      if constexpr (EvaluateFlux) {
        repositories::instanceOfSWE.flux(Q, x, h, t, dt, normal, F);
      }
    },
    [&](
      const double* __restrict__ Q,
      const double* __restrict__ deltaQ,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ BTimesDeltaQ
    ) -> void {
      if constexpr (EvaluateNonconservativeProduct) {
        repositories::instanceOfSWE
          .nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      double* __restrict__ S
    ) -> void {
      if constexpr (EvaluateSource) {
        repositories::instanceOfSWE.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal
    ) -> double {
      return repositories::instanceOfSWE.maxEigenvalue(Q, x, h, t, dt, normal);
    },
    timingComputeKernel,
    loopParallelism
  );
}


/**
 * Another wrapper.
 *
 * @see wrapPatchwiseHeapFunctorsHostKernel()
 */
template <
  class TempDataEnumerator,
  ::peano4::utils::LoopPlacement loopParallelism>
void wrapBatchedHeapFunctorHostKernels(
  int                   device,
  ::exahype2::CellData& patchData
) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapFunctors<
    SWE::NumberOfFiniteVolumesPerAxisPerPatch,
    HaloSize,
    SWE::NumberOfUnknowns,
    SWE::NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumerator>(
    patchData,
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ F
    ) -> void {
      if constexpr (EvaluateFlux) {
        repositories::instanceOfSWE.flux(Q, x, h, t, dt, normal, F);
      }
    },
    [&](
      const double* __restrict__ Q,
      const double* __restrict__ deltaQ,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ BTimesDeltaQ
    ) -> void {
      if constexpr (EvaluateNonconservativeProduct) {
        repositories::instanceOfSWE
          .nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      double* __restrict__ S
    ) -> void {
      if constexpr (EvaluateSource) {
        repositories::instanceOfSWE.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal
    ) -> double {
      return repositories::instanceOfSWE.maxEigenvalue(Q, x, h, t, dt, normal);
    },
    timingComputeKernel,
    loopParallelism
  );
}


/**
 * Another wrapper.
 *
 * @see wrapPatchwiseHeapFunctorsHostKernel()
 */
template <
  class TempDataEnumerator,
  ::peano4::utils::LoopPlacement loopParallelism>
void wrapVolumewiseFunctorHostKernels(
  int                   device,
  ::exahype2::CellData& patchData
) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseFunctors<
    SWE::NumberOfFiniteVolumesPerAxisPerPatch,
    HaloSize,
    SWE::NumberOfUnknowns,
    SWE::NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumerator>(
    patchData,
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ F
    ) -> void {
      if constexpr (EvaluateFlux) {
        repositories::instanceOfSWE.flux(Q, x, h, t, dt, normal, F);
      }
    },
    [&](
      const double* __restrict__ Q,
      const double* __restrict__ deltaQ,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal,
      double* __restrict__ BTimesDeltaQ
    ) -> void {
      if constexpr (EvaluateNonconservativeProduct) {
        repositories::instanceOfSWE
          .nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      double* __restrict__ S
    ) -> void {
      if constexpr (EvaluateSource) {
        repositories::instanceOfSWE.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](
      const double* __restrict__ Q,
      const ::tarch::la::Vector<Dimensions, double>& x,
      const ::tarch::la::Vector<Dimensions, double>& h,
      double                                         t,
      double                                         dt,
      int                                            normal
    ) -> double {
      return repositories::instanceOfSWE.maxEigenvalue(Q, x, h, t, dt, normal);
    },
    timingComputeKernel,
    loopParallelism
  );
}


/**
 * Wrapper around stateless kernel invocations
 *
 * @see wrapPatchwiseHeapFunctorsHostKernel()
 */
template <
  void (*Function)(
    ::exahype2::CellData&,
    ::tarch::timing::Measurement&,
    ::peano4::utils::LoopPlacement
  ),
  ::peano4::utils::LoopPlacement loopParallelism>
void wrapStatelessHostKernel(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);
  Function(patchData, timingComputeKernel, loopParallelism);
}


template <
  void (*Function)(int, ::exahype2::CellData&, ::tarch::timing::Measurement&)>
void wrapDeviceKernel(int device, ::exahype2::CellData& patchData) {
  assertion(device != ::tarch::accelerator::Device::HostDevice);
  Function(device, patchData, timingComputeKernel);
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
      std::function<void(int device, ::exahype2::CellData& patchData)> kernel,
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
        ::exahype2::CellData patchData(patches);
        for (int i = 0; i < patches; i++) {
          patchData.QIn[i] = ::tarch::allocateMemory<double>(
            NumberOfInputEntries,
            ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory
          );
          patchData.t[i]    = TimeStamp;
          patchData.dt[i]   = TimeStepSize;
          patchData.QOut[i] = ::tarch::allocateMemory<double>(
            NumberOfOutputEntries,
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
            NumberOfOutputEntries * sizeof(double)
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
    assessKernel(
      wrapBatchedHeapFunctorHostKernels<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, functors, batched, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapBatchedHeapFunctorHostKernels<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, functors, batched, AoS, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapBatchedHeapFunctorHostKernels<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, functors, batched, AoS, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );

    assessKernel(
      wrapPatchwiseHeapFunctorsHostKernel<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, functors, patch-wise, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapPatchwiseHeapFunctorsHostKernel<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, functors, patch-wise, AoS, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapPatchwiseHeapFunctorsHostKernel<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, functors, patch-wise, AoS, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );

    assessKernel(
      wrapVolumewiseFunctorHostKernels<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, functors, volume-wise, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapVolumewiseFunctorHostKernels<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, functors, volume-wise, AoS, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapVolumewiseFunctorHostKernels<
        ::exahype2::enumerator::AoSLexicographicEnumerator,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, functors, volume-wise, AoS, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );

    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, batched, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, batched, AoS, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, batched, AoS, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, batched, SoA, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, batched, SoA, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, batched, SoA, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, batched, AoSoA, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, batched, AoSoA, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, batched, AoSoA, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    if constexpr (not Accuracy > 0.0) {
      assessKernel(
        wrapStatelessHostKernel<
          ::exahype2::fv::rusanov::timeStepWithRusanovBatchedInsituStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep>,
          ::peano4::utils::LoopPlacement::Serial>,
        "host, stateless, batched, insitu, serial",
        launchingThreads,
        ::tarch::accelerator::Device::HostDevice,
        localPatches
      );
    }

    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, patch-wise, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, patch-wise, AoS, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, patch-wise, AoS, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, patch-wise, SoA, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, patch-wise, SoA, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, patch-wise, SoA, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, patch-wise, AoSoA, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, patch-wise, AoSoA, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, patch-wise, AoSoA, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    if constexpr (not Accuracy > 0.0) {
      assessKernel(
        wrapStatelessHostKernel<
          ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseInsituStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep>,
          ::peano4::utils::LoopPlacement::Serial>,
        "host, stateless, patch-wise, insitu, serial",
        launchingThreads,
        ::tarch::accelerator::Device::HostDevice,
        localPatches
      );
    }

    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, volume-wise, AoS, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, volume-wise, AoS, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, volume-wise, AoS, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, volume-wise, SoA, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, volume-wise, SoA, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, volume-wise, SoA, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Serial>,
      "host, stateless, volume-wise, AoSoA, serial",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::Nested>,
      "host, stateless, volume-wise, AoSoA, nested",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopPlacement::SpreadOut>,
      "host, stateless, volume-wise, AoSoA, spread-out",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
  }

  if constexpr (AssessDeviceKernels) {
#if defined(GPUOffloadingSYCL)
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoSoA, usm",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoSoA, usm",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, AoSoA, usm",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoS, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, SoA, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoSoA, copy",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoS, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, SoA, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoSoA, copy",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphCopyStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, AoS, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphCopyStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, SoA, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphCopyStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, AoSoA, copy",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedManagedStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoS, managed",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedManagedStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, SoA, managed",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedManagedStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoSoA, managed",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::
          timeStepWithRusanovPatchwiseManagedStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep,
            ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoS, managed",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::
          timeStepWithRusanovPatchwiseManagedStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep,
            ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, SoA, managed",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::
          timeStepWithRusanovPatchwiseManagedStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep,
            ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoSoA, managed",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::
          timeStepWithRusanovTaskgraphManagedStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep,
            ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, AoS, managed",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::
          timeStepWithRusanovTaskgraphManagedStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep,
            ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, SoA, managed",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::sycl::
          timeStepWithRusanovTaskgraphManagedStateless<
            SWE,
            SWE::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            SWE::NumberOfUnknowns,
            SWE::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep,
            ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, task-graph, AoSoA, managed",
      launchingThreads,
      device,
      localPatches
    );
#endif

#if defined(GPUOffloadingOMP)
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoS, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, SoA, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoSoA, copy",
      launchingThreads,
      device,
      localPatches
    );
#if defined(__NVCOMPILER_CUDA__) // __NVCC__
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoSoA, usm",
      launchingThreads,
      device,
      localPatches
    );
#endif

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoS, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, SoA, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseHeapStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoSoA, copy",
      launchingThreads,
      device,
      localPatches
    );
#if defined(__NVCOMPILER_CUDA__) // __NVCC__
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoSoA, usm",
      launchingThreads,
      device,
      localPatches
    );
#endif

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, volume-wise, AoS, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, volume-wise, SoA, copy",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::omp::timeStepWithRusanovVolumewiseStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, volume-wise, AoSoA, copy",
      launchingThreads,
      device,
      localPatches
    );
#endif

#if defined(GPUOffloadingCPP)
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, batched, AoSoA, usm",
      launchingThreads,
      device,
      localPatches
    );

    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoS, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, SoA, usm",
      launchingThreads,
      device,
      localPatches
    );
    assessKernel(
      wrapDeviceKernel<
        ::exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
          SWE,
          SWE::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          SWE::NumberOfUnknowns,
          SWE::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>>,
      "device(s) " + deviceString + ", stateless, patch-wise, AoSoA, usm",
      launchingThreads,
      device,
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
        << SWE::NumberOfFiniteVolumesPerAxisPerPatch
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
