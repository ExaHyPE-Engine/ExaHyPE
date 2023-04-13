#include "Configuration.h"

#include "peano4/parallel/Node.h"


bool toolbox::loadbalancing::DefaultConfiguration::makeSplitDependOnMemory(State state) {
  return false;
}


int toolbox::loadbalancing::DefaultConfiguration::getMaxLocalTreesPerRank(State state) {
  return peano4::parallel::Node::MaxSpacetreesPerRank;
}


double toolbox::loadbalancing::DefaultConfiguration::getWorstCaseBalancingRatio(State state) {
  return 0.9;
}


int toolbox::loadbalancing::DefaultConfiguration::getMinTreeSize(State state) {
  return 0;
}

