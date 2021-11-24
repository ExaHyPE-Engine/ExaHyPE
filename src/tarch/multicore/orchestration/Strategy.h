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
     * Determines how many tasks have to be fused and on which target
     * device we should run the fused result. Therefore, the routine
     * returns a pair. The first one is the task count, and it is an
     * exact quantity, i.e. if you specify 4, you'll always have four
     * tasks fused. The second entry is a device identifier, i.e. an
     * abstract device count. The tasks still might ignore this instruction
     * (if a task can't run on a GPU, e.g., but you say GPU 10, then it
     * will ignore this fact), but it is an indicator. I usually
     * enumerate devices ascendingly with integers starting from 0, but
     * it is really up to your backend of choice to determine which numbers
     * to use and how they map onto physical devices.
     *
     * You can also return tarch::multicore::Task::Host to indicate that
     * this is a fused task that shall run on the host rather than a
     * device.
     */
    virtual std::pair<int,int> getNumberOfTasksToFuseAndTargetDevice() = 0;
};

#endif
