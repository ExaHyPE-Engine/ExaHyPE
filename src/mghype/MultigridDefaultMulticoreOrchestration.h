// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/orchestration/Strategy.h"

#include <set>

namespace mghype {
  class MultigridDefaultMulticoreOrchestration;
}

/**
 * Dummy configuration of MGHyPE
 *
 * The "trick" with the set if described on @ref documentation_tarch_task_fusion_strategies.
 */
class mghype::MultigridDefaultMulticoreOrchestration: public tarch::multicore::orchestration::Strategy {
  private:
    bool        _isInBspSection;

    std::set< int >  _residualComputationTasks;
    std::set< int >  _operatorInversionTasks;
  public:
    MultigridDefaultMulticoreOrchestration();

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
    virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType) override;

    virtual void updateFuseStatistics(
      int    taskType,
      int    queueSize,
      double averageNumberOfPendingReadyTasksPerProcessTaskExecution
    ) override;
};
