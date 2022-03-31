#include "finitevolumes-main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"
#include "tarch/logging/Statistics.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/UnitTests.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Core.h"
#include "tarch/timing/Watch.h"
#include "tarch/timing/Measurement.h"
#include "tarch/tests/TreeTestCaseCollection.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/UnitTests.h"

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

#include "toolbox/blockstructured/UnitTests.h"

#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"
#include "exahype2/UnitTests.h"

#include "tasks/EulerOnGPUEnclaveTask.h"


#include <likwid.h>


using namespace examples::exahype2::euler;


tarch::logging::Log _log("::");


tarch::timing::Measurement   gridConstructionMeasurement;
tarch::timing::Measurement   timeStepMeasurement;
tarch::timing::Measurement   plotMeasurement;
  
  
int main(int argc, char** argv) {
  const int ExitCodeSuccess          = 0;
  const int ExitCodeUnitTestsFailed  = 1;
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
  
  #if PeanoDebug>=2
  tarch::tests::TreeTestCaseCollection* unitTests = new tarch::tests::TreeTestCaseCollection("");
  unitTests->addTestCase( peano4::getUnitTests() );
  unitTests->addTestCase( tarch::getUnitTests() );
  unitTests->addTestCase( toolbox::blockstructured::getUnitTests() );
  unitTests->addTestCase( exahype2::getUnitTests() );
  unitTests->run();
  if (unitTests->getNumberOfErrors() != 0) {
    logError("main()", "unit tests failed. Quit.");
    tarch::mpi::Rank::abort( ExitCodeUnitTestsFailed );
  }
  delete unitTests;
  #endif

  repositories::startSimulation();

  tarch::logging::Statistics::getInstance().clear();

  const bool isGlobalMaster     =     tarch::mpi::Rank::getInstance().isGlobalMaster();
  const bool isPeanoComputeNode = not tarch::mpi::Rank::getInstance().isGlobalMaster();
	  
  constexpr int base = 34-EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch;
  constexpr int Patches = base*base*base;

  peano4::grid::GridTraversalEvent event;
  peano4::datamanagement::CellMarker marker(event);

  std::vector<double*> inputData;
  std::vector<double*> outputData;
  std::list<tarch::multicore::Task*> tasks;
  for (int i=0; i<Patches; i++) {
    inputData.push_back( new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns] );
    outputData.push_back( new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*EulerOnGPU::NumberOfUnknowns] );
    tasks.push_back( new tasks::EulerOnGPUEnclaveTask(
      marker,
      0.5,          // double t
      1e-5,         // double dt
      new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns]
    ));
  }

  logInfo("main()", "all data structures set up");

  constexpr int FusedPatches=4;

  likwid_markerInit();

  {
    std::vector<double*> inputData;
    std::vector<double*> outputData;
    for (int i=0; i<Patches; i++) {
      inputData.push_back( new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns] );
      outputData.push_back( new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+0)*EulerOnGPU::NumberOfUnknowns] );
    }

    tarch::timing::Watch watch( "::", "main()", false );
    likwid_markerStartRegion( "virtual" );
    for (int i=0; i<Patches; i++) {
      tasks::EulerOnGPUEnclaveTask::applyKernelToCell(
        marker,
        0.5,          // double t
        1e-5,         // double dt
        inputData[i],outputData[i]
      );
    }
    likwid_markerStopRegion( "virtual" );
    watch.stop();

    for (int i=0; i<Patches; i++) {
      delete[] outputData[i];
    }

    logInfo("main()", "completed baseline measurements: " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/Patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
  }


  auto testFusedVariant = [&]( std::function<void(
    exahype2::CellData&              patchData,
    int, int, int,
    bool                    evaluateFlux,
    bool                    evaluateNonconservativeProduct,
    bool                    evaluateSource,
    bool                    evaluateMaximumEigenvalueAfterTimeStep
    )> kernel, std::string markerName, bool computeCFL ) {
    ::exahype2::CellData patchData(Patches);
    for (int i=0; i<Patches; i++) {
      patchData.QIn[i]         = new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch+2)*EulerOnGPU::NumberOfUnknowns];
      patchData.t[i]           = 0.5;
      patchData.dt[i]          = 1e-5;
      patchData.QOut[i]        = new double[ (EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*(EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch)*EulerOnGPU::NumberOfUnknowns];
    }

    tarch::timing::Watch watch( "::", "main()", false );
    likwid_markerStartRegion( markerName.c_str() );
    for (int i=0; i<Patches; i++) {
      ::exahype2::CellData localPatchData(1);
      localPatchData.QIn[0]         = patchData.QIn[i];
      localPatchData.t[0]           = 0.5;
      localPatchData.dt[0]          = 1e-5;
      localPatchData.QOut[0]        = patchData.QOut[i];

      //tasks[i]->fuse( std::list<tarch::multicore::Task*>() );
      kernel(
        localPatchData,
        EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch, 5, 0,
        true ,
        false ,
        false ,
        computeCFL
      );
    }
    likwid_markerStopRegion( markerName.c_str() );
    watch.stop();

    for (int i=0; i<Patches; i++) {
      delete[] patchData.QIn[i];
      delete[] patchData.QOut[i];
    }

    logInfo("main()", "completed optimisation measurements " << markerName << ": " << watch.getCalendarTime() << " / " << (watch.getCalendarTime()/Patches/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch/EulerOnGPU::NumberOfFiniteVolumesPerAxisPerPatch) );
  };

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-AoS-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-AoSoA-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_in_situ<EulerOnGPU>,
    "no-side-effects-patchwise-in-situ-CFL", true
  );

  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no-side-effects-batched-AoS-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no-side-effects-batched-AoSoA-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched_in_situ<EulerOnGPU>,
    "no-side-effects-batched-in-situ-CFL", true
  );

/*
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_split_terms<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-split-term-AoS-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_split_terms<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-split-terms-AoSoA-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_in_situ<EulerOnGPU>,
    "no-side-effects-patchwise-in-situ-CFL", true
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched_in_situ<EulerOnGPU>,
    "no-side-effects-batched-in-situ-CFL", true
  );


    testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-AoS-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-AoSoA-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no-side-effects-batched-AoS-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no-side-effects-batched-AoSoA-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_split_terms<EulerOnGPU,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-split-term-AoS-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_split_terms<EulerOnGPU,exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator>,
    "no-side-effects-patchwise-split-terms-AoSoA-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_patchwise_in_situ<EulerOnGPU>,
    "no-side-effects-patchwise-in-situ-no-CFL", false
  );
  testFusedVariant(
    ::exahype2::fv::rusanov::timeStepWithRusanov_overlap1_batched_in_situ<EulerOnGPU>,
    "no-side-effects-batched-in-situ-no-CFL", false
  );
*/

  likwid_markerClose();


  tarch::logging::Statistics::getInstance().writeToCSV();

  repositories::finishSimulation();

  peano4::shutdownSingletons();
  examples::exahype2::euler::repositories::DataRepository::shutdownDatatypes();
  tarch::multicore::shutdownSmartMPI();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
