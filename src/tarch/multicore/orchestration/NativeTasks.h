// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_NATIVE_TASKS_H_
#define _TARCH_MULTICORE_ORCHESTRATION_NATIVE_TASKS_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class NativeTasks;
    }
  }
}


/**
 * Use native tasking
 *
 * Native tasking means that we run Peano 4 such that all Peano tasks are
 * immediately mapped onto native tasks. In other words: Not a single task
 * is held back. All of the tasks are immediately piped through to the
 * task system.
 *
 * The realisation thus just returns some constants: 0 for the number of
 * tasks to hold back, and, obviously, also zero for the number of fused
 * tasks, as no tasks at all are fused.
 */
class tarch::multicore::orchestration::NativeTasks: public tarch::multicore::orchestration::Strategy {
  public:
    virtual ~NativeTasks() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    std::pair<int,int> getNumberOfTasksToFuseAndTargetDevice() override;
};

#endif
