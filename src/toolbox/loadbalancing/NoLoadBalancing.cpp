#include "NoLoadBalancing.h"
#include "loadbalancing.h"

#include "tarch/Assertions.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing() {
}


void toolbox::loadbalancing::NoLoadBalancing::finishStep() {
}


void toolbox::loadbalancing::NoLoadBalancing::dumpStatistics() {
}


void toolbox::loadbalancing::NoLoadBalancing::finishSimulation() {
}


bool toolbox::loadbalancing::NoLoadBalancing::hasSplitRecently() const {
  return false;
}


void toolbox::loadbalancing::NoLoadBalancing::enable(bool value) {
}
