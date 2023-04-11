// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "LoadBalancing.h"


namespace toolbox {
  namespace loadbalancing {
    class SpreadOutHierarchically;
  }
}


class toolbox::loadbalancing::SpreadOutHierarchically: public toolbox::loadbalancing::LoadBalancing {
  public:
    /**
     * @see getAction()
     */
    enum class Action {
      /**
       * Required for the strategy.
       */
      Unspecified,
      None,
      SpreadEquallyOverAllRanks,
      SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning
    };

    static std::string toString( Action action );

    SpreadOutHierarchically(Configuration* configuration = new DefaultConfiguration());
    virtual ~SpreadOutHierarchically();
  private:
    static tarch::logging::Log  _log;

    int _stepsToWaitForNextLoadBalancingDecision;
};

