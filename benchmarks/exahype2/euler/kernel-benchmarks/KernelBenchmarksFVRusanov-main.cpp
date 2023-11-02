#include "KernelBenchmarksFVRusanov-main.h"

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
#include "tasks/FVRusanovSolverEnclaveTask.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/logging/CommandLineLogger.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/otter.h"
#include "tarch/multicore/multicore.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "toolbox/loadbalancing/loadbalancing.h"


using namespace benchmarks::exahype2::kernelbenchmarks;

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

tarch::logging::Log _log("::");


double        validMaxEigenvalue = 0;
double*       validOutcome       = nullptr;
//double        minTime            = 0;
//constexpr int N                  = 33554432 * 2;
constexpr double TimeStamp    = 0.5;
constexpr double TimeStepSize = 1e-6;
constexpr double CellSize     = 0.1;
constexpr double CellOffset   = 4.0;


#if Dimensions == 2
constexpr int NumberOfInputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2)
                                     * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2)
                                     * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
                                      * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
                                      * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch
                                              * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch;
#else
constexpr int NumberOfInputEntries
  = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2)
    * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2)
    * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfOutputEntries
  = (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0)
    * (FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch + 0) * FVRusanovSolver::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch
                                              * FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch
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
  /*for (int i=0; i<NumberOfInputEntries/FVRusanovSolver::NumberOfUnknowns; i++) {
    Q[i*FVRusanovSolver::NumberOfUnknowns+0] = 1.0;
    Q[i*FVRusanovSolver::NumberOfUnknowns+1] = std::sin( 1.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*FVRusanovSolver::NumberOfUnknowns+2] = std::sin( 2.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    #if Dimensions == 3
      Q[i*FVRusanovSolver::NumberOfUnknowns+3] = std::sin( 3.0 * i / (NumberOfInputEntries) * tarch::la::PI );
      Q[i*FVRusanovSolver::NumberOfUnknowns+4] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
    #else
      Q[i*FVRusanovSolver::NumberOfUnknowns+3] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
    #endif
  }*/
  for (int i = 0; i < NumberOfInputEntries; i++) {
    Q[i] = std::sin(1.0 * i / (NumberOfInputEntries)*tarch::la::PI);
  }
}

tarch::multicore::BooleanSemaphore  validateOutcomeSemaphore;

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
std::tuple<double, int> validateOutcome(
  double*       Q,
  int           patchIndex,
  const double  maxEigenvalue,
  bool          reducesEigenvalue
) {
  tarch::multicore::Lock lock(validateOutcomeSemaphore);
  int    index         = 0;
  int    errors        = 0;
  double maxDifference = 0.0;
  for (int i = 0; i < NumberOfOutputEntries; i++) {
    if (not tarch::la::equals(Q[i], validOutcome[i], Accuracy)) {
      errors++;
      if (patchIndex == 0) {
        std::cout.precision(16);
        std::cout
          << std::fixed << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i] << "!=" << validOutcome[i]
          << std::endl;
      }
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i] - validOutcome[i]));
    index++;
  }
  if (
    reducesEigenvalue
    and
    not tarch::la::equals(maxEigenvalue, validMaxEigenvalue, Accuracy)
  ) {
    std::cout
      << std::fixed << " maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue[" << patchIndex
      << "]: " << maxEigenvalue << "!=" << validMaxEigenvalue << std::endl;
    errors++;
  }

  return {maxDifference, errors};
}


