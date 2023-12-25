#include "KernelBenchmarksFVRusanov-main.h"

#include <fenv.h>
#pragma float_control(precise, on)
#pragma STDC FENV_ACCESS ON

#include "config.h"
#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"
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
#include "tarch/multicore/otter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "tasks/FVRusanovSolverEnclaveTask.h"
#include "toolbox/loadbalancing/loadbalancing.h"

using namespace benchmarks::exahype2::kernelbenchmarks;
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
static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

#if Dimensions == 2
static constexpr int NumberOfInputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
                                            * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize) * FVRusanovSolver::NumberOfUnknowns;
static constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
                                             * FVRusanovSolver::NumberOfUnknowns;
static constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch;
#else
static constexpr int NumberOfInputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
                                            * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize)
                                            * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2 * HaloSize) * FVRusanovSolver::NumberOfUnknowns;
static constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
                                             * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * FVRusanovSolver::NumberOfUnknowns;
static constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch
                                                     * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch;
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
std::tuple<double, int> validateOutcome(double* Q, int patchIndex, const double maxEigenvalue) {
  ::tarch::multicore::Lock lock(validateOutcomeSemaphore);
  int                      index         = 0;
  int                      errors        = 0;
  double                   maxDifference = 0.0;

  for (int i = 0; i < NumberOfOutputEntries; i++) {
    if (not ::tarch::la::equals(Q[i], validOutcome[i], Accuracy)) {
      errors++;
      std::cerr.precision(16);
      logError("validateOutcome()", std::fixed << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i] << "!=" << validOutcome[i]);
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i] - validOutcome[i]));
    index++;
  }

  if (not tarch::la::equals(maxEigenvalue, validMaxEigenvalue, Accuracy)) {
    std::cerr.precision(16);
    logError("validateOutcome()", std::fixed << " maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue[" << patchIndex << "]: " << maxEigenvalue << "!=" << validMaxEigenvalue);
    errors++;
  }

  return {maxDifference, errors};
}


void reportRuntime(const std::string& kernelIdentificator, const ::tarch::timing::Measurement& timingKernelLaunch, int patches) {
  std::stringstream ss;
  ss << std::string(150, '-') << "\n";
  ss << kernelIdentificator << ":\n\t";

  ss << timingComputeKernel.getValue() << " |\n\t";
  ss << (timingComputeKernel.getValue() / patches / NumberOfFiniteVolumesPerPatch) << " |\n\t";
  ss << timingComputeKernel.toString() << " |\n\t";

  ss << timingKernelLaunch.getValue() << " |\n\t";
  ss << (timingKernelLaunch.getValue() / patches / NumberOfFiniteVolumesPerPatch);
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
template <class TempDataEnumerator, ::peano4::utils::LoopParallelism loopParallelism>
void wrapPatchwiseHeapFunctorsHostKernel(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapFunctors<
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    HaloSize,
    FVRusanovSolver::NumberOfUnknowns,
    FVRusanovSolver::NumberOfAuxiliaryVariables,
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
        repositories::InstanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
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
        repositories::InstanceOfFVRusanovSolver.nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](
      const double* __restrict__ Q, const ::tarch::la::Vector<Dimensions, double>& x, const ::tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S
    ) -> void {
      if constexpr (EvaluateSource) {
        repositories::InstanceOfFVRusanovSolver.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](const double* __restrict__ Q, const ::tarch::la::Vector<Dimensions, double>& x, const ::tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal)
      -> double { return repositories::InstanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal); },
    timingComputeKernel,
    loopParallelism
  );
}


/**
 * Another wrapper.
 *
 * @see wrapPatchwiseHeapFunctorsHostKernel()
 */
template <class TempDataEnumerator, ::peano4::utils::LoopParallelism loopParallelism>
void wrapBatchedHeapFunctorHostKernels(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapFunctors<
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    HaloSize,
    FVRusanovSolver::NumberOfUnknowns,
    FVRusanovSolver::NumberOfAuxiliaryVariables,
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
        repositories::InstanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
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
        repositories::InstanceOfFVRusanovSolver.nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](
      const double* __restrict__ Q, const ::tarch::la::Vector<Dimensions, double>& x, const ::tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S
    ) -> void {
      if constexpr (EvaluateSource) {
        repositories::InstanceOfFVRusanovSolver.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](const double* __restrict__ Q, const ::tarch::la::Vector<Dimensions, double>& x, const ::tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal)
      -> double { return repositories::InstanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal); },
    timingComputeKernel,
    loopParallelism
  );
}


