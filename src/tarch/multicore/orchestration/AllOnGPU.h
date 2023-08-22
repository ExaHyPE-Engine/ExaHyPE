// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Strategy.h"

namespace tarch {
  namespace multicore {
    namespace orchestration {
      class AllOnGPU;
    } // namespace orchestration
  }   // namespace multicore
} // namespace tarch

/**
 * Deploy all tasks to the GPU. Hold back all tasks in local queues until we hit
 * the end of the BSP section. After that, let the code deploy all the tasks onto
 * a GPU in one big rush, i.e. the orchestration strategy deploys all GPU tasks
 * onto one GPU only.
 */
class tarch::multicore::orchestration::AllOnGPU: public tarch::multicore::orchestration::Strategy {
private:
  const int _device;
  bool      _isInBspSection;

public:
  AllOnGPU(int device);

  virtual void startBSPSection(int nestedParallelismLevel) override;
  virtual void endBSPSection(int nestedParallelismLevel) override;

  /**
   * Within a BSP section, I hold all tasks back. Afterwards, I release all
   * of them.
   */
  virtual int getNumberOfTasksToHoldBack(int taskType) override;

  /**
   * Return how many tasks to fuse at least, at most and to which device
   * to deploy them.
   *
   * @return (-1,std::numeric_limits<int>::max(),0) within the BSP section,
   *   such that the lower limit is never exceeded. Return the device
   *   otherwise with a min size of 1.
   */
  virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice(int taskType) override;

  /**
   * No
   */
  virtual bool fuseTasksImmediatelyWhenSpawned(int taskType) override;

  /**
   * No if we encounter nested parallelism.
   */
  virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType)
    override;
};
