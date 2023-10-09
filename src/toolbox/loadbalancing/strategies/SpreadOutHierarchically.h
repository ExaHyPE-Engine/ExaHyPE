// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "toolbox/loadbalancing/AbstractLoadBalancing.h"

#include "toolbox/loadbalancing/metrics/CellCount.h"


namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      class SpreadOutHierarchically;
    }
  }
}


/**
 * Spread out hiearchically
 *
 * First try to give each rank one tree and then make each rank fork out once
 * more such that each thread gets one tree. After that, switch into stagnation.
 *
 * The routine tries to get as many cores/ranks in as possible. Therefore, we
 * employ a bottom-up splitting strategy which prioritises fair fine grid
 * splits over an "efficient" tree decomposition. Consult a discussion of
 * splitting variants and constraints in peano4::grid::Spacetree::isCellTopDownSplitCandidate()
 * and  peano4::grid::Spacetree::isCellBottomUpSplitCandidate(),
 * peano4::grid::Spacetree::splitOrJoinCell() and notably the general
 * @ref peano_domain_decomposition "domain decomposition overview" for the
 * rationale.
 */
class toolbox::loadbalancing::strategies::SpreadOutHierarchically: public toolbox::loadbalancing::AbstractLoadBalancing {
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

    SpreadOutHierarchically(Configuration* configuration = new DefaultConfiguration(), CostMetrics*   costMetrics = new toolbox::loadbalancing::metrics::CellCount());
    virtual ~SpreadOutHierarchically();

    virtual void finishStep() override;

  private:
    static tarch::logging::Log  _log;

    int _stepsToWaitForNextLoadBalancingDecision;

    void updateState();
    void updateLoadBalancing();
    void triggerSplit( int numberOfCells, int targetRank );
    Action getAction() const;
    int getNumberOfSplitsOnLocalRank() const;
};

