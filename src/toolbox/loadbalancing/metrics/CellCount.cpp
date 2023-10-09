#include "CellCount.h"


#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::metrics::CellCount::CellCount() {

}


std::string toolbox::loadbalancing::metrics::CellCount::toString() const {
  return CostMetrics::toString( "cell-count" );
}


void toolbox::loadbalancing::metrics::CellCount::updateGlobalView() {
  _localRankWeight = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();
  CostMetrics::updateGlobalView();
}

double toolbox::loadbalancing::metrics::CellCount::getCostOfLocalTree(int spacetreeNumber) const {
  return peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(spacetreeNumber).getNumberOfLocalUnrefinedCells();
}
