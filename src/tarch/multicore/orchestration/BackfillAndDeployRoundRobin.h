// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Strategy.h"

namespace tarch {
  namespace multicore {
    namespace orchestration {
      class BackfillAndDeployRoundRobin;
    } // namespace orchestration
  }   // namespace multicore
} // namespace tarch

/**
 * Backfill strategy with GPU offloading
 *
 * This mapping holds back all tasks in the user-defined queues throughout the
 * "active" BSP section, i.e. no tasks should become a native one directly when
 * it is spawned. getNumberOfTasksToHoldBack(int taskType) returns inf while one thread at
 * least is still traversing its subdomain.
 *
 * If a thread becomes idle while others still traverse their domain, it
 * processes tasks that have been spawned before. processPendingTasksWhileWaitingInBSPSection()
 * consequently returns true.
 *
 * If we have more tasks than the given threshold in this phase of the
 * computation, we deploy them to a GPU (see below). This behaviour is controlled
 * via fuseTasksImmediatelyWhenSpawned(int taskType), i.e. it kicks in directly when we
 * spawn the task and a set of tasks consequently might be deployed onto an
 * accelerator while we still run through the mesh. Due to this check, idling
 * threads throughout the mesh traversal will get the left-overs and compete
 * with the actual spawns who grabs the big chunks and deploys them to the GPU.
 *
 * After all traversal tasks have joined again, we map the remaining tasks in our
 * local queue all to native tasks.
 *
 *
 * ## GPU strategy
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
  /**
   * Set up orchestration
   *
   * By default, the first device the code will use is the host. After that,
   * it will start to deploy tasks in a round robin fashion. It is important
   * at this point that we do not ask the core object something, as the
   * Core singleton might not be configured yet.
   */
  BackfillAndDeployRoundRobin(int minTasksToFuse);
  virtual ~BackfillAndDeployRoundRobin() = default;

  virtual void            startBSPSection(int nestedParallelismLevel) override;
  virtual void            endBSPSection(int nestedParallelismLevel) override;
  virtual int             getNumberOfTasksToHoldBack(int taskType) override;
  virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice(int taskType) override;
  virtual bool            fuseTasksImmediatelyWhenSpawned(int taskType) override;
  virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType)
    override;
};
