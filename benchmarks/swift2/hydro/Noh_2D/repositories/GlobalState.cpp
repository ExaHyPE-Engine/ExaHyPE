#include <algorithm>

#include "GlobalState.h"

#include "peano4/grid/grid.h"
#include "peano4/parallel/SpacetreeSet.h"


#include "swift2/UserInterface.h"


#include "globaldata/hydroPart.h"
#include "vertexdata/hydroPartSet.h"



namespace benchmarks {namespace swift2 {namespace noh {namespace repositories {


tarch::logging::Log _log( "benchmarks::swift2::noh::repositories::GlobalState" );

toolbox::loadbalancing::RecursiveSubdivision              loadBalancer(new toolbox::loadbalancing::RecursiveSubdivision::DefaultConfiguration());
bool rerunPreviousGridSweep() {
  bool result = false;
  result |= globaldata::hydroPart::getSpecies().rerunPreviousGridSweep();
  return result;
}


double getMinTimeStamp() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::hydroPart::getSpecies().getMinTimeStamp() );
  return result;
}


double getMaxTimeStamp() {
  double result = 0.0;
  result = std::max( result, globaldata::hydroPart::getSpecies().getMaxTimeStamp() );
  return result;
}


double getMinCutOffRadius() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::hydroPart::getSpecies().getMinCutOffRadius() );
  return result;
}


double getMaxCutOffRadius() {
  double result = 0.0;
  result = std::max( result, globaldata::hydroPart::getSpecies().getMaxCutOffRadius() );
  return result;
}


double getMinTimeStepSize() {
  double result = std::numeric_limits<double>::max();
  result = std::min( result, globaldata::hydroPart::getSpecies().getMinTimeStepSize() );
  return result;
}


double getMaxTimeStepSize() {
  double result = 0.0;
  result = std::max( result, globaldata::hydroPart::getSpecies().getMaxTimeStepSize() );
  return result;
}


void startGridConstructionStep() {
  logTraceIn( "GridConstructionStep()" );
}


void startIntermediateStep() {
  logTraceIn( "IntermediateStep()" );
}


void startInitialisationStep() {
  vertexdata::hydroPartSet::clearReassignmentStatistics();
  vertexdata::hydroPartSet::clearParticleStateStatistics();

  if (not ::swift2::commandlinesettings::enableDynamicLoadBalancing() ) {
    loadBalancer.enable(false);
  }
  logTraceIn( "InitialisationStep()" );
}


void startSortingStep() {
  vertexdata::hydroPartSet::clearReassignmentStatistics();
  vertexdata::hydroPartSet::clearParticleStateStatistics();
  logTraceIn( "SortingStep()" );
}


void startTimeStep() {
  vertexdata::hydroPartSet::clearReassignmentStatistics();
  vertexdata::hydroPartSet::clearParticleStateStatistics();
  globaldata::hydroPart::getSpecies().startTimeStep();
  logTraceIn( "TimeStep()" );
}


void startPlotStep() {
  logTraceIn( "PlotStep()" );
}


void finishTimeStep() {
  globaldata::hydroPart::getSpecies().finishTimeStep();
  vertexdata::hydroPartSet::finishedTraversal();
  vertexdata::hydroPartSet::reduceAndPrintStatistics();

  loadBalancer.finishStep();

  logInfo("TimeStep()", "min mesh size=" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getMinH()(0) );
  logInfo("TimeStep()",
    "#local cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalRefinedCells() <<
    ")\t(unrefined/refined)"
   );
  logInfo("TimeStep()",
    "#remote cells=(" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteUnrefinedCells() <<
    "/" << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfRemoteRefinedCells() <<
    ")\t(unrefined/refined)"
   );

  #ifdef UseSmartMPI
  smartmpi::tick();
  #endif
  logTraceIn( "TimeStep()" );
}


void finishGridConstructionStep() {
  vertexdata::hydroPartSet::finishedTraversal();

  loadBalancer.finishStep();
  logTraceIn( "GridConstructionStep()" );
}


void finishInitialisationStep() {
  vertexdata::hydroPartSet::finishedTraversal();
  vertexdata::hydroPartSet::reduceAndPrintStatistics();

  loadBalancer.finishStep();
  logTraceIn( "InitialisationStep()" );
}


void finishSortingStep() {
  vertexdata::hydroPartSet::finishedTraversal();
  vertexdata::hydroPartSet::reduceAndPrintStatistics();

  loadBalancer.finishStep();
  logTraceIn( "SortingStep()" );
}


void finishPlotStep() {
  vertexdata::hydroPartSet::finishedTraversal();
  logTraceIn( "PlotStep()" );
}


void finishIntermediateStep() {
  vertexdata::hydroPartSet::finishedTraversal();
  logTraceIn( "IntermediateStep()" );
}

}}}}
