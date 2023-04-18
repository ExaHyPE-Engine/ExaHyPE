#include "CCZ4FVRusanovKernelBenchmarks-main.h"
#include "Constants.h"

#include "observers/CreateGrid.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"

#include "repositories/DataRepository.h"
#include "repositories/SolverRepository.h"
#include "repositories/StepRepository.h"

#include "tasks/CCZ4FVRusanovEnclaveTask.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"
#include "tarch/timing/Watch.h"
#include "tarch/timing/Measurement.h"

#include "peano4/peano.h"

#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"

#include "toolbox/loadbalancing/loadbalancing.h"

#include "config.h"

#ifdef UseFenv
#include "tarch/FenvWrapper.h"
#endif

#if defined(SharedTBB)
#include <tbb/task_group.h>
#endif

using namespace benchmarks::exahype2::ccz4;

static_assert(Accuracy >= std::numeric_limits<double>::epsilon() || Accuracy == 0.0);

tarch::logging::Log _log("::");

tarch::timing::Measurement gridConstructionMeasurement;
tarch::timing::Measurement timeStepMeasurement;
tarch::timing::Measurement plotMeasurement;

double validMaxEigenvalue = 0;
double* validOutcome = nullptr;

constexpr int NumberOfInputEntries  = (CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2)*(CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2)*(CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2) * CCZ4FVRusanov::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0)*(CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0)*(CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0) * CCZ4FVRusanov::NumberOfUnknowns;
constexpr int NumberOfFiniteVolumesPerPatch = CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch*CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch*CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch;

/**
 * Set input data
 *
 * We really don't care if this makes any sense. Just make the data reasonably
 * smooth and ensure that all is positive, as there's for example density among
 * the values.
 */
void initInputData( double* Q ) {
  for (int i=0; i<NumberOfInputEntries; i++) {
    Q[i] = std::sin( 1.0 * i / (NumberOfInputEntries) * tarch::la::PI );
  }
}

/**
 * Make a persistent snapshot of a solution and assume, from hereon, that
 * this snapshot is the valid data. You can call this routine as often as
 * you want. Only the very first call will trigger a snapshot.
 */
void storeOutcome( const double* Q, const double maxEigenvalue) {
  if (validOutcome==nullptr) {
    validOutcome = new double[ NumberOfOutputEntries ];
    std::memcpy(validOutcome,Q,sizeof(double)*NumberOfOutputEntries);
    logInfo( "storeOutcome(double*)", "bookmarked reference solution" );
  }
  validMaxEigenvalue = maxEigenvalue;
}

/**
 * Works if and only if storeOutcome has been performed before.
 *
 * @return Tuple of maximum difference and total number of errors.
 */
std::tuple<double, int> validateOutcome(double* Q, int patchIndex, const double maxEigenvalue) {
  int index = 0;
  int errors = 0;
  double maxDifference = 0.0;
  for (int i=0; i<NumberOfOutputEntries; i++) {
    if (not tarch::la::equals( Q[i], validOutcome[i], Accuracy) ) {
      errors++;
      if (patchIndex == 0) {
        std::cout.precision(16);
        std::cout << std::fixed << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i] << "!=" << validOutcome[i] << std::endl;
      }
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i]-validOutcome[i]) );
    index++;
  }
  if (not tarch::la::equals( maxEigenvalue, validMaxEigenvalue, Accuracy)) {
    std::cout << std::fixed << " maxEigenvalue[" << patchIndex << "]!=validMaxEigenvalue[" << patchIndex << "]: " << maxEigenvalue << "!=" << validMaxEigenvalue << std::endl;
    errors++;
  }

  return { maxDifference, errors };
}