void reportRuntime(
  const std::string&                 kernelIdentificator,
  const tarch::timing::Measurement&  timingOneKernel,
  const tarch::timing::Measurement&  timingWholeKernel,
  int                                patches,
  int                                launchingThreads
) {
  logInfo(
    "reportRuntime()",
    kernelIdentificator <<
    "\t| " << timingOneKernel.getValue() <<
    "\t| " << (timingOneKernel.getValue() / patches / NumberOfFiniteVolumesPerPatch / NumberOfSamples) <<
    "\t| " << timingWholeKernel.getValue() <<
    "\t| " << (timingWholeKernel.getValue() / patches / NumberOfFiniteVolumesPerPatch / NumberOfSamples / launchingThreads) <<
    "\t| " << timingOneKernel.toString() <<
    "\t| " << timingWholeKernel.toString()
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
 * interface, i.e. which are not patch data or the boolean, have to be
 * mapped onto template arguments.
 *
 * Here, we use the functor-based generic kernel implementation.
 */
template <::peano4::utils::LoopParallelism  loopParallelism>
void wrapHostKernelWithFunctorPatchWise(
  int                               device,
  ::exahype2::CellData &            patchData,
  bool                              evaluateMaximumEigenvalueAfterTimeStep
) {
  assertionEquals( device, tarch::accelerator::Device::HostDevice );

  ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_functors(
    patchData,
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    1,
    FVRusanovSolver::NumberOfUnknowns,
    FVRusanovSolver::NumberOfAuxiliaryVariables,
    true ,
    false ,
    false ,
    evaluateMaximumEigenvalueAfterTimeStep ,
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F // F[4]
    )->void {
      repositories::InstanceOfFVRusanovSolver.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    },
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[4]
    )->void {},
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S // S[4]
    )->void {},
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
    )->double {
      return repositories::InstanceOfFVRusanovSolver.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
    }
  );
}


/**
 * Another wrapper.
 *
 * @see wrapHostKernelWithFunctorPatchWise()
 */
template <::peano4::utils::LoopParallelism  loopParallelism>
void wrapHostKernelWithFunctorBatched(
  int                               device,
  ::exahype2::CellData &            patchData,
  bool                              evaluateMaximumEigenvalueAfterTimeStep
) {
  assertionEquals( device, tarch::accelerator::Device::HostDevice );

  ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_functors(
    patchData,
    FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch,
    1,
    FVRusanovSolver::NumberOfUnknowns,
    FVRusanovSolver::NumberOfAuxiliaryVariables,
    true ,
    false ,
    false ,
    evaluateMaximumEigenvalueAfterTimeStep ,
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F // F[4]
    )->void {
      repositories::InstanceOfFVRusanovSolver.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    },
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[4]
    )->void {},
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S // S[4]
    )->void {},
    [&](
      [[maybe_unused]] const double* __restrict__                   Q, // Q[4+0]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
    )->double {
      return repositories::InstanceOfFVRusanovSolver.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
    }
  );
}


/**
 * Wrapper around stateless kernel invocations
 *
 * @see wrapHostKernelWithFunctorPatchWise()
 */
template <
  void (*Function)(
    ::exahype2::CellData&   patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep,
    peano4::utils::LoopParallelism   loopParallelism
  ),
  ::peano4::utils::LoopParallelism  loopParallelism
>
void wrapHostKernelWithStaticCalls(
  int                               device,
  ::exahype2::CellData &            patchData,
  bool                              evaluateMaximumEigenvalueAfterTimeStep
) {
  assertionEquals( device, tarch::accelerator::Device::HostDevice );

  Function(
    patchData,
    true ,
    false ,
    false ,
    evaluateMaximumEigenvalueAfterTimeStep,
    loopParallelism
  );
}


template <void (*Function)(
  int                     chosenDevice,
  ::exahype2::CellData&   patchData,
  bool                    evaluateFlux,
  bool                    evaluateNonconservativeProduct,
  bool                    evaluateSource,
  bool                    evaluateMaximumEigenvalueAfterTimeStep,
  tarch::timing::Measurement&  measurement
)>
void wrapDeviceKernel(
  int                               device,
  ::exahype2::CellData &            patchData,
  bool                              evaluateMaximumEigenvalueAfterTimeStep
) {
  assertion( device!=tarch::accelerator::Device::HostDevice );

  static tarch::timing::Measurement timingComputeKernel;
  Function(
    device,
    patchData,
    true ,
    false ,
    false ,
    evaluateMaximumEigenvalueAfterTimeStep,
    timingComputeKernel
  );
  //if (timingComputeKernel.getNumberOfMeasurements()==NumberOfSamples) {
    if (timingComputeKernel.getValue() != 0.0) {
      logInfo( "wrapDeviceKernel(...)", "core compute kernel time: " << timingComputeKernel.getValue() << " " << timingComputeKernel.toString() );
      timingComputeKernel.erase();
    }  
  //}
}


