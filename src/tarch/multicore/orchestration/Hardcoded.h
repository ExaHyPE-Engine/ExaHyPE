// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Strategy.h"

namespace tarch {
  namespace multicore {
    namespace orchestration {
      class Hardcoded;
    } // namespace orchestration
  }   // namespace multicore
} // namespace tarch

/**
 * A hard coded strategy that can realise a few standard tasking patterns
 *
 *
 *
 */
class tarch::multicore::orchestration::Hardcoded: public tarch::multicore::orchestration::Strategy {
private:
  int  _numberOfTasksToHoldBack;
  int  _minTasksToFuse;
  int  _maxTasksToFuse;
  int  _deviceForFusedTasks;
  bool _fuseTasksImmediatelyWhenSpawned;
  int  _maxNestedConcurrency;

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
   * Fall back to native tasking
   *
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
   * @param targetDevice Non-negative number or tarch::multicore::Task::Host.
   */
  static Hardcoded* createFuseAll(
    int numberOfTasksToFuse, bool fuseImmediately, bool processTasksWhileWaitingInBSPArea, int targetDevice
  );

  Hardcoded(
    int  numberOfTasksToHoldBack,
    int  minTasksToFuse,
    int  maxTasksToFuse,
    int  deviceForFusedTasks,
    bool fuseTasksImmediatelyWhenSpawned,
    int  maxNestedConcurrency
  );
  virtual ~Hardcoded() = default;

  virtual void            startBSPSection(int nestedParallelismLevel) override;
  virtual void            endBSPSection(int nestedParallelismLevel) override;
  virtual int             getNumberOfTasksToHoldBack(int taskType) override;
  virtual FuseInstruction getNumberOfTasksToFuseAndTargetDevice(int taskType) override;
  virtual bool            fuseTasksImmediatelyWhenSpawned(int taskType) override;
  virtual ExecutionPolicy paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int taskType)
    override;
};
