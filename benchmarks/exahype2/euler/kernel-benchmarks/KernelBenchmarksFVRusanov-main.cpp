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

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

tarch::logging::Log _log("::");

double                             validMaxEigenvalue = 0;
double*                            validOutcome       = nullptr;
constexpr double                   TimeStamp          = 0.5;
constexpr double                   TimeStepSize       = 1e-6;
constexpr double                   CellSize           = 0.1;
constexpr double                   CellOffset         = 4.0;
tarch::timing::Measurement         timingComputeKernel;
tarch::multicore::BooleanSemaphore validateOutcomeSemaphore;

#if Dimensions == 2
constexpr int NumberOfInputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2) * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2)
                                     * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
                                      * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch;
#else
constexpr int NumberOfInputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2) * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2)
                                     * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
                                      * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch
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
    Q[i] = std::sin(1.0 * i / (NumberOfInputEntries)*tarch::la::PI);
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
  tarch::multicore::Lock lock(validateOutcomeSemaphore);
  if (validOutcome == nullptr) {
    validOutcome = new double[NumberOfOutputEntries];
    std::memcpy(validOutcome, Q, sizeof(double) * NumberOfOutputEntries);
    logInfo("storeOutcome(double*)", "bookmarked reference solution");
  }
  validMaxEigenvalue = maxEigenvalue;
}


/**
 * Validate data against pre-stored simulation outcome
 *
 * Works if and only if storeOutcome has been performed before.
 *
 * @return Tuple of maximum difference and total number of errors.
 */
std::tuple<double, int> validateOutcome(double* Q, int patchIndex, const double maxEigenvalue) {
  tarch::multicore::Lock lock(validateOutcomeSemaphore);
  int                    index         = 0;
  int                    errors        = 0;
  double                 maxDifference = 0.0;

  for (int i = 0; i < NumberOfOutputEntries; i++) {
    if (not tarch::la::equals(Q[i], validOutcome[i], Accuracy)) {
      errors++;
      if (patchIndex == 0) {
        std::cout.precision(16);
        std::cout << std::fixed << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i] << "!=" << validOutcome[i] << std::endl;
      }
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i] - validOutcome[i]));
    index++;
  }

  if (EvaluateMaximumEigenvalueAfterTimeStep and not tarch::la::equals(maxEigenvalue, validMaxEigenvalue, Accuracy)) {
    std::cout << std::fixed << " maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue[" << patchIndex << "]: " << maxEigenvalue << "!=" << validMaxEigenvalue << std::endl;
    errors++;
  }

  return {maxDifference, errors};
}