constexpr double TimeStamp    = 0.5;
constexpr double TimeStepSize = 1e-6;
constexpr double CellSize     = 0.1;
constexpr double CellOffset   = 4.0;

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
void runBenchmarksOnHost(int numberOfPatches) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  /**
   * Allocate test data, run the kernel, and time the execution.
   *
   * If the user has specified an accuracy, check if the outcome agrees with
   * the baseline data up to this accuracy. If the routine is invoked on
   * thread 0, dump the time spent in the kernel.
   */
  auto runBaseline = [&](
    int currentThread,
    int patches
  ) -> void {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    for (int i=0; i<patches; i++) {
      inputData.push_back( tarch::allocateMemory<double>(NumberOfInputEntries, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory) );
      outputData.push_back( new double[ NumberOfOutputEntries ] );
      initInputData( inputData[i] );
    }

    tarch::timing::Watch watch( "::runBenchmarksOnHost", "runBaseline(...)", false );
    double outputMaxEigenvalue = 0;
    for (int i=0; i<patches; i++) {
      outputMaxEigenvalue = tasks::CCZ4FVRusanovEnclaveTask::applyKernelToCell(
        marker,
        TimeStamp,
        TimeStepSize,
        inputData[i],
        outputData[i]
      );
    }
    watch.stop();

    if (currentThread==0) {
      logInfo("main()", "baseline: "
        << watch.getCalendarTime() << " / "
        << (watch.getCalendarTime()/patches/NumberOfFiniteVolumesPerPatch) );
    }

    if constexpr (Accuracy>0.0) {
      int errors = 0;
      double maxDifference = 0.0;
      for (int i=0; i<patches; i++) {
        storeOutcome( outputData[i],outputMaxEigenvalue );
        auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( outputData[i],i ,outputMaxEigenvalue);
        errors += errorsPerPatch;
        maxDifference = std::max(maxDifference, maxDifferencePerPatch);
      }

      if (errors > 0) {
        logError("main()", "max difference of outcome from baseline for all patches is "
          << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
        std::abort();
      }
    }

    for (int i=0; i<patches; i++) {
      // inputData is freed automatically by the kernel
      delete[] outputData[i];
    }
  };

  /**
   * @see runBaseline
   */
  auto runStatelessVariantOnHost = [&]( std::function<void(
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel,
    const std::string&      markerName,
    int                     currentThread,
    int                     patches
  ) -> void {
    ::exahype2::CellData patchData(patches);
    for (int i=0; i<patches; i++) {
      patchData.QIn[i]           = new double[ NumberOfInputEntries ];
      patchData.t[i]             = TimeStamp;
      patchData.dt[i]            = TimeStepSize;
      patchData.QOut[i]          = new double[ NumberOfOutputEntries ];
      patchData.cellCentre[i]   = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
      patchData.cellSize[i]     = tarch::la::Vector<Dimensions,double>(CellSize);
      initInputData( patchData.QIn[i] );
    }

    tarch::timing::Watch watch( "::runBenchmarksOnHost", "runStatelessVariantOnHost(...)", false );
    kernel(
      patchData,
      false ,
      true ,
      true ,
      true   //computeCFL
    );
    watch.stop();

    if (currentThread==0) {
      logInfo("main()", "stateless kernels on host with "
        <<  markerName << ": "
        << watch.getCalendarTime() << " / "
        << (watch.getCalendarTime()/patches/NumberOfFiniteVolumesPerPatch) );
    }

    if constexpr (Accuracy>0.0) {
      int errors = 0;
      double maxDifference = 0.0;
      for (int i=0; i<patches; i++) {
        auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( patchData.QOut[i],i,patchData.maxEigenvalue[i]);
        errors += errorsPerPatch;
        maxDifference = std::max(maxDifference, maxDifferencePerPatch);
      }

      if (errors > 0) {
        logError("main()", "max difference of outcome from " << markerName << " for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
        std::abort();
      }
    }

    for (int i=0; i<patches; i++) {
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }
  };

  const auto numberOfPatchesPerThread = numberOfPatches / tarch::multicore::Core::getInstance().getNumberOfThreads();

  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runBaseline(thread, numberOfPatchesPerThread);

    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "heap, patch-wise, AoS", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "heap, patch-wise, AoSoA", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "heap, patch-wise, SoA", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_in_situ_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables>,
      "heap, patch-wise, in-situ", thread, numberOfPatchesPerThread
    );

    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "heap, batched, AoS", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "heap, batched, AoSoA", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "heap, batched, SoA", thread, numberOfPatchesPerThread
    );
    runStatelessVariantOnHost(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_in_situ_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables>,
      "heap, batched, in-situ", thread, numberOfPatchesPerThread
    );
  }
}

