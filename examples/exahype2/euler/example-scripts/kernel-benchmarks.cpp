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
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

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


using namespace examples::exahype2::euler;


tarch::logging::Log _log("::");


tarch::timing::Measurement   gridConstructionMeasurement;
tarch::timing::Measurement   timeStepMeasurement;
tarch::timing::Measurement   plotMeasurement;
  

void runBenchmarks(int patches) {
  peano4::grid::GridTraversalEvent event;
  peano4::datamanagement::CellMarker marker(event);

  logInfo( "main()", "========================================" );
  logInfo( "main()", "  Number of patches fused: " << patches   );
  logInfo( "main()", "========================================" );

  {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    for (int i=0; i<patches; i++) {
      inputData.push_back( new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns] );
      outputData.push_back( new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*EulerOnGPU::NumberOfUnknowns] );
    }

    tarch::timing::Watch watch( "::", "main()", false );
    for (int i=0; i<patches; i++) {
      tasks::EulerOnGPUEnclaveTask::applyKernelToCell(
        marker,
        0.5,          // double t
        1e-5,         // double dt
        inputData[i],outputData[i]
      );
    }
    watch.stop();

    for (int i=0; i<patches; i++) {
      delete[] outputData[i];
    }

    logInfo("main()", "completed baseline measurements: " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
  }


  auto testFusedVariant = [&]( std::function<void(
    exahype2::CellData&              patchData,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel, std::string markerName, bool computeCFL ) {
    ::exahype2::CellData patchData(patches);
    for (int i=0; i<patches; i++) {
      patchData.QIn[i]         = new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns];
      patchData.t[i]           = 0.5;
      patchData.dt[i]          = 1e-5;
      patchData.QOut[i]        = new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*EulerOnGPU::NumberOfUnknowns];
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
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }

    logInfo("main()", "completed optimisation measurements " << markerName << ": " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
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
    ::exahype2::CellData patchData(patches);
    for (int i=0; i<patches; i++) {
      patchData.QIn[i]         = new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns];
      patchData.t[i]           = 0.5;
      patchData.dt[i]          = 1e-5;
      patchData.QOut[i]        = new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*EulerOnGPU::NumberOfUnknowns];
    }

    tarch::timing::Watch watch( "::", "main()", false );

    kernel(
      device,
      patchData,
      true ,
      false ,
      false ,
      computeCFL
    );

    watch.stop();

    for (int i=0; i<patches; i++) {
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }

    logInfo("main()", "completed optimisation measurements " << markerName << ": " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
  };

  #if Dimension==2
  if (patches<=64) {
  #else
  if (patches<=4) {
  #endif
    testFusedVariant(
      ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_call_stack_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
      "no side effects, patch-wise, call stack, AoS", true
    );
    testFusedVariant(
      ::exahype2::fv::rusanov::timeStepWithRusanov_batched_call_stack_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
      "no side effects, batched, call stack, AoS", true
    );
  }

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no side effects, patch-wise, heap, AoS", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no side effects, batched, heap, AoS", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no side effects, patch-wise, heap, AoSoA", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no side effects, batched, heap, AoSoA", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::SoALexicographicVolumeEnumerator>,
    "no side effects, patch-wise, heap, SoA", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::SoALexicographicVolumeEnumerator>,
    "no side effects, batched, heap, SoA", true
  );

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_patchwise_in_situ_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0>,
    "no side effects, patch-wise, in-situ", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_batched_in_situ_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0>,
    "no side effects, batched, in-situ", true
  );

  #if defined(GPUOffloadingOMP)
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
    logWarning( "main()", "no device found" );
  }
  for (int device=0; device<tarch::multicore::Core::getInstance().getNumberOfGPUs(); device++) {
    logInfo( "main()", "device " << device );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
      "omp, patch-wise, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
      "omp, patch-wise, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_patchwise_heap_static_calls<EulerOnGPU,EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 1, 5, 0,exahype2::fv::enumerator::SoALexicographicVolumeEnumerator>,
      "omp, patch-wise, SoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
      "omp, batched, AoS", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
      "omp, batched, AoSoA", true
    );
    testFusedVariantOnDevice(
      device,
      ::exahype2::fv::rusanov::omp::timeStepWithRusanov_batched_heap_static_calls<EulerOnGPU,exahype2::fv::enumerator::SoALexicographicVolumeEnumerator>,
      "omp, batched, SoA", true
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


  peano4::initSingletons(
    DomainOffset,
    DomainSize,
    0
  );

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

  tarch::logging::Statistics::getInstance().clear();

  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();

  #if Dimensions==2
  for (int patches=1; patches<=65536; patches*=2) {
  #else
  for (int patches=1; patches<=2048; patches*=2) {
  #endif
    runBenchmarks(patches);
  }

  repositories::finishSimulation();

  peano4::shutdownSingletons();
  examples::exahype2::euler::repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
