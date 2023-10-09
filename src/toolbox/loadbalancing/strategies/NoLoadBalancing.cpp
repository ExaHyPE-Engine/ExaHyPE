#include "NoLoadBalancing.h"
#include "loadbalancing.h"
#include "toolbox/loadbalancing/metrics/CellCount.h"

#include "tarch/Assertions.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing():
  NoLoadBalancing( nullptr, nullptr ) {
}


toolbox::loadbalancing::NoLoadBalancing::NoLoadBalancing(
  Configuration* configuration,
  CostMetrics*   costMetrics
):
  AbstractLoadBalancing( configuration, costMetrics ) {
  _state = State::SwitchedOff;
  _statistics.notifyOfStateChange(_state);
}


void toolbox::loadbalancing::NoLoadBalancing::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();
}


void toolbox::loadbalancing::NoLoadBalancing::enable( [[maybe_unused]] bool value ) {
  AbstractLoadBalancing::enable(false);
}
