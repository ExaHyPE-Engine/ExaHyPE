#include "Configuration.h"

#include "peano4/parallel/Node.h"

std::string toolbox::loadbalancing::Configuration::toString() const {
  return "<toString() undef>";
}


toolbox::loadbalancing::DefaultConfiguration::DefaultConfiguration(int maxTreesPerRank):
  _maxTreesPerRank(maxTreesPerRank<=0 ? peano4::parallel::Node::MaxSpacetreesPerRank : maxTreesPerRank)
{}

bool toolbox::loadbalancing::DefaultConfiguration::makeSplitDependOnMemory(State) {
  return false;
}

int toolbox::loadbalancing::DefaultConfiguration::getMaxLocalTreesPerRank(State) {
  return _maxTreesPerRank;
}

double toolbox::loadbalancing::DefaultConfiguration::getWorstCaseBalancingRatio(State) {
  return 0.9;
}

int toolbox::loadbalancing::DefaultConfiguration::getMinTreeSize(State) {
  return 0;
}

peano4::SplitInstruction::Mode toolbox::loadbalancing::DefaultConfiguration::getMode(State) {
  return peano4::SplitInstruction::Mode::BottomUp;
}