void reportRuntime(const std::string& kernelIdentificator, const tarch::timing::Measurement& timingKernelLaunch, int patches) {
  logInfo(
    "reportRuntime()",
    kernelIdentificator
      << "\t| " << timingKernelLaunch.getValue() << "\t| " << (timingKernelLaunch.getValue() / patches / NumberOfFiniteVolumesPerPatch / NumberOfSamples) << "\t| "
      << timingComputeKernel.getValue() << "\t| " << (timingComputeKernel.getValue() / patches / NumberOfFiniteVolumesPerPatch / NumberOfSamples) << "\t| "
      << timingKernelLaunch.toString() << "\t| " << timingComputeKernel.toString()
  );
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
  assertionEquals(device, tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapFunctors<
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    1,
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
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__ F
    ) -> void {
      if constexpr (EvaluateFlux) {
        repositories::InstanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
      }
    },
    [&](
      const double* __restrict__ Q,
      const double* __restrict__ deltaQ,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__ BTimesDeltaQ
    ) -> void {
      if constexpr (EvaluateNonconservativeProduct) {
        // repositories::InstanceOfFVRusanovSolver.nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S)
      -> void {
      if constexpr (EvaluateSource) {
        // repositories::InstanceOfFVRusanovSolver.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal) -> double {
      return repositories::InstanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal);
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
template <class TempDataEnumerator, ::peano4::utils::LoopParallelism loopParallelism>
void wrapBatchedHeapFunctorHostKernels(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, tarch::accelerator::Device::HostDevice);

  ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapFunctors<
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    1,
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
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__ F
    ) -> void {
      if constexpr (EvaluateFlux) {
        repositories::InstanceOfFVRusanovSolver.flux(Q, x, h, t, dt, normal, F);
      }
    },
    [&](
      const double* __restrict__ Q,
      const double* __restrict__ deltaQ,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double* __restrict__ BTimesDeltaQ
    ) -> void {
      if constexpr (EvaluateNonconservativeProduct) {
        // repositories::InstanceOfFVRusanovSolver.nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
      }
    },
    [&](const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S)
      -> void {
      if constexpr (EvaluateSource) {
        // repositories::InstanceOfFVRusanovSolver.sourceTerm(Q, x, h, t, dt, S);
      }
    },
    [&](const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal) -> double {
      return repositories::InstanceOfFVRusanovSolver.maxEigenvalue(Q, x, h, t, dt, normal);
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
template <void (*Function)(::exahype2::CellData&, tarch::timing::Measurement&, ::peano4::utils::LoopParallelism), ::peano4::utils::LoopParallelism loopParallelism>
void wrapStatelessHostKernel(int device, ::exahype2::CellData& patchData) {
  assertionEquals(device, tarch::accelerator::Device::HostDevice);
  Function(patchData, timingComputeKernel, loopParallelism);
}


template <void (*Function)(int, ::exahype2::CellData&, tarch::timing::Measurement&)>
void wrapDeviceKernel(int device, ::exahype2::CellData& patchData) {
  assertion(device != tarch::accelerator::Device::HostDevice);
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
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  auto assessKernel =
    [&](std::function<void(int device, ::exahype2::CellData& patchData)> kernel, const std::string& markerName, int launchingThreads, int device, int patches) -> void {
    timingComputeKernel.erase();
    tarch::timing::Measurement timingKernelLaunch;

    for (int j = 0; j < NumberOfSamples; j++) {
      parallelFor(launchingThread, launchingThreads) {
        ::exahype2::CellData patchData(patches);
        for (int i = 0; i < patches; i++) {
          patchData.QIn[i]        = tarch::allocateMemory(NumberOfInputEntries, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
          patchData.t[i]          = TimeStamp;
          patchData.dt[i]         = TimeStepSize;
          patchData.QOut[i]       = tarch::allocateMemory(NumberOfOutputEntries, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
          patchData.cellCentre[i] = tarch::la::Vector<Dimensions, double>(CellOffset + 0.5 * CellSize);
          patchData.cellSize[i]   = tarch::la::Vector<Dimensions, double>(CellSize);
          initInputData(patchData.QIn[i]);
        }

        tarch::timing::Watch watchKernelLaunch("::runBenchmarks", "assessKernel(...)", false);
        kernel(device, patchData);
        watchKernelLaunch.stop();
        timingKernelLaunch.setValue(watchKernelLaunch.getCalendarTime());

        if constexpr (Accuracy > 0.0) {
          int    errors        = 0;
          double maxDifference = 0.0;
          for (int i = 0; i < patches; i++) {
            storeOutcome(patchData.QOut[i], patchData.maxEigenvalue[0]);
            auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome(patchData.QOut[i], i, patchData.maxEigenvalue[i]);
            errors += errorsPerPatch;
            maxDifference = std::max(maxDifference, maxDifferencePerPatch);
          }

          if (errors > 0) {
            logError(
              "main()",
              "max difference of outcome from "
                << markerName << " for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")"
                << " for " << errors << " entries"
            );
            std::abort();
          }
        }

        for (int i = 0; i < patches; i++) {
          tarch::freeMemory(patchData.QIn[i], tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
          tarch::freeMemory(patchData.QOut[i], tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
        }
      }
      endParallelFor
    }

    reportRuntime(markerName, timingKernelLaunch, patches);
  };

  assessKernel(
    wrapPatchwiseHeapFunctorsHostKernel<::exahype2::enumerators::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::None>,
    "host, heap-functors, patch-wise, AoS, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapPatchwiseHeapFunctorsHostKernel<::exahype2::enumerators::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, heap-functors, patch-wise, AoS, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapPatchwiseHeapFunctorsHostKernel<::exahype2::enumerators::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::Subtasks>,
    "host, heap-functors, patch-wise, AoS, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapBatchedHeapFunctorHostKernels<::exahype2::enumerators::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::None>,
    "host, heap-functors, batched, AoS, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapBatchedHeapFunctorHostKernels<::exahype2::enumerators::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, heap-functors, batched, AoS, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapBatchedHeapFunctorHostKernels<::exahype2::enumerators::AoSLexicographicEnumerator, ::peano4::utils::LoopParallelism::Subtasks>,
    "host, heap-functors, batched, AoS, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSLexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::None>,
    "host, stateless, patch-wise, AoS, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSLexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, stateless, patch-wise, AoS, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSLexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::Subtasks>,
    "host, stateless, patch-wise, AoS, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::SoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::None>,
    "host, stateless, patch-wise, SoA, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::SoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, stateless, patch-wise, SoA, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::SoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::Subtasks>,
    "host, stateless, patch-wise, SoA, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::None>,
    "host, stateless, patch-wise, AoSoA, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, stateless, patch-wise, AoSoA, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovPatchwiseHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::Subtasks>,
    "host, stateless, patch-wise, AoSoA, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSLexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::None>,
    "host, stateless, batched, AoS, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSLexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, stateless, batched, AoS, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSLexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::Subtasks>,
    "host, stateless, batched, AoS, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::SoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::None>,
    "host, stateless, batched, SoA, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::SoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, stateless, batched, SoA, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::SoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::Subtasks>,
    "host, stateless, batched, SoA, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::None>,
    "host, stateless, batched, AoSoA, sequential",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::ParallelFor>,
    "host, stateless, batched, AoSoA, parallel-for",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );
  assessKernel(
    wrapStatelessHostKernel<
      ::exahype2::fv::rusanov::timeStepWithRusanovBatchedHeapStateless<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        EvaluateFlux,
        EvaluateNonconservativeProduct,
        EvaluateSource,
        EvaluateMaximumEigenvalueAfterTimeStep,
        ::exahype2::enumerators::AoSoALexicographicEnumerator>,
      ::peano4::utils::LoopParallelism::Subtasks>,
    "host, stateless, batched, AoSoA, subtasks",
    launchingThreads,
    ::tarch::accelerator::Device::HostDevice,
    numberOfPatches
  );

  int device = 0;

  /*#if defined(GPUOffloadingSYCL)
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoS, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, SoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoSoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoS, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, SoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoSoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, AoS, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, SoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, AoSoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoS, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, SoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoSoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoS, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, SoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoSoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, AoS, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, SoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, AoSoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoS, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, SoA, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoSoA, managed",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoS, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, SoA, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoSoA, managed",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, AoS, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, SoA, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_managed_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", task-graph, AoSoA, managed",
      launchingThreads,
      device,
      numberOfPatches
    );
  #endif

  #if defined(GPUOffloadingOMP)
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoS, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, SoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoSoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoS, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, SoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_copy_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoSoA, copy",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoS, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, SoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", batched, AoSoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );

    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSLexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoS, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::SoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, SoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
    assessKernel(
      wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_usm_static_calls<
        FVRusanovSolver,
        FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
        1,
        FVRusanovSolver::NumberOfUnknowns,
        FVRusanovSolver::NumberOfAuxiliaryVariables,
        exahype2::enumerators::AoSoALexicographicEnumerator>>,
      "device " + std::to_string(device) + ", patch-wise, AoSoA, usm",
      launchingThreads,
      device,
      numberOfPatches
    );
  #endif*/
}

int main(int argc, char** argv) {
  constexpr int ExitCodeSuccess      = 0;
  constexpr int ExitCodeInvalidBuild = 3;

  peano4::initParallelEnvironment(&argc, &argv);

  ::exahype2::setDefaultLogStatements();
  tarch::logging::CommandLineLogger::getInstance().setLogFormat(
    " ",   // const std::string& columnSeparator
    false, // bool logTimeStamp,
    true,  // bool logTimeStampHumanReadable,
    false, // bool logMachineName,
    false, // bool logThreadName,
    false, // bool logMessageType,
    false, // bool logTrace,
    ""     // const std::string& outputLogFileName
  );

  tarch::multicore::Core::getInstance().configure(tarch::multicore::Core::UseDefaultNumberOfThreads);
  tarch::accelerator::Device::getInstance().configure();

  exahype2::initNonCritialAssertionEnvironment();
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and tarch::multicore::Core::getInstance().getNumberOfThreads() <= 1) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  // peano4::runTestsAndBenchmarks();

  OTTER_INITIALISE();

  feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

  logInfo("main()", "Number of compute threads: " << tarch::multicore::Core::getInstance().getNumberOfThreads());
  logInfo("main()", "Number of GPU devices: " << tarch::accelerator::Device::getInstance().getNumberOfDevices());
  logInfo("main()", "Number of finite volumes per axis per patch: " << FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch);
  logInfo("main()", "Number of samples per measurement: " << NumberOfSamples);
  logInfo("main()", "Dimensions: " << Dimensions);

#if defined(SharedOMP)
#pragma omp parallel
  {
#pragma omp master
    {
#endif
      for (int launchingThreads = 1; launchingThreads <= MaxNumberOfLaunchingThreads; launchingThreads *= 2) {
        for (int numberOfPatches = launchingThreads; numberOfPatches <= MaxNumberOfPatchesToStudy; numberOfPatches *= 2) {
          std::stringstream msg;
          msg << "kernel-benchmarks-" << numberOfPatches << "-patches-" << tarch::multicore::Core::getInstance().getNumberOfThreads() << "-threads";
          OTTER_PHASE_SWITCH(msg.str().c_str());

          logInfo("main()", "===============================================================");
          logInfo("main()", "  Number of threads launching compute kernels: " << launchingThreads);
          logInfo("main()", "  Number of patches per thread/compute kernel launch: " << numberOfPatches);
          logInfo("main()", "===============================================================");

          runBenchmarks(numberOfPatches, launchingThreads);
        }
      }
#if defined(SharedOMP)
    }
  }
#endif


  if constexpr (Accuracy > 0.0) {
    logInfo("main()", "all kernels yield the same outcome up to a accuracy of " << Accuracy << " unless reported otherwise");
  } else {
    logInfo("main()", "no accuracy checks were performed");
  }

  OTTER_FINALISE();

  delete[] validOutcome;
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
