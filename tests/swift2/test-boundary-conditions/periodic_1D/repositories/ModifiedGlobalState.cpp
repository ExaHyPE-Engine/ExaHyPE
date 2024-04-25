#include <algorithm>

#include "GlobalState.h"
#include "Constants.h"

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"


#include "swift2/UserInterface.h"


#include "globaldata/GenericParticle.h"
#include "vertexdata/GenericParticleSet.h"


// #include "toolbox/loadbalancing/strategies/SpreadOutOnceGridStagnates.h"


namespace tests {namespace swift2 {namespace periodicBoundaryConditions1D {namespace repositories {


tarch::logging::Log _log( "tests::swift2::periodicBoundaryConditions1D::repositories::GlobalState" );

// toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates              loadBalancer(new toolbox::loadbalancing::DefaultConfiguration(2));
toolbox::loadbalancing::strategies::SpreadOutHierarchically              loadBalancer(new toolbox::loadbalancing::DefaultConfiguration());


bool rerunPreviousGridSweep() {
  bool result = false;
  result |= vertexdata::GenericParticleSet::getNumberOfDropsIntoHorizontalTreeDecomposition()>0;

  result |= globaldata::GenericParticle::getSpecies().rerunPreviousGridSweep();
  return result;
}


double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::GenericParticle::getSpecies().getMinTimeStamp() );
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  result = std::max( result, globaldata::GenericParticle::getSpecies().getMaxTimeStamp() );
  return result;
}


double getMinSearchRadius() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::GenericParticle::getSpecies().getMinSearchRadius() );
  return result;
}


double getMaxSearchRadius() {
  double result = 0.0;
  result = std::max( result, globaldata::GenericParticle::getSpecies().getMaxSearchRadius() );
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::GenericParticle::getSpecies().getMinTimeStepSize() );
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  result = std::max( result, globaldata::GenericParticle::getSpecies().getMaxTimeStepSize() );
  return result;
}


void startGridConstructionStep() {
  logTraceIn( "GridConstructionStep()" );
}


void startIntermediateStep() {
  logTraceIn( "IntermediateStep()" );

  vertexdata::GenericParticleSet::clearReassignmentStatistics();
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
  globaldata::GenericParticle::getSpecies().startTimeStep();

  vertexdata::GenericParticleSet::clearReassignmentStatistics();
  vertexdata::GenericParticleSet::clearParticleStateStatistics();
  logTraceOut( "startTimeStep()" );
}


void startPlotStep() {
  logTraceIn( "PlotStep()" );
}


void finishTimeStep() {
  logTraceIn( "finishTimeStep()" );

  globaldata::GenericParticle::getSpecies().finishTimeStep();
  vertexdata::GenericParticleSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  vertexdata::GenericParticleSet::reduceParticleStateStatistics();
  logInfo("finishTimeStep()", vertexdata::GenericParticleSet::printParticleStateStatistics() );

  static int previousNumberOfRemainingLocalParticles = -1;

  if (previousNumberOfRemainingLocalParticles<0) {
    previousNumberOfRemainingLocalParticles = vertexdata::GenericParticleSet::getNumberOfRemainingLocalParticles();
    logInfo( "finishTimeStep()", "will validate that simulations continues to host " << previousNumberOfRemainingLocalParticles << " particle(s)" );
  }

  if ( vertexdata::GenericParticleSet::getNumberOfRemainingLocalParticles() < previousNumberOfRemainingLocalParticles) {
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
  vertexdata::GenericParticleSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  loadBalancer.finishStep();
  logTraceOut( "GridConstructionStep()" );
}


void finishInitialisationStep() {
  vertexdata::GenericParticleSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::GenericParticleSet::reduceReassignmentStatistics();
  vertexdata::GenericParticleSet::reduceParticleStateStatistics();

  loadBalancer.finishStep();
  logTraceOut( "InitialisationStep()" );
}


void finishInitialConditionStep() {
  vertexdata::GenericParticleSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::GenericParticleSet::clearReassignmentStatistics();
  vertexdata::GenericParticleSet::clearParticleStateStatistics();

  loadBalancer.finishStep();
  logTraceOut( "InitialConditionStep()" );
}


void finishPlotStep() {
  vertexdata::GenericParticleSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
  logTraceOut( "PlotStep()" );
}


void finishIntermediateStep() {
  vertexdata::GenericParticleSet::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);

  vertexdata::GenericParticleSet::reduceReassignmentStatistics();
  if (vertexdata::GenericParticleSet::registeredAnyResorting()) {
    logInfo( "finishIntermediateStep()", vertexdata::GenericParticleSet::printReassignmentStatistics() );
  }
  logTraceOut( "IntermediateStep()" );
}

}}}}
