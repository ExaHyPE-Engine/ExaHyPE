#include "NoLoadBalancing.h"
#include "loadbalancing.h"

#include "tarch/Assertions.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing():
  NoLoadBalancing( nullptr, false ) {
}


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing(
  Configuration* configuration,
  bool deleteInDestructor
):
  AbstractLoadBalancing( configuration, deleteInDestructor ) {
  _state = State::SwitchedOff;
  _statistics.notifyOfStateChange(_state);
}


void toolbox::loadbalancing::NoLoadBalancing::finishStep() {
  _statistics.updateGlobalView();
  _blacklist.update();
}


void toolbox::loadbalancing::NoLoadBalancing::enable( [[maybe_unused]] bool value ) {
  AbstractLoadBalancing::enable(false);
}
