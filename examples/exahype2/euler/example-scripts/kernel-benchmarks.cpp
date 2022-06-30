#include "finitevolumes-main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/timing/Watch.h"
#include "tarch/timing/Measurement.h"
#include "tarch/tests/TreeTestCaseCollection.h"

#include "peano4/peano.h"

#include "repositories/DataRepository.h"
#include "repositories/StepRepository.h"
#include "repositories/SolverRepository.h"

#include "observers/CreateGrid.h"
#include "observers/CreateGridButPostponeRefinement.h"
#include "observers/CreateGridAndConvergeLoadBalancing.h"
#include "observers/InitGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"

#include "toolbox/loadbalancing/loadbalancing.h"


#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"

#include "tasks/EulerOnGPUEnclaveTask.h"

#include "config.h"

#if defined(SharedTBB)
#include <tbb/task_group.h>
#endif

using namespace examples::exahype2::euler;


tarch::logging::Log _log("::");


tarch::timing::Measurement   gridConstructionMeasurement;
tarch::timing::Measurement   timeStepMeasurement;
tarch::timing::Measurement   plotMeasurement;

double* validOutcome = nullptr;


#if Dimensions==2
constexpr int NumberOfInputEntries  = (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2) * EulerOnGPU::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0) * EulerOnGPU::NumberOfUnknowns;
#else
constexpr int NumberOfInputEntries  = (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2) * EulerOnGPU::NumberOfUnknowns;
constexpr int NumberOfOutputEntries = (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0) * EulerOnGPU::NumberOfUnknowns;
#endif


/**
 * Set input data
 *
 * I really don't care if this makes any sense. Just make the data reasonably
 * smooth and ensure that all is positive, as there's for example density among
 * the values.
 */
void initInputData( double* Q ) {
  for (int i=0; i<NumberOfInputEntries/5; i++) {
    Q[i*5+0] = 1.0;
    Q[i*5+1] = std::sin( 1.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*5+2] = std::sin( 2.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*5+3] = std::sin( 3.0 * i / (NumberOfInputEntries) * tarch::la::PI );
    Q[i*5+4] = 1.1 + std::sin( i / (NumberOfInputEntries) * tarch::la::PI );
  }
}


/**
 * Make a persistent snapshot of a solution and assume, from hereon, that
 * this snapshot is the valid data. You can call this routine as often as
 * you want. Only the very first call will trigger a snapshot.
 */
void storeOutcome( const double* Q ) {
  if (validOutcome==nullptr) {
    validOutcome = new double[ NumberOfOutputEntries ];
    std:memcpy(validOutcome,Q,sizeof(double)*NumberOfOutputEntries);
    logInfo( "storeOutcome(double*)", "bookmarked reference solution" );
  }
}


/**
 * Works if and only if storeOutcome has been performed
 * before.
 */
void validateOutcome( const double* Q, int patchIndex ) {
  int index = 0;
  int errors = 0;
  double maxDifference = 0.0;
  for (int i=0; i<NumberOfOutputEntries; i++) {
    if (not tarch::la::equals( Q[i], validOutcome[i], examples::exahype2::euler::Accuracy) ) {
      errors++;
    }
    maxDifference = std::max(maxDifference, std::abs(Q[i]-validOutcome[i]) );
    index++;
  }
  if (errors>0) {
    logError( "validateOutcome(...)", "outcomes in patch " << patchIndex << " differ in " << errors << " entries with max difference of " << maxDifference << " (admissible accuracy=" << examples::exahype2::euler::Accuracy << ")"  );
    logError( "validateOutcome(...)", "terminate immediately" );
    exit(-1);
  }
}



constexpr double TimeStamp    = 0.5;
constexpr double TimeStepSize = 1e-6;
constexpr double CellSize     = 0.1;
constexpr double CellOffset   = 4.0;

/**
 * Run the benchmark for one particular number of patches per fused kernel call.
 */