/**
 * Another wrapper.
 *
 * @see wrapPatchwiseHeapFunctorsHostKernel()
 */
template <class TempDataEnumerator, ::peano4::utils::LoopParallelism loopParallelism>
void wrapVolumewiseFunctorHostKernels(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseFunctors<
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    HaloSize,
    FVRusanovSolver::NumberOfUnknowns,
    FVRusanovSolver::NumberOfAuxiliaryVariables,
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
        repositories::InstanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
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
        repositories::InstanceOfFVRusanovSolver.nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](
      const double* __restrict__ Q, const ::tarch::la::Vector<Dimensions, double>& x, const ::tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S
    ) -> void {
      if constexpr (EvaluateSource) {
        repositories::InstanceOfFVRusanovSolver.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](const double* __restrict__ Q, const ::tarch::la::Vector<Dimensions, double>& x, const ::tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal)
      -> double { return repositories::InstanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal); },
    timingComputeKernel,
    loopParallelism
  );
}


/**
 * Wrapper around stateless kernel invocations
 *
 * @see wrapPatchwiseHeapFunctorsHostKernel()
 */
template <void (*Function)(::exahype2::CellData&, ::tarch::timing::Measurement&, ::peano4::utils::LoopParallelism), ::peano4::utils::LoopParallelism loopParallelism>
void wrapStatelessHostKernel(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, ::tarch::accelerator::Device::HostDevice);
  Function(patchData, timingComputeKernel, loopParallelism);
}


template <void (*Function)(int, ::exahype2::CellData&, ::tarch::timing::Measurement&)>
void wrapDeviceKernel(int device, ::exahype2::CellData& patchData) {
  assertion(device != ::tarch::accelerator::Device::HostDevice);
  Function(device, patchData, timingComputeKernel);
}


