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
 * Deploy all tasks to one GPU
 */
class tarch::multicore::orchestration::AllOnGPU: public tarch::multicore::orchestration::Strategy {
private:
  const int   _device;
  int         _deviceToUse;
  bool        _isInBspSection;
  const std::chrono::microseconds  _timeout;

  void toggleDevice();
public:
  static int constexpr RoundRobin = -1;

  /**
   * Initialise strategy
   *
   * @param device Either a nonnegative integer picking one device or
   *   RoundRobin.
   */
  AllOnGPU(int device, const std::chrono::microseconds& timeout = std::chrono::microseconds(100) );

  virtual void startBSPSection(int nestedParallelismLevel) override;
  virtual void endBSPSection(int nestedParallelismLevel) override;

  /**
   * Return how many tasks to fuse at least, at most and to which device
   * to deploy them.
   *
   * @return (-1,std::numeric_limits<int>::max(),0) within the BSP section,
   *   such that the lower limit is never exceeded. Return the device
   *   otherwise with a min size of 1.
   */
  virtual FuseInstruction fuse(int taskType) override;

  /**
   * No if we encounter nested parallelism.
   */
  virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType)
    override;

  virtual void updateFuseStatistics(
    int    taskType,
    int    queueSize,
    double averageNumberOfPendingReadyTasksPerProcessTaskExecution,
    double probabilityThatTaskIsCompleted
  ) override;
};