void runBenchmarks(int patches) {
  peano4::grid::GridTraversalEvent event;
  event.setX(CellOffset);
  event.setH(CellSize);
  peano4::datamanagement::CellMarker marker(event);

  logInfo( "main()", "========================================" );
  logInfo( "main()", "  Number of patches fused: " << patches   );
  logInfo( "main()", "========================================" );

  {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    for (int i=0; i<patches; i++) {
      inputData.push_back( new double[ NumberOfInputEntries ] );
      outputData.push_back( new double[ NumberOfOutputEntries ] );
      initInputData( inputData[i] );
    }

    tarch::timing::Watch watch( "::", "main()", false );
    for (int i=0; i<patches; i++) {
      tasks::EulerOnGPUEnclaveTask::applyKernelToCell(
        marker,
        TimeStamp,            // double t
        TimeStepSize,         // double dt
        inputData[i],outputData[i]
      );
    }
    watch.stop();

    for (int i=0; i<patches; i++) {
      storeOutcome( outputData[i] );
      validateOutcome( outputData[i],i );
      delete[] outputData[i];
    }

    logInfo("main()", "completed measurements for baseline on host: "       << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
  }


  auto testFusedVariant = [&]( std::function<void(
    exahype2::CellData&     patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel, std::string markerName, bool computeCFL ) {
    ::exahype2::CellData patchData(patches);
    for (int i=0; i<patches; i++) {
      patchData.QIn[i]           = new double[ NumberOfInputEntries ];
      patchData.t[i]             = TimeStamp;
      patchData.dt[i]            = TimeStepSize;
      patchData.QOut[i]          = new double[ NumberOfOutputEntries ];
      patchData.patchCentre[i]   = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
      patchData.patchSize[i]     = tarch::la::Vector<Dimensions,double>(CellSize);
      initInputData( patchData.QIn[i] );
    }

    tarch::timing::Watch watch( "::", "main()", false );

    kernel(
      patchData,
      true ,
      false ,
      false ,
      computeCFL
    );

    watch.stop();

    for (int i=0; i<patches; i++) {
      validateOutcome( patchData.QOut[i],i );
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }

    logInfo("main()", "completed measurements for " << markerName << " on host: " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
  };


  auto testFusedVariantOnDevice = [&](
    int device,
    std::function<void(
      int                     device,
      exahype2::CellData&     patchData,
      bool                    evaluateFlux,
      bool                    evaluateNonconservativeProduct,
      bool                    evaluateSource,
      bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel, std::string markerName, bool computeCFL ) {

    constexpr int MaxHostTasks = 5;
    ::exahype2::CellData patchData[MaxHostTasks] = { patches, patches, patches, patches, patches };

    for (int hostTask=0; hostTask<MaxHostTasks; hostTask++)
    for (int i=0; i<patches; i++) {
      patchData[hostTask].QIn[i]   = new double[ NumberOfInputEntries ];
      patchData[hostTask].t[i]     = TimeStamp;
      patchData[hostTask].dt[i]    = TimeStepSize;
      patchData[hostTask].QOut[i]  = new double[ NumberOfOutputEntries ];
      patchData[hostTask].patchCentre[i]     = tarch::la::Vector<Dimensions,double>(CellOffset+0.5*CellSize);
      patchData[hostTask].patchSize[i]       = tarch::la::Vector<Dimensions,double>(CellSize);
    }

    tarch::timing::Watch singleKernelWatch( "::", "main()", false );
    kernel(
      device,
      patchData[0],
      true ,
      false ,
      false ,
      computeCFL
    );
    singleKernelWatch.stop();
    logInfo("main()", "completed measurements for single thread on device " << device << ", " << markerName << ": " << singleKernelWatch.getCalendarTime() << " / " << (singleKernelWatch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );

    tarch::timing::Watch parallelKernelWatch( "::", "main()", false );
    #if defined(GPUOffloadingOMP)
    #pragma omp parallel for
    for (int hostTask=1; hostTask<MaxHostTasks; hostTask++) {
      kernel(
        device,
        patchData[hostTask],
        true ,
        false ,
        false ,
        computeCFL
      );
    }
    #endif

    #if defined(GPUOffloadingSYCL) and defined(SharedTBB)
    ::tbb::task_group taskGroup;
    // @todo remove semaphore
    tarch::multicore::BooleanSemaphore semaphore;
    for (int hostTask=1; hostTask<MaxHostTasks; hostTask++) {
      taskGroup.run(
        [&]() -> void {
          tarch::multicore::Lock lock(semaphore);
/*
          kernel(
            device,
            patchData[hostTask],
            true ,
            false ,
            false ,
            computeCFL
          );
*/
        }
      );
    }
    taskGroup.wait();
    #endif
    parallelKernelWatch.stop();
    logInfo("main()", "completed measurements for " << (MaxHostTasks-1) << " threads on device " << device << ", " << markerName << ": " << parallelKernelWatch.getCalendarTime() << " / " << (parallelKernelWatch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/(MaxHostTasks-1)) );

    for (int hostTask=0; hostTask<MaxHostTasks; hostTask++)
    for (int i=0; i<patches; i++) {
//      validateOutcome( patchData[hostTask].QOut[i],i );
      delete[] patchData[hostTask].QIn[i];
      delete[] patchData[hostTask].QOut[i];
    }

  };

  #if Dimension==2
  if (patches<=64) {
  #else
  if (patches<=4) {
  #endif
    testFusedVariant(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_call_stack_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "call stack, patch-wise, AoS", true
    );
    testFusedVariant(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_call_stack_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "call stack, batched, AoS", true
    );
  }

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
    "heap, patch-wise, AoS", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
    "heap, batched, AoS", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
    "heap, patch-wise, AoSoA", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
    "heap, batched, AoSoA", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
    "heap, patch-wise, SoA", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
    "heap, batched, SoA", true
  );

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_in_situ_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0>,
    "heap, patch-wise, in-situ", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_in_situ_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0>,
    "heap, batched, in-situ", true
  );

  #if defined(GPUOffloadingOMP)
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
    logWarning( "main()", "no device found" );
  }
  for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
    logInfo( "main()", "device " << device );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "omp, patch-wise, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "omp, patch-wise, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
      "omp, patch-wise, SoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "omp, batched, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "omp, batched, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
      "omp, batched, SoA", true
    );
  }
  #endif

  #if defined(GPUOffloadingCPP)
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
    logWarning( "main()", "no device found" );
  }
  for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
    logInfo( "main()", "device " << device );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "cpp, patch-wise, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "cpp, batched, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "cpp, patch-wise, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "cpp, batched, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
      "cpp, patch-wise, SoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::cpp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
      "cpp, batched, SoA", true
    );
  }
  #endif

  #if defined(GPUOffloadingSYCL)
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
    logWarning( "main()", "no device found" );
  }
  for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
    logInfo( "main()", "device " << device );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "sycl, task-graph, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSLexicographicEnumerator>,
      "sycl, batched, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "sycl, task-graph, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::AoSoALexicographicEnumerator>,
      "sycl, batched, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
      "sycl, task-graph, SoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::sycl::timeStepWithRusanov_taskgraph_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::enumerator::SoALexicographicEnumerator>,
      "sycl, batched, SoA", true
    );
  }
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

  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>1 and tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {
    logError("main()", "MPI runs without multithreading are not supported currently.");
    return ExitCodeInvalidBuild;
  }

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {
    logError("main()", "Invalid command line arguments.");
    return ExitCodeInvalidArguments;
  }

  examples::exahype2::euler::repositories::DataRepository::initDatatypes();

  repositories::startSimulation();

  otterTraceInitialise();

  tarch::logging::Statistics::getInstance().clear();

  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();


  #if defined(GPUOffloadingSYCL)
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()>0 ) {
    logWarning( "main()", "you might have to set ZE_SERIALIZE=1|2|3 before you start the benchmark" );
  }
  #endif

  #if Dimensions==2
  for (int patches=1; patches<=1024*2*2*2*2; patches*=2) {
  #else
  for (int patches=1; patches<=1024*2; patches*=2) {
  #endif
    std::stringstream msg;
    msg << "benchmark-" << patches;
    otterPhaseSwitch( msg.str().c_str() );

    runBenchmarks(patches);
  }

  otterTraceFinalise();

//  repositories::finishSimulation();

  examples::exahype2::euler::repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
