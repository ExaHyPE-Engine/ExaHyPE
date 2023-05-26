// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "Strategy.h"


namespace tarch {
  namespace multicore {
    namespace orchestration {
      class AllOnGPU;
    }
  }
}


/**
 * Deploy all tasks to the GPU. Hold back all tasks in local queues until we hit
 * the end of the BSP section. After that, let the code deploy all the tasks onto
 * a GPU in one big rush, i.e. the orchestration strategy deploys all GPU tasks
 * onto one GPU only.
 */
class tarch::multicore::orchestration::AllOnGPU: public tarch::multicore::orchestration::Strategy {
  private:
    const int  _device;
    bool       _isInBspSection;
  public:
    AllOnGPU(int device);

    virtual void startBSPSection() override;
    virtual void endBSPSection()   override;

    /**
     * Within a BSP section, I hold all tasks back. Afterwards, I release all
     * of them.
     */
    virtual int getNumberOfTasksToHoldBack() override;

    /**
     * Return how many tasks to fuse at least, at most and to which device
     * to deploy them.
     *
     * @return (-1,std::numeric_limits<int>::max(),0) within the BSP section,
     *   such that the lower limit is never exceeded. Return the device
     *   otherwise with a min size of 1.
     */
    virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice() override;

    /**
     * No
     */
    virtual bool fuseTasksImmediatelyWhenSpawned() override;

    /**
     * No
     */
    virtual ExecutionPolicy getExecutionPolicyForForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) override;

};

