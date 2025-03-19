// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/parallel/Node.h"
#include "tarch/logging/Log.h"
#include "toolbox/loadbalancing/AbstractLoadBalancing.h"
#include "toolbox/loadbalancing/metrics/CellCount.h"

#include <map>

namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      class SpreadOut;
    } // namespace strategies
  }   // namespace loadbalancing
} // namespace toolbox

class toolbox::loadbalancing::strategies::SpreadOut: public toolbox::loadbalancing::AbstractLoadBalancing {
public:
  SpreadOut(
    Configuration* configuration = new DefaultConfiguration(),
    CostMetrics*   costMetrics   = new toolbox::loadbalancing::metrics::CellCount()
  );
  virtual ~SpreadOut() override = default;

  virtual void finishStep() override;

private:
  static tarch::logging::Log _log;

  int _maxPermittedTreesPerRank;

  void updateInterRankLoadBalancing();
  void updateIntraRankLoadBalancing();

  void triggerSplit(int numberOfCells, int targetRank);
};
