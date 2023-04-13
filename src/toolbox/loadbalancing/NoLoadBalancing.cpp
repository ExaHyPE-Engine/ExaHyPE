#include "NoLoadBalancing.h"
#include "loadbalancing.h"

#include "tarch/Assertions.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing():
  LoadBalancing( nullptr, false ) {
}


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing(
  Configuration* configuration,
  bool deleteInDestructor
):
  LoadBalancing( configuration, deleteInConstructor ) {
}


void toolbox::loadbalancing::NoLoadBalancing::finishStep() {
}


bool toolbox::loadbalancing::NoLoadBalancing::hasSplitRecently() const {
  return false;
}


void toolbox::loadbalancing::NoLoadBalancing::enable(bool value) {
}


bool toolbox::loadbalancing::NoLoadBalancing::isEnabled(bool globally) const {
  return false;
}


int toolbox::loadbalancing::NoLoadBalancing::getGlobalNumberOfTrees() const {
  return -1;
}


