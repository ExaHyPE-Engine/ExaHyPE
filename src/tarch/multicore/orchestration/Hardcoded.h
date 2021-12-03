// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_HARDCODED_H_
#define _TARCH_MULTICORE_ORCHESTRATION_HARDCODED_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Hardcoded;
    }
  }
}


/**
 * A hard coded strategy that can realise a few standard tasking patterns
 *
 *
 *
 */
class tarch::multicore::orchestration::Hardcoded: public tarch::multicore::orchestration::Strategy {
  private:
    int  _numberOfTasksToHoldBack;
    int  _tasksToFuse;
    int  _deviceForFusedTasks;
    bool _fuseTasksImmediatelyWhenSpawned;
    bool _processPendingTasksWhileWaiting;
  public:
    /**
     * If you want to use sole BSP, you effectively switch off the tasking.
     * Technically, this is realised by a strategy which enqueues all tasks
     * that are spawned into the pending task queue. No tasks are handed
     * over to the actual back-end. Therefore, the tasks will be done
     * lazily upon demand within processPendingTasks().
     */
    static Hardcoded* createBSP();

    /**
     * Native tasking means simply that we do not hold back any tasks but
     * immediately map them onto native tasks.
     */
    static Hardcoded* createNative();

    /**
     * Backfill strategy from the IWOMP paper.
     */
    static Hardcoded* createBackfill();

    /**
     * Create a strategy where tasks are always fused if possible given the
     * configuration constraints.
     *
     * @param numberOfTasksToFuse The remaining tasks(<numberOfTasksToFuse) will
     *   remain stuck in the background queue and will stay there until processed
     *   lazily.
     * @param fuseImmediately Fuse right when they are spawned. Otherwise, tasks
     *   end up in a local queue. If a thread runs out of work, it looks into this
     *   queue and then fuses. So the fuse happens later, but it does not hold
     *   back any task production thread.
     * @param targetDevice Non-negative number or tarch::multicore::Task::Host
     */
    static Hardcoded* createFuseAll(int numberOfTasksToFuse, bool fuseImmediately, int targetDevice);

    Hardcoded(
      int  numberOfTasksToHoldBack,
      int  tasksToFuse,
      int  deviceForFusedTasks,
      bool fuseTasksImmediatelyWhenSpawned,
      bool processPendingTasksWhileWaiting
    );
    virtual ~Hardcoded() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    std::pair<int,int> getNumberOfTasksToFuseAndTargetDevice()     override;
    bool fuseTasksImmediatelyWhenSpawned() override;
    bool processPendingTasksWhileWaitingInBSPSection() override;
};

#endif
