#include "Configuration.h"

#include "peano4/parallel/Node.h"

std::string toolbox::loadbalancing::Configuration::toString() const {
  return "<toString() undef>";
}

bool toolbox::loadbalancing::DefaultConfiguration::makeSplitDependOnMemory(State) {
  return false;
}

int toolbox::loadbalancing::DefaultConfiguration::getMaxLocalTreesPerRank(State) {
  return peano4::parallel::Node::MaxSpacetreesPerRank;
}

double toolbox::loadbalancing::DefaultConfiguration::getWorstCaseBalancingRatio(State) {
  return 0.9;
}

int toolbox::loadbalancing::DefaultConfiguration::getMinTreeSize(State) {
  return 0;
}
