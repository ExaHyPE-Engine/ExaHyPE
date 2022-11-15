#include "EulerFVRusanovKernelBenchmarks-main.h"
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
#include "tasks/EulerFVRusanovEnclaveTask.h"
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

#include "toolbox/loadbalancing/loadbalancing.h"

#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"

#include "config.h"
#include "tarch/tests/TreeTestCaseCollection.h"
#include "exahype2/UnitTests.h"
//#ifdef UseFenv
//#include "tarch/FenvWrapper.h"
//#endif

#if defined(SharedTBB)
#include <tbb/task_group.h>
#endif

using namespace benchmarks::exahype2::euler;

tarch::logging::Log _log("::");

tarch::timing::Measurement gridConstructionMeasurement;
tarch::timing::Measurement timeStepMeasurement;
tarch::timing::Measurement plotMeasurement;

double validMaxEigenvalue = 0;
double* validOutcome = nullptr;

#if Dimensions==2
constexpr int NumberOfInputEntries  = (EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2) * EulerFVRusanov::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0) * EulerFVRusanov::NumberOfUnknowns;
#else
constexpr int NumberOfInputEntries  = (EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+2) * EulerFVRusanov::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch+0) * EulerFVRusanov::NumberOfUnknowns;
#endif

/**
 * Set input data
 *
 * We really don't care if this makes any sense. Just make the data reasonably
 * smooth and ensure that all is positive, as there's for example density among
 * the values.
 */
void initInputData( double* Q ) {
  for (int i=0; i<NumberOfInputEntries/EulerFVRusanov::NumberOfUnknowns; i++) {
    Q[i*EulerFVRusanov::NumberOfUnknowns+0] = 1.0;
    Q[i*EulerFVRusanov::NumberOfUnknowns+1] = std::sin( 1.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*EulerFVRusanov::NumberOfUnknowns+2] = std::sin( 2.0 * i / (NumberOfInputEntries) * tarch::la::PI );
#if Dimensions == 3
    Q[i*EulerFVRusanov::NumberOfUnknowns+3] = std::sin( 3.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*EulerFVRusanov::NumberOfUnknowns+4] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
#else
    Q[i*EulerFVRusanov::NumberOfUnknowns+3] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
#endif
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
std::tuple<double, int> validateOutcome( double* Q, int patchIndex , const double maxEigenvalue) {
  int index = 0;
  int errors = 0;
  double maxDifference = 0.0;
  for (int i=0; i<NumberOfOutputEntries; i++) {
    if (not tarch::la::equals( Q[i], validOutcome[i], Accuracy) ) {
      errors++;
      std::cout.precision(16);
      std::cout << std::fixed << "Q[" << i << "]!=validOutcome[" << i << "]: " << Q[i] << "!=" << validOutcome[i] << std::endl;
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

static constexpr double TimeStamp    = 0.5;
static constexpr double TimeStepSize = 1e-6;
static constexpr double CellSize     = 0.1;
static constexpr double CellOffset   = 4.0;

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
 * @param patches Number of patches to study (in one go)
 */
void runBenchmarksOnHost(int patches) {
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
  auto testCPUVariant = [&](
    int currentThread
  ) -> void {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    for (int i=0; i<patches; i++) {
      inputData.push_back( new double[ NumberOfInputEntries ] );
      outputData.push_back( new double[ NumberOfOutputEntries ] );
      initInputData( inputData[i] );
    }

    tarch::timing::Watch watch( "::runBenchmarksOnHost", "testCPUVariant(...)", false );
    double outputMaxEigenvalue = 0;
    for (int i=0; i<patches; i++) {
      outputMaxEigenvalue = tasks::EulerFVRusanovEnclaveTask::applyKernelToCell(
        marker,
        TimeStamp,
        TimeStepSize,
        inputData[i],
        outputData[i]
      );
    }
    watch.stop();

    int errors = 0;
    double maxDifference = 0.0;
    if (Accuracy>0.0) {
      for (int i=0; i<patches; i++) {
        storeOutcome( outputData[i],outputMaxEigenvalue );
        auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( outputData[i],i ,outputMaxEigenvalue);
        errors += errorsPerPatch;
        maxDifference = std::max(maxDifference, maxDifferencePerPatch);
        delete[] outputData[i];
      }
    }

    if (currentThread==0) {
      logInfo("main()", "baseline: "       << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch) );
    }
    if (errors > 0) {
      logError("main()", "max difference of outcome for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
      exit(-1);
    }
  };

  /**
   * @see testCPUVariant
   */
  auto testStatelessVariantOnHost = [&]( std::function<void(
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel,
    const std::string&      markerName,
    int                     currentThread
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

    tarch::timing::Watch watch( "::runBenchmarksOnHost", "testStatelessVariantOnHost(...)", false );
    kernel(
      patchData,
      true ,
      false ,
      false ,
      true   //computeCFL
    );
    watch.stop();

    int errors = 0;
    double maxDifference = 0.0;
    if (Accuracy>0.0) {
      double maxDifference = 0.0;
      for (int i=0; i<patches; i++) {
        auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( patchData.QOut[i],i,patchData.maxEigenvalue[i]);
        errors += errorsPerPatch;
        maxDifference = std::max(maxDifference, maxDifferencePerPatch);
        delete[] patchData.QIn[i];
        delete[] patchData.QOut[i];
      }
    }

    if (currentThread==0) {
      logInfo("main()", "stateless kernels on host with " << markerName << ": " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch) );
    }
    if (errors > 0) {
      logError("main()", "max difference of outcome for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
      exit(-1);
    }
  };

#if defined(SharedOMP)
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
#elif defined(SharedTBB)
  ::tbb::task_group taskGroup;
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    taskGroup.run(
      [=]() -> void {
#elif defined(SharedCPP)
  #error Not implemented yet
#elif defined(SharedSYCL)
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    tarch::multicore::getSYCLQueue(tarch::multicore::Core::HostDevice).submit([&](auto &h) {
      h.host_task([=]() {
#else
  int thread = 0;
#endif
        testCPUVariant(thread);

        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
          "heap, patch-wise, AoS", thread
        );
        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
          "heap, patch-wise, AoSoA", thread
        );
        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
          "heap, patch-wise, SoA", thread
        );
        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_in_situ_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables>,
          "heap, patch-wise, in-situ", thread
        );

        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
          "heap, batched, AoS", thread
        );
        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
          "heap, batched, AoSoA", thread
        );
        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
          "heap, batched, SoA", thread
        );
        testStatelessVariantOnHost(
          ::exahype2::fv::rusanov::timeStepWithRusanov_batched_in_situ_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables>,
          "heap, batched, in-situ", thread
        );

#if defined(SharedOMP)
  }
#elif defined(SharedTBB)
    });
  }
  taskGroup.wait();
#elif defined(SharedSYCL)
      });
    });
  }
  tarch::multicore::getSYCLQueue(tarch::multicore::Core::HostDevice).wait();
