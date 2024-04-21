#include <algorithm>

#include "GlobalState.h"
#include "Constants.h"

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"


#include "swift2/UserInterface.h"


#include "globaldata/HydroPart.h"
#include "vertexdata/HydroPartSet.h"


#include "toolbox/loadbalancing/strategies/SpreadOutOnceGridStagnates.h"


namespace benchmarks {namespace swift2 {namespace noh {namespace repositories {


tarch::logging::Log _log( "benchmarks::swift2::noh::repositories::GlobalState" );

toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates              loadBalancer(new toolbox::loadbalancing::DefaultConfiguration(2));


bool rerunPreviousGridSweep() {
  bool result = false;
  result |= vertexdata::HydroPartSet::getNumberOfDropsIntoHorizontalTreeDecomposition()>0;

  result |= globaldata::HydroPart::getSpecies().rerunPreviousGridSweep();
  return result;
}


double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::HydroPart::getSpecies().getMinTimeStamp() );
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  result = std::max( result, globaldata::HydroPart::getSpecies().getMaxTimeStamp() );
  return result;
}


double getMinSearchRadius() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::HydroPart::getSpecies().getMinSearchRadius() );
  return result;
}


double getMaxSearchRadius() {
  double result = 0.0;
  result = std::max( result, globaldata::HydroPart::getSpecies().getMaxSearchRadius() );
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::HydroPart::getSpecies().getMinTimeStepSize() );
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  result = std::max( result, globaldata::HydroPart::getSpecies().getMaxTimeStepSize() );
  return result;
}


void startGridConstructionStep() {
  logTraceIn( "GridConstructionStep()" );
}


void startIntermediateStep() {
  logTraceIn( "IntermediateStep()" );

  vertexdata::HydroPartSet::clearReassignmentStatistics();
}


void startInitialConditionStep() {
  if (not ::swift2::commandlinesettings::enableDynamicLoadBalancing() ) {
    loadBalancer.enable(false);
  }
  logTraceIn( "InitialConditionStep()" );
}


void startInitialisationStep() {
  if (not ::swift2::commandlinesettings::enableDynamicLoadBalancing() ) {
    loadBalancer.enable(false);
  }
  logTraceIn( "InitialisationStep()" );
}


void startTimeStep() {
  logTraceIn( "startTimeStep()" );
  globaldata::HydroPart::getSpecies().startTimeStep();

  vertexdata::HydroPartSet::clearReassignmentStatistics();
  vertexdata::HydroPartSet::clearParticleStateStatistics();
  logTraceOut( "startTimeStep()" );
}


void startPlotStep() {
  logTraceIn( "PlotStep()" );
}


void finishTimeStep() {
  logTraceIn( "finishTimeStep()" );

  globaldata::HydroPart::getSpecies().finishTimeStep();
  vertexdata::HydroPartSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  vertexdata::HydroPartSet::reduceParticleStateStatistics();
  logInfo("finishTimeStep()", vertexdata::HydroPartSet::printParticleStateStatistics() );

  static int previousNumberOfRemainingLocalParticles = -1;

  if (previousNumberOfRemainingLocalParticles<0) {
    previousNumberOfRemainingLocalParticles = vertexdata::HydroPartSet::getNumberOfRemainingLocalParticles();
    logInfo( "finishTimeStep()", "will validate that simulations continues to host " << previousNumberOfRemainingLocalParticles << " particle(s)" );
  }

  if ( vertexdata::HydroPartSet::getNumberOfRemainingLocalParticles() < previousNumberOfRemainingLocalParticles) {
    toolbox::particles::assignmentchecks::eliminateExistingParticles();
    toolbox::particles::assignmentchecks::ensureDatabaseIsEmpty();
    logError( "finishTimeStep()", "seems we have lost particles" );
    exit(-1);
  }


  loadBalancer.finishStep();

  logInfo("finishTimeStep()", "min mesh size=" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH()(0) );
  logInfo("finishTimeStep()",
    "#local cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalRefinedCells() <<
    ")\t(unrefined/refined)"
   );
  logInfo("finishTimeStep()",
    "#remote cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteRefinedCells() <<
    ")\t(unrefined/refined)"
   );

  #ifdef UseSmartMPI
  smartmpi::tick();
  #endif
  logTraceOut( "finishTimeStep()" );
}


void finishGridConstructionStep() {
  vertexdata::HydroPartSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  loadBalancer.finishStep();
  logTraceOut( "GridConstructionStep()" );
}


void finishInitialisationStep() {
  vertexdata::HydroPartSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::HydroPartSet::reduceReassignmentStatistics();
  vertexdata::HydroPartSet::reduceParticleStateStatistics();

  loadBalancer.finishStep();
  logTraceOut( "InitialisationStep()" );
}


void finishInitialConditionStep() {
  vertexdata::HydroPartSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::HydroPartSet::clearReassignmentStatistics();
  vertexdata::HydroPartSet::clearParticleStateStatistics();

  loadBalancer.finishStep();
  logTraceOut( "InitialConditionStep()" );
}


void finishPlotStep() {
  vertexdata::HydroPartSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  logTraceOut( "PlotStep()" );
}


void finishIntermediateStep() {
  vertexdata::HydroPartSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::HydroPartSet::reduceReassignmentStatistics();
  if (vertexdata::HydroPartSet::registeredAnyResorting()) {
    logInfo( "finishIntermediateStep()", vertexdata::HydroPartSet::printReassignmentStatistics() );
  }
  logTraceOut( "IntermediateStep()" );
}

}}}}
