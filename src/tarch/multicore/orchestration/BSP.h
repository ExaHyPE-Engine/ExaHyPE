// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_BSP_H_
#define _TARCH_MULTICORE_ORCHESTRATION_BSP_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class BSP;
    }
  }
}


/**
 * BSP strategy to use solely BSP
 *
 * If you want to use sole BSP, you effectively switch off the tasking.
 * Technically, this is realised by a strategy which enqueues all tasks
 * that are spawned into the pending task queue. No tasks are handed
 * over to the actual back-end. Therefore, the tasks will be done
 * lazily upon demand within processPendingTasks().
 */
class tarch::multicore::orchestration::BSP: public tarch::multicore::orchestration::Strategy {
  public:
    virtual ~BSP() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    int getNumberOfTasksToFuse()     override;
};

#endif
