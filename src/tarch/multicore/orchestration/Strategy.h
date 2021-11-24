// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_STRATEGY_H_
#define _TARCH_MULTICORE_ORCHESTRATION_STRATEGY_H_


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Strategy;
    }
  }
}


#include <utility>


/**
 * Interface for any task orchestration factory
 */
class tarch::multicore::orchestration::Strategy {
  public:
    virtual ~Strategy() = default;

    /**
     * Notifies the strategy that we enter a BSP section
     */
    virtual void startBSPSection() = 0;
    virtual void endBSPSection()   = 0;

    /**
     * Tell the runtime system how many tasks to hold back: If there
     * are more tasks than the result, the tasking system will map them
     * onto native tasks. As long as we have fewer tasks than this
     * number, the runtime system will store tasks in its internal
     * queue and not pass them on. Holding tasks back gives us the
     * opportunity to fuse tasks, and it reduces pressure from the
     * underlying task system. It also is an implicit priorisation, i.e.
     * tasks that we hold back are ready yet have low priority.
     *
     * Is not const, as I want strategies give the opportunity to adopt
     * decisions after each call.
     */
    virtual int getNumberOfTasksToHoldBack() = 0;

    /**
     *
     */
    virtual std::pair<int,int> getNumberOfTasksToFuseAndTargetDevice() = 0;
};

#endif