#else
#endif
}


void testStatelessVariantOnDevice(
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
    patchData.QIn[i]           = new double[ NumberOfInputEntries ];
    patchData.t[i]             = TimeStamp;
    patchData.dt[i]            = TimeStepSize;
    patchData.QOut[i]          = new double[ NumberOfOutputEntries ];
    patchData.cellCentre[i]   = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
    patchData.cellSize[i]     = tarch::la::Vector<Dimensions,double>(CellSize);
    initInputData( patchData.QIn[i] );
  }

  tarch::timing::Watch watch( "::runBenchmarksOnDevice", "testStatelessVariantOnDevice(...)", false );
  kernel(
    device,
    patchData,
    true ,
    false ,
    false ,
    true   //computeCFL
  );
  watch.stop();

  int errors = 0;
  double maxDifference = 0.0;
  if (Accuracy>0.0) {
    double maxDifference = 0.0;
    for (int i=0; i<patches; i++) {
      auto [maxDifferencePerPatch, errorsPerPatch] = validateOutcome( patchData.QOut[i],i,patchData.maxEigenvalue[i]);
      errors += errorsPerPatch;
      maxDifference = std::max(maxDifference, maxDifferencePerPatch);
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }
  }

  if (currentThread==0) {
    logInfo("main()", "stateless kernels on device " << device << " with " << markerName << ": " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch/EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch) );
  }
  if (errors > 0) {
    logError("main()", "max difference of outcome for all patches is " << maxDifference << " (admissible accuracy=" << Accuracy << ")" << " for " << errors << " entries");
    exit(-1);
  }
};