/**
 * Run the benchmark for one particular number of patches
 *
 * This operation benchmarks exclusively the host performance. This happens in
 * two steps: We first assess the baseline performance, i.e. the normal kernel
 * call, and then we look if the stateless optimisation pays off.
 *
 * These two steps are ran on multiple threads. In OpenMP for example, there is
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
  int counter = 0;

  /**
   * Run a benchmark on the host
   *
   * It is important that we assess the version with the eigenvalue reduction
   * first, as the very first run will determine our reference solution.  So
   * we need a valid eigenvalue there.
   */
  auto assessKernel =
    [&](
      std::function<void(
        int                               device,
        ::exahype2::CellData &            patchData,
        bool                              evaluateMaximumEigenvalueAfterTimeStep
      )>                 kernel,
      const std::string& markerName,
      int                launchingThreads,
      int                device,
      int                patches
    ) -> void {
      for (int reduceMaxTimeStep=0; reduceMaxTimeStep<2; reduceMaxTimeStep++) {
        tarch::timing::Measurement timingWholeBenchmark;
        tarch::timing::Measurement timingOneKernel;

        for (int j=0; j<NumberOfSamples; j++) {
          tarch::timing::Watch watchWholeBenchmark("::runBenchmarks", "assessKernel(...)", false);

          parallelFor(launchingThread, launchingThreads) {
            ::exahype2::CellData patchData(patches);
            for (int i = 0; i < patches; i++) {
              patchData.QIn[i]        = tarch::allocateMemory( NumberOfInputEntries, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
              patchData.t[i]          = TimeStamp;
              patchData.dt[i]         = TimeStepSize;
              patchData.QOut[i]       = tarch::allocateMemory( NumberOfOutputEntries, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
              patchData.cellCentre[i] = tarch::la::Vector<Dimensions, double>(CellOffset + 0.5 * CellSize);
              patchData.cellSize[i]   = tarch::la::Vector<Dimensions, double>(CellSize);
              initInputData(patchData.QIn[i]);
            }

            tarch::timing::Watch watchOneKernel("::runBenchmarks", "assessKernel(...)", false);
            kernel(
              device,
              patchData,
              reduceMaxTimeStep==0
            );
            watchOneKernel.stop();
            if (launchingThread==0) {
              timingOneKernel.setValue( watchOneKernel.getCalendarTime() );
            }
            if constexpr (Accuracy > 0.0) {
              int    errors        = 0;
              double maxDifference = 0.0;
              for (int i = 0; i < patches; i++) {
				if (counter == 0 and reduceMaxTimeStep == 0) {
                  storeOutcome(patchData.QOut[i], patchData.maxEigenvalue[0]);
			      counter++;
				}
                auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome(
                  patchData.QOut[i], i, patchData.maxEigenvalue[i], reduceMaxTimeStep==0
                );
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
              tarch::freeMemory(patchData.QIn[i],tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
              tarch::freeMemory(patchData.QOut[i],tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
            }
          } endParallelFor
          watchWholeBenchmark.stop();
          timingWholeBenchmark.setValue( watchWholeBenchmark.getCalendarTime() );
        }

        std::string reportedMarkerName = markerName;
        if (reduceMaxTimeStep==0) {
          reportedMarkerName += ", with reduction";
        }
        else {
          reportedMarkerName += ", no reduction";
        }
        reportRuntime( reportedMarkerName, timingOneKernel, timingWholeBenchmark, patches, launchingThreads );
      }
  };

  assessKernel( wrapHostKernelWithFunctorPatchWise<::peano4::utils::LoopParallelism::None>,        "host, functors, patch-wise, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithFunctorPatchWise<::peano4::utils::LoopParallelism::ParallelFor>, "host, functors, patch-wise, parallel for  ", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithFunctorPatchWise<::peano4::utils::LoopParallelism::Subtasks>,    "host, functors, patch-wise, subtasks      ", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithFunctorBatched<::peano4::utils::LoopParallelism::None>,          "host, functors, batched, no parallelism   ", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithFunctorBatched<::peano4::utils::LoopParallelism::ParallelFor>,   "host, functors, batched, parallel for     ", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithFunctorBatched<::peano4::utils::LoopParallelism::Subtasks>,      "host, functors, batched, subtasks         ", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator>,::peano4::utils::LoopParallelism::None>,        "host, static, patch-wise, AoS, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator>,::peano4::utils::LoopParallelism::ParallelFor>, "host, static, patch-wise, AoS, parallel for",   launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator>,::peano4::utils::LoopParallelism::Subtasks>,    "host, static, patch-wise, AoS, subtasks",       launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator>,::peano4::utils::LoopParallelism::None>,        "host, static, patch-wise, SoA, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator>,::peano4::utils::LoopParallelism::ParallelFor>, "host, static, patch-wise, SoA, parallel for",   launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator>,::peano4::utils::LoopParallelism::Subtasks>,    "host, static, patch-wise, SoA, subtasks",       launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator>,::peano4::utils::LoopParallelism::None>,        "host, static, patch-wise, AoSoA, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator>,::peano4::utils::LoopParallelism::ParallelFor>, "host, static, patch-wise, AoSoA, parallel for",   launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator>,::peano4::utils::LoopParallelism::Subtasks>,    "host, static, patch-wise, AoSoA, subtasks",       launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator>,::peano4::utils::LoopParallelism::None>,        "host, static, batched, AoS, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator>,::peano4::utils::LoopParallelism::ParallelFor>, "host, static, batched, AoS, parallel for",   launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator>,::peano4::utils::LoopParallelism::Subtasks>,    "host, static, batched, AoS, subtasks",       launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator>,::peano4::utils::LoopParallelism::None>,        "host, static, batched, SoA, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator>,::peano4::utils::LoopParallelism::ParallelFor>, "host, static, batched, SoA, parallel for",   launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator>,::peano4::utils::LoopParallelism::Subtasks>,    "host, static, batched, SoA, subtasks",       launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator>,::peano4::utils::LoopParallelism::None>,        "host, static, batched, AoSoA, no parallelism", launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator>,::peano4::utils::LoopParallelism::ParallelFor>, "host, static, batched, AoSoA, parallel for",   launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );
  assessKernel( wrapHostKernelWithStaticCalls<::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator>,::peano4::utils::LoopParallelism::Subtasks>,    "host, static, batched, AoSoA, subtasks",       launchingThreads, tarch::accelerator::Device::HostDevice, numberOfPatches );

  for (int device=0; device<tarch::accelerator::Device::getInstance().getNumberOfDevices(); device++) {
    #if defined(GPUOffloadingSYCL)
	if (numberOfPatches < 256 && Dimensions == 2 && launchingThreads == 1) {
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, AoS, usm",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, SoA, usm",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", batched, AoSoA, usm", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, AoS, usm",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, SoA, usm",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", patch-wise, AoSoA, usm", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", task-graph, AoS, usm",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", task-graph, SoA, usm",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_usm_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", task-graph, AoSoA, usm", launchingThreads, device, numberOfPatches );
}
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, AoS, copy",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, SoA, copy",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", batched, AoSoA, copy", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, AoS, copy",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, SoA, copy",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", patch-wise, AoSoA, copy", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", task-graph, AoS, copy",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", task-graph, SoA, copy",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_copy_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", task-graph, AoSoA, copy", launchingThreads, device, numberOfPatches ); 

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, AoS, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, SoA, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", batched, AoSoA, managed", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, AoS, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, SoA, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", patch-wise, AoSoA, managed", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", task-graph, AoS, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", task-graph, SoA, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", task-graph, AoSoA, managed", launchingThreads, device, numberOfPatches );

    #endif

    #if defined(GPUOffloadingOMP)
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, AoS, map",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, SoA, map",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", batched, AoSoA, map", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, AoS, map",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, SoA, map",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", patch-wise, AoSoA, map", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, AoS, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", batched, SoA, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", batched, AoSoA, managed", launchingThreads, device, numberOfPatches );

      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSLexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, AoS, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::SoALexicographicEnumerator> >,     "device " + std::to_string(device) + ", patch-wise, SoA, managed",   launchingThreads, device, numberOfPatches );
      assessKernel( wrapDeviceKernel<::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls< FVRusanovSolver, FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch, 1, FVRusanovSolver::NumberOfUnknowns, FVRusanovSolver::NumberOfAuxiliaryVariables, exahype2::enumerator::AoSoALexicographicEnumerator> >,   "device " + std::to_string(device) + ", patch-wise, AoSoA, managed", launchingThreads, device, numberOfPatches );
    #endif
  }
}


