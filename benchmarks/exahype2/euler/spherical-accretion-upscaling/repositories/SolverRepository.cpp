#include <algorithm>

#include "SolverRepository.h"
#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"

#include "exahype2/LoadBalancingConfiguration.h"






namespace benchmarks {namespace exahype2 {namespace euler {namespace sphericalaccretionupscaling {namespace repositories {

int statisticsExchangeTag = tarch::mpi::Rank::reserveFreeTag("SolverRepository - statistics exchange tag");

tarch::logging::Log _log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::repositories::SolverRepository" );

::exahype2::RefinementControl  refinementControl;
::exahype2::PlotFilter         plotFilter;
peano4::grid::GridStatistics   gridStatisticsAfterGridConstruction;


toolbox::loadbalancing::RecursiveSubdivision              loadBalancer(new ::exahype2::LoadBalancingConfiguration());
SelfSimilarInfallFD4 InstanceOfSelfSimilarInfallFD4;


bool mayPlot() {
  bool result = true;
  result &= InstanceOfSelfSimilarInfallFD4.mayPlot();
  return result;
}


bool isFirstGridSweepOfTimeStep() {
  bool result = true;
  result &= InstanceOfSelfSimilarInfallFD4.isFirstGridSweepOfTimeStep();
  return result;
}


bool isLastGridSweepOfTimeStep() {
  bool result = true;
  result &= InstanceOfSelfSimilarInfallFD4.isLastGridSweepOfTimeStep();
  return result;
}


double getMinTimeStamp(bool ofCurrentlyRunningGridSweep) {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfSelfSimilarInfallFD4.getMinTimeStamp(ofCurrentlyRunningGridSweep) );
  return result;
}


double getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) {
  double result = 0.0;
  result = std::max( result, InstanceOfSelfSimilarInfallFD4.getMaxTimeStamp(ofCurrentlyRunningGridSweep) );
  return result;
}


double getMinMeshSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfSelfSimilarInfallFD4.getMinMeshSize() );
  return result;
}


double getMaxMeshSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfSelfSimilarInfallFD4.getMaxMeshSize() );
  return result;
}


double getMaxAdmissibleMeshSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfSelfSimilarInfallFD4.MaxAdmissibleCellH );
  return result;
}


double getMinAdmissibleMeshSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfSelfSimilarInfallFD4.MinAdmissibleCellH );
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfSelfSimilarInfallFD4.getMinTimeStepSize() );
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfSelfSimilarInfallFD4.getMaxTimeStepSize() );
  return result;
}


void startGridConstructionStep() {
  InstanceOfSelfSimilarInfallFD4.startGridConstructionStep();
  logTraceIn( "GridConstructionStep()" );
}


void startGridInitialisationStep() {
  InstanceOfSelfSimilarInfallFD4.startGridInitialisationStep();
  logTraceIn( "GridInitialisationStep()" );
}


void startTimeStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  InstanceOfSelfSimilarInfallFD4.startTimeStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
  logTraceIn( "TimeStep()" );
}


void startPlottingStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  InstanceOfSelfSimilarInfallFD4.startPlottingStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
  logTraceIn( "PlottingStep()" );
}


void finishTimeStep() {
  InstanceOfSelfSimilarInfallFD4.finishTimeStep();

  

  refinementControl.finishStep();
  loadBalancer.finishStep();

  #ifdef UseSmartMPI
  smartmpi::tick();
  #endif
  logTraceOut( "TimeStep()" );
}


void finishGridConstructionStep() {
  InstanceOfSelfSimilarInfallFD4.finishGridConstructionStep();

  

  refinementControl.finishStep();
  loadBalancer.finishStep();

  gridStatisticsAfterGridConstruction = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  #ifdef Parallel
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++ ) {
      if (rank!=tarch::mpi::Rank::getGlobalMasterRank()) {
        peano4::grid::GridStatistics rankLocalStats;
        logDebug( "GridConstructionStep()", "receive grid statistics from rank " << rank << " via tag " << statisticsExchangeTag );
        peano4::grid::GridStatistics::receive(
          rankLocalStats,
          rank,
          statisticsExchangeTag,
          [&]() {
            tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
            tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
          },
          [&]() {
            tarch::mpi::Rank::getInstance().writeTimeOutWarning( "", "GridConstructionStep()", rank, statisticsExchangeTag );
            tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "", "receifinishGridConstructionStepve()", rank, statisticsExchangeTag );
            tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
          },
          tarch::mpi::Rank::getInstance().getCommunicator()
        );
        gridStatisticsAfterGridConstruction = gridStatisticsAfterGridConstruction + rankLocalStats;
      }
    }
  }
  else {
    logDebug( "GridConstructionStep()", "send out local statistics " << gridStatisticsAfterGridConstruction.toString() << " to global master via tag " << statisticsExchangeTag );
    peano4::grid::GridStatistics::send(
      gridStatisticsAfterGridConstruction,
      tarch::mpi::Rank::getInstance().getGlobalMasterRank(),
      statisticsExchangeTag,
      [&]() {
        tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
        tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
      },
      [&]() {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "", "receive()", tarch::mpi::Rank::getInstance().getGlobalMasterRank(), statisticsExchangeTag );
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "", "receive()", tarch::mpi::Rank::getInstance().getGlobalMasterRank(), statisticsExchangeTag );
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      },
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
  #endif
  logTraceOut( "GridConstructionStep()" );
}


void finishGridInitialisationStep() {
  InstanceOfSelfSimilarInfallFD4.finishGridInitialisationStep();

  

  loadBalancer.finishStep();
  logTraceOut( "GridInitialisationStep()" );
}


void finishPlottingStep() {
  InstanceOfSelfSimilarInfallFD4.finishPlottingStep();

  

  plotFilter.finishPlottingStep();
  logTraceOut( "PlottingStep()" );
}


void startSimulation() {
  
  InstanceOfSelfSimilarInfallFD4.startSimulation();

}


void finishSimulation() {
  InstanceOfSelfSimilarInfallFD4.finishSimulation();
  loadBalancer.finishStep();
  refinementControl.finishStep();
}


}}}}}
