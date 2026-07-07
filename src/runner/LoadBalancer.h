// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/loadbalancing/LoadBalancer.h"

namespace runner {
  void initLoadBalancer(bool hasPeriodicBoundaryConditions);

  toolbox::loadbalancing::LoadBalancer& loadBalancer();
} // namespace runner