/**
 * Run the benchmark for one particular number of patches
 *
 * This operation benchmarks exclusively the host performance. This happens in
 * two steps: We first assess the baseline performance, i.e., the sequential kernel
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
    [&](std::function<void(int device, ::exahype2::CellData& patchData)> kernel, const std::string& markerName, int launchingThreads, int device, int patches) -> void {
    timingComputeKernel.erase();
    ::tarch::timing::Measurement timingKernelLaunch;

    // TODO: Does the number of samples change the outcome of the solution?
    for (int j = 0; j < NumberOfSamples; j++) {
      parallelFor(launchingThread, launchingThreads) {
        ::exahype2::CellData patchData(patches);
        for (int i = 0; i < patches; i++) {
          patchData.QIn[i]           = ::tarch::allocateMemory(NumberOfInputEntries, ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
          patchData.t[i]             = TimeStamp;
          patchData.dt[i]            = TimeStepSize;
          patchData.QOut[i]          = ::tarch::allocateMemory(NumberOfOutputEntries, ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
          patchData.cellCentre[i]    = ::tarch::la::Vector<Dimensions, double>(CellOffset + 0.5 * CellSize);
          patchData.cellSize[i]      = ::tarch::la::Vector<Dimensions, double>(CellSize);
          patchData.maxEigenvalue[i] = 0.0;
          initInputData(patchData.QIn[i]);
          std::memset(patchData.QOut[i], 0.0, NumberOfOutputEntries * sizeof(double));
        }

        ::tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
        kernel(device, patchData);
        watchKernelLaunch.stop();
        timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());

        if constexpr (Accuracy > 0.0) {
          int    errors        = 0;
          double maxDifference = 0.0;
          for (int i = 0; i < patches; i++) {
            storeOutcome(patchData.QOut[i], patchData.maxEigenvalue[i]);
            auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome(patchData.QOut[i], i, patchData.maxEigenvalue[i]);
            errors += errorsPerPatch;
            maxDifference = std::max(maxDifference, maxDifferencePerPatch);
          }

          if (errors > 0) {
            logError(
              "runBenchmarks()",
              "max difference of outcome from "
                << markerName << " for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")"
                << " for " << errors << " entries"
            );
            std::abort();
          }
        }

        for (int i = 0; i < patches; i++) {
          ::tarch::freeMemory(patchData.QIn[i], ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
          ::tarch::freeMemory(patchData.QOut[i], ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
        }
      }
      endParallelFor
    }

    if (::tarch::mpi::Rank::getInstance().getRank() == ::tarch::mpi::Rank::getGlobalMasterRank()) {
      reportRuntime(markerName, timingKernelLaunch, patches);
    }
  };

  // Kernel launches
  const int rank              = ::tarch::mpi::Rank::getInstance().getRank();
  const int device            = rank;
  const int numberOfRanks     = ::tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int patchesPerProcess = numberOfPatches / numberOfRanks;
  const int remainder         = numberOfPatches % numberOfRanks;
  const int startPatch        = rank * patchesPerProcess;
  const int endPatch          = startPatch + patchesPerProcess + (rank == numberOfRanks - 1 ? remainder : 0);
  const int localPatches      = endPatch - startPatch;

  // Headers
  std::stringstream ss;
  ss << std::left;
  ss << std::string(150, '-') << "\n";
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

  if constexpr (EvaluateHostKernels) {
    assessKernel(
      wrapBatchedHeapFunctorHostKernels<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::None>,
      "host, functors, batched, AoS, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapBatchedHeapFunctorHostKernels<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, functors, batched, AoS, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapBatchedHeapFunctorHostKernels<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::Subtasks>,
      "host, functors, batched, AoS, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );

    assessKernel(
      wrapPatchwiseHeapFunctorsHostKernel<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::None>,
      "host, functors, patch-wise, AoS, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapPatchwiseHeapFunctorsHostKernel<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, functors, patch-wise, AoS, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapPatchwiseHeapFunctorsHostKernel<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::Subtasks>,
      "host, functors, patch-wise, AoS, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );

    assessKernel(
      wrapVolumewiseFunctorHostKernels<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::None>,
      "host, functors, volume-wise, AoS, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapVolumewiseFunctorHostKernels<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, functors, volume-wise, AoS, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapVolumewiseFunctorHostKernels<::exahype2::enumerator::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::Subtasks>,
      "host, functors, volume-wise, AoS, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );

    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, batched, AoS, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, batched, AoS, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, batched, AoS, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, batched, SoA, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, batched, SoA, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, batched, SoA, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, batched, AoSoA, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, batched, AoSoA, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, batched, AoSoA, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    if constexpr (not Accuracy > 0.0) {
      assessKernel(
        wrapStatelessHostKernel<
          ::exahype2::fv::rusanov::timeStepWithRusanovBatchedInsituStateless<
            FVRusanovSolver,
            FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            FVRusanovSolver::NumberOfUnknowns,
            FVRusanovSolver::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep>,
          ::peano4::utils::LoopParallelism::None>,
        "host, stateless, batched, insitu, sequential",
        launchingThreads,
        ::tarch::accelerator::Device::HostDevice,
        localPatches
      );
    }

    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, patch-wise, AoS, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, patch-wise, AoS, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, patch-wise, AoS, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, patch-wise, SoA, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, patch-wise, SoA, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, patch-wise, SoA, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, patch-wise, AoSoA, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, patch-wise, AoSoA, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, patch-wise, AoSoA, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    if constexpr (not Accuracy > 0.0) {
      assessKernel(
        wrapStatelessHostKernel<
          ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseInsituStateless<
            FVRusanovSolver,
            FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
            HaloSize,
            FVRusanovSolver::NumberOfUnknowns,
            FVRusanovSolver::NumberOfAuxiliaryVariables,
            EvaluateFlux,
            EvaluateNonconservativeProduct,
            EvaluateSource,
            EvaluateMaximumEigenvalueAfterTimeStep>,
          ::peano4::utils::LoopParallelism::None>,
        "host, stateless, patch-wise, insitu, sequential",
        launchingThreads,
        ::tarch::accelerator::Device::HostDevice,
        localPatches
      );
    }

    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, volume-wise, AoS, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, volume-wise, AoS, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSLexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, volume-wise, AoS, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, volume-wise, SoA, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, volume-wise, SoA, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::SoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, volume-wise, SoA, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::None>,
      "host, stateless, volume-wise, AoSoA, sequential",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::ParallelFor>,
      "host, stateless, volume-wise, AoSoA, parallel-for",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
    assessKernel(
      wrapStatelessHostKernel<
        ::exahype2::fv::rusanov::timeStepWithRusanovVolumewiseStateless<
          FVRusanovSolver,
          FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
          HaloSize,
          FVRusanovSolver::NumberOfUnknowns,
          FVRusanovSolver::NumberOfAuxiliaryVariables,
          EvaluateFlux,
          EvaluateNonconservativeProduct,
          EvaluateSource,
          EvaluateMaximumEigenvalueAfterTimeStep,
          ::exahype2::enumerator::AoSoALexicographicEnumerator>,
        ::peano4::utils::LoopParallelism::Subtasks>,
      "host, stateless, volume-wise, AoSoA, subtasks",
      launchingThreads,
      ::tarch::accelerator::Device::HostDevice,
      localPatches
    );
  }

  if constexpr (EvaluateDeviceKernels) {
#if defined(GPUOffloadingSYCL)
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovBatchedManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovPatchwiseManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanovTaskgraphManagedStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseCopyStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovVolumewiseStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovVolumewiseStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanovVolumewiseStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
      wrapDeviceKernel<::exahype2::fv::rusanov::cpp::timeStepWithRusanovPatchwiseUSMStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        HaloSize,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
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
  ::exahype2::setDefaultLogStatements();
  ::tarch::logging::CommandLineLogger::getInstance().setLogFormat(
    " ",   // const std::string& columnSeparator
    false, // bool logTimeStamp,
    true,  // bool logTimeStampHumanReadable,
    false, // bool logMachineName,
    false, // bool logThreadName,
    false, // bool logMessageType,
    false, // bool logTrace,
    ""     // const std::string& outputLogFileName
  );
  ::tarch::multicore::Core::getInstance().configure(tarch::multicore::Core::UseDefaultNumberOfThreads);
  ::tarch::accelerator::Device::getInstance().configure();
  ::exahype2::initNonCritialAssertionEnvironment();
  ::tarch::multicore::initSmartMPI();
  ::peano4::fillLookupTables();
  // if (::tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and ::tarch::multicore::Core::getInstance().getNumberOfThreads() <= 1) {
  //   logError("main()", "MPI runs without multithreading are not supported currently.");
  //   return EXIT_FAILURE;
  // }
  ::peano4::runTestsAndBenchmarks();
  OTTER_INITIALISE();

  if (EnableFPE) {
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
  }

  if (::tarch::mpi::Rank::getInstance().getRank() == ::tarch::mpi::Rank::getGlobalMasterRank()) {
    logInfo("main()", "Dimensions: " << Dimensions);
    logInfo("main()", "Number of threads launching compute kernels: " << NumberOfLaunchingThreads);
    logInfo("main()", "Number of patches per thread/compute kernel launch to study: " << toString(NumberOfPatchesToStudy));
    logInfo("main()", "Number of compute threads: " << ::tarch::multicore::Core::getInstance().getNumberOfThreads());
    logInfo("main()", "Number of MPI ranks: " << ::tarch::mpi::Rank::getInstance().getNumberOfRanks());
    logInfo("main()", "Number of GPU devices: " << ::tarch::accelerator::Device::getInstance().getNumberOfDevices());
    logInfo("main()", "Number of finite volumes per axis per patch: " << FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch);
    logInfo("main()", "Number of samples per measurement: " << NumberOfSamples);
    logInfo("main()", "Evaluate max. eigenvalue (reduction step): " << EvaluateMaximumEigenvalueAfterTimeStep);
    if constexpr (EnableFPE) {
      logInfo("main()", "Floating-point exception handler enabled");
    }
    if constexpr (Accuracy > 0.0) {
      logInfo("main()", "Performing accuracy checks with precision: " << Accuracy);
    }
#if defined(GPUOffloadingSYCL)
    logInfo("main()", "Set SYCL_DEVICE_FILTER=gpu when using SYCL on the device");
    logInfo("main()", "Set SYCL_PI_TRACE=2 in case of runtime errors");
#endif
  }

  for (int n = 0; n < NumberOfPatchesToStudy.size(); n++) {
    std::stringstream msg;
    msg << "kernel-benchmarks-" << NumberOfPatchesToStudy[n] << "-patches-" << ::tarch::multicore::Core::getInstance().getNumberOfThreads() << "-threads";
    OTTER_PHASE_SWITCH(msg.str().c_str());
    runBenchmarks(NumberOfPatchesToStudy[n], NumberOfLaunchingThreads);
  }

  if constexpr (Accuracy > 0.0) {
    logInfo("main()", "all kernels yield the same outcome up to a accuracy of " << Accuracy << " unless reported otherwise");
  } else {
    logInfo("main()", "no accuracy checks were performed");
  }

  OTTER_FINALISE();
  delete[] validOutcome;
  ::peano4::shutdownParallelEnvironment();
  return EXIT_SUCCESS;
}