void runStreamTriadOnDevice(int device) {
  #if defined(GPUOffloadingOMP)
  std::vector<double> timings;
  double*             a = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Heap);
  double*             b = tarch::allocateMemory<double>(N, tarch::MemoryLocation::Heap);
#pragma omp target enter data       map(alloc : a[0 : N], b[0 : N])
  for (unsigned int k = 0; k < EndTime; k++) {
    tarch::timing::Watch watch("::runBenchmarksOnDevice", "measureBandwidth", false);
// #pragma omp target loop
#pragma omp target teams distribute parallel for simd
    for (int i = 0; i < N; i++) {
      b[i] = a[i];
    }
    watch.stop();
    timings.push_back(watch.getCalendarTime());
  }
#pragma omp target exit data map(release : a[0 : N], b[0 : N])
  tarch::freeMemory(a, tarch::MemoryLocation::Heap);
  tarch::freeMemory(b, tarch::MemoryLocation::Heap);
  minTime                = *std::minmax_element(timings.begin() + 1, timings.end()).first;
  const double bandwidth = 1.0E-9 * 2 * sizeof(double) * N / minTime;
  logInfo(
    "main()",
    "achievable bandwidth on device " << device << ": " << bandwidth << " [GB/s], " << minTime << " / " << (minTime / N)
  );

  tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