void runStatelessVariantOnDevice(
  int                       patches,
  int                       device,
  std::function<void(
    int                     device,
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
  )> kernel,
  const std::string&      markerName,
  int                     currentThread
) {
  ::exahype2::CellData patchData(patches);
  for (int i=0; i<patches; i++) {
    patchData.QIn[i]           = tarch::allocateMemory<double>(NumberOfInputEntries, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    patchData.t[i]             = TimeStamp;
    patchData.dt[i]            = TimeStepSize;
    patchData.QOut[i]          = new double[ NumberOfOutputEntries ];
    patchData.cellCentre[i]   = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
    patchData.cellSize[i]     = tarch::la::Vector<Dimensions,double>(CellSize);
    initInputData( patchData.QIn[i] );
  }

  // We launch the kernel once before we do the actual measurement.
  // This is to simulate the "burn-in" phase of the GPU memory manager.
  kernel(
    device,
    patchData,
    false ,
    true ,
    true ,
    true   //computeCFL
  );

  tarch::timing::Watch watch( "::runBenchmarksOnDevice", "runStatelessVariantOnDevice(...)", false );
  kernel(
    device,
    patchData,
    false ,
    true ,
    true ,
    true   //computeCFL
  );
  watch.stop();

  if (currentThread==0) {
    logInfo("main()", "stateless kernels on device " << device << " with "
      << markerName << ": "
      << watch.getCalendarTime() << " / "
      << (watch.getCalendarTime()/patches/NumberOfFiniteVolumesPerPatch));
  }

  if constexpr (Accuracy>0.0) {
    int errors = 0;
    double maxDifference = 0.0;
    for (int i=0; i<patches; i++) {
      auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( patchData.QOut[i],i,patchData.maxEigenvalue[i]);
      errors += errorsPerPatch;
      maxDifference = std::max(maxDifference, maxDifferencePerPatch);
    }

    if (errors > 0) {
      logError("main()", "max difference of outcome from "
        << markerName << " for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
      std::abort();
    }
  }

  for (int i=0; i<patches; i++) {
    tarch::freeMemory( patchData.QIn[i], tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
    delete[] patchData.QOut[i];
  }
};

#if defined(GPUOffloadingSYCL)
void submitSYCLKernels(int patches, int device, int thread) {
  tarch::accelerator::getSYCLQueue(thread).submit([&](auto &h) {
    h.host_task([=]() {
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "sycl, task-graph, AoS", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "sycl, task-graph, AoSoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "sycl, task-graph, SoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "sycl, patch-wise, AoS", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "sycl, patch-wise, AoSoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "sycl, patch-wise, SoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "sycl, batched, AoS", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "sycl, batched, AoSoA", thread
      );
      runStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "sycl, batched, SoA", thread
      );
    });
  }).wait();
}
#endif

/**
 * This is the heart of the overall benchmark. We first of all invoke
 * as many threads as we are supposed to use, and we then make each
 * (logical) thread issue GPU offloading calls. Only the first thread
 * reports its runtime results (so we don't get a mess).
 *
 * @see runBenchmarksOnHost::runBaseline()
 */
void runBenchmarksOnDevice(int numberOfPatches) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

#if defined(SharedOMP) and defined(GPUOffloadingOMP)
  const int device = 0;
  const auto numberOfPatchesPerThread = numberOfPatches / tarch::multicore::Core::getInstance().getNumberOfThreads();

  #if defined(__NVCOMPILER_CUDA__) or defined(__HIPCC__) or defined(__AMDGPU__) or defined(__clang__)
    tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "omp, patch-wise, AoS, managed", thread
      );
    }
    tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
          ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
          "omp, patch-wise, AoSoA, managed", thread
        );
      }
    tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "omp, patch-wise, SoA, managed", thread
      );
    }
  #else
    #pragma omp parallel
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "omp, patch-wise, AoS, map", thread
      );
    }
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "omp, patch-wise, AoSoA, map", thread
      );
    }
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "omp, patch-wise, SoA, map", thread
      );
    }
  #endif

  #if defined(__NVCOMPILER_CUDA__) or defined(__HIPCC__) or defined(__AMDGPU__) or defined(__clang__)
    tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "omp, batched, AoS, managed", thread
      );
    }
    tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "omp, batched, AoSoA, managed", thread
      );
    }
    tarch::accelerator::omp::GPUMemoryManager::getInstance().clear();
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "omp, batched, SoA, managed", thread
      );
    }
  #else
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "omp, batched, AoS, map", thread
      );
    }
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "omp, batched, AoSoA, map", thread
      );
    }
    #pragma omp parallel for
    for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
      runStatelessVariantOnDevice(
        numberOfPatchesPerThread, device,
        ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "omp, batched, SoA, map", thread
      );
    }
  #endif
