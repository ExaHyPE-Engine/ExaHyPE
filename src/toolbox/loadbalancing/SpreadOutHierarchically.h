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
      SpreadEquallyOverAllThreads
    };

    static std::string toString( Action action );

    SpreadOutHierarchically(Configuration* configuration = new DefaultConfiguration(), bool deleteInDestructor = true);
    virtual ~SpreadOutHierarchically();

    void finishStep() override;
  private:
    static tarch::logging::Log  _log;

    int _stepsToWaitForNextLoadBalancingDecision;

    void updateState();
    void updateLoadBalancing();
    void triggerSplit( int numberOfCells, int targetRank );
    Action getAction() const;
    int getNumberOfSplitsOnLocalRank() const;
};

