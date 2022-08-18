// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_ORCHESTRATION_BackfillAndDeployRoundRobin_H_
#define _TARCH_MULTICORE_ORCHESTRATION_BackfillAndDeployRoundRobin_H_


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class BackfillAndDeployRoundRobin;
    }
  }
}


/**
 * This mapping holds back all tasks in the user-defined queues throughout the
 * BSP section, i.e. no tasks should become a native one at this time. It also
 * allows the code to fuse tasks immediately. After we've terminated the BSP
 * section, all pending tasks are mapped onto native ones (see case distinction
 * in realisation of getNumberOfTasksToHoldBack()).
 *
 * When we use the GPU, we always allow the code to fuse as many tasks as it
 * can, though we limited the minimum number of tasks to a hard-coded value.
 * If there are multiple GPUs, we toggle between those guys, i.e. we use the
 * GPUs in a round-robin fashion.
 */
class tarch::multicore::orchestration::BackfillAndDeployRoundRobin: public tarch::multicore::orchestration::Strategy {
  private:
    int  _minTasksToFuse;
    int  _nextDeviceToUse;
    bool _inBSPSection;

    void toggleDevice();
  public:
    BackfillAndDeployRoundRobin(
      int  minTasksToFuse
    );
    virtual ~BackfillAndDeployRoundRobin() = default;

    void startBSPSection() override;
    void endBSPSection()   override;
    int getNumberOfTasksToHoldBack() override;
    FuseInstruction getNumberOfTasksToFuseAndTargetDevice()     override;
    bool fuseTasksImmediatelyWhenSpawned() override;
    bool processPendingTasksWhileWaitingInBSPSection() override;
};

#endif
