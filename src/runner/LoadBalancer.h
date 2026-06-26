// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/loadbalancing/LoadBalancer.h"

namespace runner {
  /**
   * The single load balancer instance owned by the runtime. It is created by
   * runner::run() (which knows the periodic-boundary flag) and shared by the runner
   * phases/statistics and the generated repositories.
   */
  void initLoadBalancer(bool hasPeriodicBoundaryConditions);

  toolbox::loadbalancing::LoadBalancer& loadBalancer();
} // namespace runner
