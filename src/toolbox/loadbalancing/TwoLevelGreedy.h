// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_LOAD_BALANCING_TWO_LEVEL_GREEDY_
#define _TOOLBOX_LOAD_BALANCING_TWO_LEVEL_GREEDY_



#include "tarch/logging/Log.h"
#include "tarch/mpi/mpi.h"
#include <map>



namespace toolbox {
  namespace loadbalancing {
    class TwoLevelGreedy;
  }
}


/**
 * Greedy balancing with two levels of decision making
 *
 * We wait until there are enough cells to give at least one cell to each rank. After
 * that, we wait per rank until there are enough cells to give at least one cell to
 * each core. Then we terminate.
 */
class toolbox::loadbalancing::TwoLevelGreedy {
  public:
    /**
     *
     */
    TwoLevelGreedy();
    ~TwoLevelGreedy();

    /**
     * Triggers actual load balancing data exchange, triggers reblaancing, and
     * dumps statistics.
     */
    void finishStep();
    void finishSimulation();

    /**
     * I need the stats here mainly for debugging purposes. The load balancing
     * alread dumps information per time step. This routine however is more
     * elaborate/detailed and not used by default.
     */
    std::string toString() const;

    bool hasSplitRecently() const;

    /**
     * Switch on/off.
     */
    void enable(bool);
  private:
    /**
     * @see getStrategyStep()
     */
    enum class StrategyStep {
      Wait,
      SpreadEquallyOverAllRanks,
      SpreadEquallyOverAllCores,
      Completed
    };

    static std::string toString( StrategyStep step );

    /**
     * Analyse current global situation and commit to a strategy. This routine
     * analyses the rank state and returns an action.
     */
    StrategyStep getStrategyStep() const;

    /**
     * Is used by tree identification and either indicates that there are no trees
     * at all or means that the heaviest tree is on the blacklist. See implementation
     * remarks in class description.
     */
    static constexpr int NoHeaviestTreeAvailable = -1;

    static tarch::logging::Log  _log;

    bool _hasSpreadOutOverAllRanks;
    bool _hasSpreadOutOverAllCores;

    /**
     * 0 means no
     */
    int _hasSplitRecently;
};


#endif