#elif defined(SharedTBB) and defined(GPUOffloadingSYCL)
  ::tbb::task_group taskGroup;
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    taskGroup.run(
      [=]() -> void {
        submitSYCLKernels(numberOfPatchesPerThread, device, thread);
      }
    );
  }
  taskGroup.wait();
#elif defined(SharedSYCL) and defined(GPUOffloadingSYCL)
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    tarch::multicore::getHostSYCLQueue().submit([&](auto &h) {
      h.host_task([=]() {
        submitSYCLKernels(numberOfPatchesPerThread, device, thread);
      });
    }
  }
  tarch::multicore::getHostSYCLQueue().wait();
#elif defined(SharedOMP) and defined(GPUOffloadingSYCL)
  ::tbb::task_group taskGroup;
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    taskGroup.run(
      [=]() -> void {
        submitSYCLKernels(numberOfPatchesPerThread, device, thread);
      }
    );
  }
#elif defined(GPUOffloadingSYCL)
  submitSYCLKernels(numberOfPatchesPerThread, device, 0);
#elif /*defined(SharedCPP) and*/ defined(GPUOffloadingCPP)
  // @TODO: Parallelise with C++ threads
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "cpp, patch-wise, AoS", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "cpp, patch-wise, AoSoA", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "cpp, patch-wise, SoA", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "cpp, batched, AoS", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "cpp, batched, AoSoA", thread
    );
    runStatelessVariantOnDevice(
      numberOfPatchesPerThread, device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<CCZ4FVRusanov,CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, CCZ4FVRusanov::NumberOfUnknowns, CCZ4FVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "cpp, batched, SoA", thread
    );
  }
#endif
}

int main(int argc, char** argv) {
  constexpr int ExitCodeSuccess          = 0;
  constexpr int ExitCodeInvalidArguments = 2;
  constexpr int ExitCodeInvalidBuild     = 3;

  peano4::initParallelEnvironment(&argc,&argv);
  exahype2::initNonCritialAssertionEnvironment();
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

#ifdef UseFenv
  tarch::feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 and tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }

  repositories::startSimulation();

  otterTraceInitialise();

  tarch::logging::Statistics::getInstance().clear();

  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();

  #if defined(GPUOffloadingSYCL)
  if ( tarch::accelerator::Device::getInstance().getNumberOfDevices()>0 ) {
    logWarning( "main()", "you might have to set ZE_SERIALIZE=1|2|3 before you start the benchmark" );
  }
  #endif

  logInfo("main()", "Number of threads: " << tarch::multicore::Core::getInstance().getNumberOfThreads());
  logInfo("main()", "Number of finite volumes per axis per patch: " << CCZ4FVRusanov::NumberOfFiniteVolumesPerAxisPerPatch);
  logInfo("main()", "Dimensions: " << Dimensions);

  int numberOfPatches = tarch::multicore::Core::getInstance().getNumberOfThreads();
  for (; numberOfPatches <= 1024*2*2; numberOfPatches *= 2) {
    std::stringstream msg;
    msg << "kernel-benchmarks-" << numberOfPatches << "-patches-" << tarch::multicore::Core::getInstance().getNumberOfThreads() << "-threads";
    otterPhaseSwitch( msg.str().c_str() );

    logInfo("main()", "========================================");
    logInfo("main()", "  Number of patches: " << numberOfPatches);
    logInfo("main()", "========================================");

    runBenchmarksOnHost(numberOfPatches);
    if (tarch::accelerator::Device::getInstance().getNumberOfDevices() > 0) {
      runBenchmarksOnDevice(numberOfPatches);
    }
  }

  if constexpr (Accuracy>0.0) {
    logInfo("main()", "all kernels yield the same outcome up to a accuracy of " << Accuracy);
  } else {
    logInfo("main()", "no accuracy checks were performed");
  }

  otterTraceFinalise();

  delete validOutcome;

  repositories::finishSimulation();
  repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
