#include "CustomCellWeight.h"

#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::metrics::CustomCellWeight::CustomCellWeight() {

}


std::string toolbox::loadbalancing::metrics::CustomCellWeight::toString() const {
  return CostMetrics::toString( "custom-cell-weight" );
}


void toolbox::loadbalancing::metrics::CustomCellWeight::updateGlobalView() {
  _localRankWeight = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().getNumberOfLocalUnrefinedCells();
  CostMetrics::updateGlobalView();
}

double toolbox::loadbalancing::metrics::CustomCellWeight::getCostOfLocalTree(int spacetreeNumber) const {
  return peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(spacetreeNumber).getNumberOfLocalUnrefinedCells();
}