#endif
}


int main(int argc, char** argv) {
  constexpr int ExitCodeSuccess          = 0;
  constexpr int ExitCodeInvalidBuild     = 3;

  peano4::initParallelEnvironment(&argc, &argv);

  ::exahype2::setDefaultLogStatements();
  tarch::logging::CommandLineLogger::getInstance().setLogFormat(
    " ",       // const std::string& columnSeparator
    false,     // bool logTimeStamp,
    true,      // bool logTimeStampHumanReadable,
    false,     // bool logMachineName,
    false,     // bool logThreadName,
    false,     // bool logMessageType,
    false,     // bool logTrace,
    ""         // const std::string&  outputLogFileName
  );

  tarch::multicore::Core::getInstance().configure( tarch::multicore::Core::UseDefaultNumberOfThreads );
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

  logInfo("main()", "Number of threads: " << tarch::multicore::Core::getInstance().getNumberOfThreads());
  logInfo("main()", "Number of GPU devices: " << tarch::accelerator::Device::getInstance().getNumberOfDevices());
  logInfo(
    "main()", "Number of finite volumes per axis per patch: " << FVRusanovSolver::NumberOfFiniteVolumesPerAxisPerPatch
  );
  logInfo("main()", "Dimensions: " << Dimensions);
  logInfo("main()", "Number of samples per measurement: " << NumberOfSamples);

  #if defined(SharedOMP) or defined(GPUOffloadingOMP)
  #pragma omp parallel
  {
  #pragma omp master
  {
  #endif

  for (int device=0; device<tarch::accelerator::Device::getInstance().getNumberOfDevices(); device++) {
    runStreamTriadOnDevice(device);
  }


  for (int launchingThreads=1; launchingThreads<=/*tarch::multicore::Core::getInstance().getNumberOfThreads()*/4; launchingThreads*=2) {
    for (int numberOfPatches = launchingThreads; numberOfPatches <= MaxPatches; numberOfPatches *= 2) {
      std::stringstream msg;
      msg << "kernel-benchmarks-" << numberOfPatches << "-patches-"
          << tarch::multicore::Core::getInstance().getNumberOfThreads() << "-threads";
      OTTER_PHASE_SWITCH(msg.str().c_str());

      logInfo("main()", "===============================================================" );
      logInfo("main()", "  Number of threads launching compute kernels: " << launchingThreads);
      logInfo("main()", "  Number of patches per thread/compute kernel launch: " << numberOfPatches);
      logInfo("main()", "===============================================================" );

      runBenchmarks(numberOfPatches, launchingThreads);
    }
  }

  #if defined(SharedOMP) or defined(GPUOffloadingOMP)
  }}
  #endif

  if constexpr (Accuracy > 0.0) {
    logInfo("main()", "all kernels yield the same outcome up to a accuracy of " << Accuracy << " unless reported otherwise" );
  } else {
    logInfo("main()", "no accuracy checks were performed");
  }

  OTTER_FINALISE();

  delete[] validOutcome;
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
