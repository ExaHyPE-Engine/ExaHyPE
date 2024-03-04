#include "../strategies/NoLoadBalancing.h"

#include "tarch/Assertions.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "toolbox/loadbalancing/loadbalancing.h"
#include "toolbox/loadbalancing/metrics/CellCount.h"


toolbox::loadbalancing::strategies::NoLoadBalancing::NoLoadBalancing():
  NoLoadBalancing(nullptr, nullptr) {}


toolbox::loadbalancing::strategies::NoLoadBalancing::NoLoadBalancing(
  Configuration* configuration, CostMetrics* costMetrics
):
  AbstractLoadBalancing(configuration, costMetrics) {
  _state = State::SwitchedOff;
  _statistics.notifyOfStateChange(_state);
}


void toolbox::loadbalancing::strategies::NoLoadBalancing::finishStep() {
  _statistics.updateGlobalView();
  if (_costMetrics != nullptr)
    _costMetrics->updateGlobalView();
  _blacklist.update();
}


void toolbox::loadbalancing::strategies::NoLoadBalancing::enable([[maybe_unused]] bool value) {
  AbstractLoadBalancing::enable(false);
}
