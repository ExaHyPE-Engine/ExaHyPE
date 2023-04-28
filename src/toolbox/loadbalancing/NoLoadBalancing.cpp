#include "NoLoadBalancing.h"
#include "loadbalancing.h"

#include "tarch/Assertions.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing():
  AbstractLoadBalancing( nullptr, false ) {
}


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing(
  Configuration* configuration,
  bool deleteInDestructor
):
  AbstractLoadBalancing( configuration, deleteInDestructor ) {
}


void toolbox::loadbalancing::NoLoadBalancing::finishStep() {
}


