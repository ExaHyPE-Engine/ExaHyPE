#include "SolverRepository.h"

#include <algorithm>

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"



namespace examples {namespace exahype2 {namespace loh1 {namespace observers {

int statisticsExchangeTag = tarch::mpi::Rank::reserveFreeTag("SolverRepository - statistics exchange tag");

tarch::logging::Log _log( "SolverRepository" );

::exahype2::RefinementControl  refinementControl;



peano4::grid::GridStatistics   gridStatisticsAfterGridConstruction;


toolbox::loadbalancing::RecursiveSubdivision              loadBalancer(0.9);
LOH1 InstanceOfLOH1;



double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfLOH1.getMinTimeStamp() );
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  result = std::max( result, InstanceOfLOH1.getMaxTimeStamp() );
  return result;
}


double getMinMeshSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfLOH1.getMinMeshSize() );
  return result;
}


double getMaxMeshSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfLOH1.getMaxMeshSize() );
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, InstanceOfLOH1.getMinTimeStepSize() );
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  result = std::max( result, InstanceOfLOH1.getMaxTimeStepSize() );
  return result;
}


void startGridConstructionStep() {
  InstanceOfLOH1.startGridConstructionStep();
}


void startGridInitialisationStep() {
  InstanceOfLOH1.startGridInitialisationStep();
}


void startTimeStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  InstanceOfLOH1.startTimeStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
}


void startPlottingStep(
  double minTimeStamp,
  double maxTimeStamp,
  double minTimeStepSize,
  double maxTimeStepSize
) {
  InstanceOfLOH1.startPlottingStep(minTimeStamp,maxTimeStamp,minTimeStepSize,maxTimeStepSize);
}


void finishTimeStep() {
  InstanceOfLOH1.finishTimeStep();

  refinementControl.finishStep();
  loadBalancer.finishStep();
}


void finishGridConstructionStep() {
  InstanceOfLOH1.finishGridConstructionStep();

  refinementControl.finishStep();
  loadBalancer.finishStep();

  gridStatisticsAfterGridConstruction = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  #ifdef Parallel
  if ( tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++ ) {
      if (rank!=tarch::mpi::Rank::getGlobalMasterRank()) {
        peano4::grid::GridStatistics rankLocalStats;
        logDebug( "finishGridConstructionStep()", "receive grid statistics from rank " << rank << " via tag " << statisticsExchangeTag );
        peano4::grid::GridStatistics::receive(
          rankLocalStats,
          rank,
          statisticsExchangeTag,
          tarch::mpi::Rank::getInstance().getCommunicator()
        );
        gridStatisticsAfterGridConstruction = gridStatisticsAfterGridConstruction + rankLocalStats;
      }
    }
  }
  else {
    logDebug( "finishGridConstructionStep()", "send out local statistics " << gridStatisticsAfterGridConstruction.toString() << " to global master via tag " << statisticsExchangeTag );
    peano4::grid::GridStatistics::send(
      gridStatisticsAfterGridConstruction,
      tarch::mpi::Rank::getInstance().getGlobalMasterRank(),
      statisticsExchangeTag,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
  #endif
}


void finishGridInitialisationStep() {
  InstanceOfLOH1.finishGridInitialisationStep();
}


void finishPlottingStep() {
  InstanceOfLOH1.finishPlottingStep();
}


void finishSimulation() {
  refinementControl.finishStep();
}


}}}}