#if defined(GPUOffloadingSYCL)
void submitSYCLKernels(int patches, int device, int thread) {
  tarch::accelerator::getSYCLQueue(thread).submit([&](auto &h) {
    h.host_task([=]() {
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "sycl, task-graph, AoS", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "sycl, task-graph, AoSoA", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "sycl, task-graph, SoA", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "sycl, patch-wise, AoS", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "sycl, patch-wise, AoSoA", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_patchwise_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
        "sycl, patch-wise, SoA", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
        "sycl, batched, AoS", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
        "sycl, batched, AoSoA", thread
      );
      testStatelessVariantOnDevice(
        patches, device,
        ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
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
 * @see runBenchmarksOnHost::testCPUVariant()
 */
void runBenchmarksOnDevice(int patches, int device) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  #if defined(SharedOMP) and defined(GPUOffloadingOMP)
  #pragma omp parallel for
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "omp, patch-wise, AoS, map", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "omp, patch-wise, AoSoA, map", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_map_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "omp, patch-wise, SoA, map", thread
    );

    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "omp, patch-wise, AoS, user-managed-memory", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "omp, patch-wise, AoSoA, user-managed-memory", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_managed_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "omp, patch-wise, SoA, user-managed-memory", thread
    );

    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "omp, batched, AoS, map", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "omp, batched, AoSoA, map", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_map_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "omp, batched, SoA, map", thread
    );

    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSLexicographicEnumerator>,
      "omp, batched, AoS, user-managed-memory", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "omp, batched, AoSoA, user-managed-memory", thread
    );
    testStatelessVariantOnDevice(
      patches, device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_managed_static_calls<EulerFVRusanov,EulerFVRusanov::NumberOfFiniteVolumesPerAxisPerPatch, 1, EulerFVRusanov::NumberOfUnknowns, EulerFVRusanov::NumberOfAuxiliaryVariables,exahype2::enumerator::SoALexicographicEnumerator>,
      "omp, batched, SoA, user-managed-memory", thread
    );
  }
  #elif defined(SharedTBB) and defined(GPUOffloadingSYCL)
  ::tbb::task_group taskGroup;
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    taskGroup.run(
      [=]() -> void {
        submitSYCLKernels(patches, device, thread);
      }
    );
  }
  taskGroup.wait();
  #elif defined(SharedSYCL) and defined(GPUOffloadingSYCL)
  for (int thread=0; thread<tarch::multicore::Core::getInstance().getNumberOfThreads(); thread++) {
    tarch::multicore::getHostSYCLQueue().submit([&](auto &h) {
      h.host_task([=]() {
        submitSYCLKernels(patches, device, thread);
      });
    }
  }
  tarch::multicore::getHostSYCLQueue().wait();
  #elif defined(SharedCPP)
  #error Not implemented yet
  #endif
}


int main(int argc, char** argv) {
  const int ExitCodeSuccess          = 0;
  const int ExitCodeInvalidArguments = 2;
  const int ExitCodeInvalidBuild     = 3;

  peano4::initParallelEnvironment(&argc,&argv);
  exahype2::initNonCritialAssertionEnvironment();
  tarch::multicore::initSmartMPI();
  peano4::fillLookupTables();

//#ifdef UseFenv
  //tarch::feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
//#endif

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 and tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }

  /*repositories::DataRepository::initDatatypes();
  tarch::tests::TreeTestCaseCollection* unitTests = new tarch::tests::TreeTestCaseCollection("");
  unitTests->addTestCase( exahype2::getUnitTests() );
  unitTests->run();
  tarch::mpi::Rank::abort( 3 );
  delete unitTests;*/

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

  #if Dimensions==2
  for (int patches=1; patches<=1024*2*2*2*2; patches*=2) {
  #else
  for (int patches=1; patches<=1024*2; patches*=2) {
  #endif
    std::stringstream msg;
    msg << "benchmark-" << patches << "-host";
    otterPhaseSwitch( msg.str().c_str() );

    logInfo( "main()", "========================================" );
    logInfo( "main()", "  Number of patches fused: " << patches   );
    logInfo( "main()", "========================================" );

    runBenchmarksOnHost(patches);

    for (int device=0; device<tarch::accelerator::Device::getInstance().getNumberOfDevices(); device++) {
      std::stringstream msg;
      msg << "benchmark-" << patches << "-" << device;

      otterPhaseSwitch( msg.str().c_str() );

      runBenchmarksOnDevice(patches,device);
    }
  }

  if (Accuracy>0.0) {
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
