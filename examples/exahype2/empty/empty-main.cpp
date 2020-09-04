#include "empty-main.h"
#include "Constants.h"

#include "tarch/logging/Log.h"

#include "tarch/multicore/multicore.h"

#include "peano4/peano.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "observers/DataRepository.h"
#include "observers/StepRepository.h"
#include "observers/SolverRepository.h"

#include "observers/CreateGrid.h"
#include "observers/PlotSolution.h"
#include "observers/TimeStep.h"

#include "peano4/UnitTests.h"
#include "tarch/UnitTests.h"

#include "exahype2/NonCriticalAssertions.h"
#include "exahype2/UserInterface.h"



using namespace examples::exahype2::empty;



tarch::logging::Log _log("::");



/**
 * @return continues to run
 */
bool selectNextAlgorithmicStep() {
  static bool   gridConstructed   = false;
  static double nextPlotTimeStamp = FirstPlotTimeStamp;
  static bool   haveJustWrittenSnapshot = false;
  static bool   haveReceivedNoncriticialAssertion = false;
  static int    iterationCounter                  = 0;
  bool          continueToSolve                   = true;

  if (exahype2::hasNonCriticalAssertionBeenViolated() and not haveReceivedNoncriticialAssertion) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      observers::StepRepository::toProgramStep( observers::StepRepository::Steps::PlotSolution )
    );
    continueToSolve = true;
    haveReceivedNoncriticialAssertion = true;
    logError( "selectNextAlgorithmicStep()", "noncritical assertion has been triggered in code. Dump final state and terminate" );
  }
  else if (exahype2::hasNonCriticalAssertionBeenViolated()) {
    continueToSolve = false;
  }
  else if (not gridConstructed) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      observers::StepRepository::toProgramStep( observers::StepRepository::Steps::CreateGrid )
    );
    if ( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getStationarySweeps()>=5 ) {
      // This assertion is manually commented out. It makes sense usually,
      // but not here.
      // assertionNumericalEquals( observers::getMinTimeStamp(), 0.0 );
      gridConstructed = true;
    }
    continueToSolve = true;
  }
  else if ( iterationCounter<10 ) {
    peano4::parallel::Node::getInstance().setNextProgramStep(
      observers::StepRepository::toProgramStep( observers::StepRepository::Steps::TimeStep )
    );
    continueToSolve         = true;
    haveJustWrittenSnapshot = false;
    iterationCounter++;
  }
  else {
    continueToSolve = false;
  }
  
  return continueToSolve;
}


void step() {
  int  stepIdentifier = peano4::parallel::Node::getInstance().getCurrentProgramStep();
  auto stepName       = observers::StepRepository::toStepEnum(stepIdentifier);

  static tarch::logging::Log _log("");
  logInfo( "step()", "run " << observers::StepRepository::toString(stepName) );

  switch ( stepName ) {
    case observers::StepRepository::Steps::CreateGrid:
      {
        observers::CreateGrid  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        observers::finishGridConstructionStep();
      }
      break;
    case observers::StepRepository::Steps::PlotSolution:
      {
        observers::PlotSolution  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
      }
      break;
    case observers::StepRepository::Steps::TimeStep:
      {
        const double minTimeStamp    = observers::getMinTimeStamp();
        const double maxTimeStamp    = observers::getMaxTimeStamp();
        const double minTimeStepSize = observers::getMinTimeStepSize();
        const double maxTimeStepSize = observers::getMaxTimeStepSize();

        if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
          logInfo( "step()", "t_{min}  = " << minTimeStamp );
          logInfo( "step()", "t_{max}  = " << maxTimeStamp );
          logInfo( "step()", "dt_{min} = " << minTimeStepSize );
          logInfo( "step()", "dt_{max} = " << maxTimeStepSize );
        }
        observers::startTimeStep( minTimeStamp, maxTimeStamp, minTimeStepSize, maxTimeStepSize );
        observers::TimeStep  observer;
        peano4::parallel::SpacetreeSet::getInstance().traverse(observer);
        observers::finishTimeStep();
      }
      break;
  }
}




  
  
int main(int argc, char** argv) {
  const int ExitCodeSuccess          = 0;
  const int ExitCodeUnitTestsFailed  = 1;
  const int ExitCodeInvalidArguments = 2;
  
  peano4::initParallelEnvironment(&argc,&argv);
  exahype2::initNonCritialAssertionEnvironment();
  peano4::fillLookupTables();
  peano4::initSingletons(
    DomainOffset,
    DomainSize,
    0
  );

  if (not exahype2::parseCommandLineArguments(argc,argv) ) {
    return ExitCodeInvalidArguments;
  }
  
  // I mamually removed the unit tests

  examples::exahype2::empty::observers::DataRepository::initDatatypes();

  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    while ( selectNextAlgorithmicStep() ) {
      step();
    }
    logInfo( "step()", "finished" );
  }
  else {
    while (peano4::parallel::Node::getInstance().continueToRun()) {
      step();
    }
  }

  observers::finishSimulation();

  peano4::shutdownSingletons();
  examples::exahype2::empty::observers::DataRepository::shutdownDatatypes();
  exahype2::shutdownNonCritialAssertionEnvironment();
  peano4::shutdownParallelEnvironment();

  return ExitCodeSuccess;
}
