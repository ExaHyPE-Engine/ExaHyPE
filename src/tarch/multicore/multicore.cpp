// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "multicore.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#include "topology/topologies.h"
#endif

#include "tarch/multicore/otter.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/taskfusion/taskfusion.h"

#include "tarch/logging/Statistics.h"


namespace {
  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log("tarch::multicore");

  /**
   * Basically filter out the values
   *
   * - ExecuteImmediately
   * - NoOutDependencies
   *
   */
  std::set<tarch::multicore::TaskNumber> prune(const std::set<tarch::multicore::TaskNumber>& inDependencies) {
    std::set<tarch::multicore::TaskNumber> result;
    for (auto& p: inDependencies) {
      if (p>=0) result.insert(p);
    }
    return result;
  }
}


const std::string tarch::multicore::SpawnedTasksStatisticsIdentifier("tarch::multicore::SpawnedTasksStatisticsIdentifier");
const std::string tarch::multicore::SpawnedFusableTasksStatisticsIdentifier("tarch::multicore::SpawnedFusableTasksStatisticsIdentifier");
const std::string tarch::multicore::BSPConcurrencyLevelStatisticsIdentifier("tarch::multicore::BSPConcurrencyLevelStatisticsIdentifier");


std::string tarch::multicore::toString( const std::set<TaskNumber>& taskNumbers ) {
  if ( taskNumbers.empty() ) {
    return "{<empty>}";
  }
  else if ( taskNumbers.size()==1 ) {
    return "{" + std::to_string( *taskNumbers.begin() ) + "}";
  }
  else {
    std::ostringstream msg;
    msg << "{";
    for (auto p: taskNumbers) {
      if (p!=*taskNumbers.begin()) {
        msg << ",";
      }
      msg << p;
    }
    msg << "}";
    return msg.str();
  }
}


void tarch::multicore::initSmartMPI() {
  #ifdef UseSmartMPI
  using namespace smartmpi::topology;
  typedef UseSmartMPI MyTopology;
  smartmpi::topology::Topology* smartMPITopology = new MyTopology(
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  smartmpi::init( smartMPITopology );

  smartmpi::appendToSchedulerChain( "HardcodedMigrationScheduler" );

  tarch::mpi::Rank::getInstance().setCommunicator( smartMPITopology->computeNodeOrSmartServerCommunicator );
  #endif
}


void tarch::multicore::shutdownSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::shutdown();
  #endif
}


void tarch::multicore::setOrchestration(tarch::multicore::orchestration::Strategy* realisation) {
  assertion(orchestrationStrategy != nullptr);
  assertion(realisation != nullptr);

  delete orchestrationStrategy;
  orchestrationStrategy = realisation;
}


tarch::multicore::orchestration::Strategy* tarch::multicore::swapOrchestration(tarch::multicore::orchestration::Strategy* realisation) {
  assertion(orchestrationStrategy != nullptr);
  assertion(realisation != nullptr);

  tarch::multicore::orchestration::Strategy* result = orchestrationStrategy;
  orchestrationStrategy                             = realisation;
  return result;
}


tarch::multicore::orchestration::Strategy& tarch::multicore::getOrchestration() {
  return *orchestrationStrategy;
}


void tarch::multicore::waitForTasks(const std::set<TaskNumber>& inDependencies, TaskWaitType taskWaitType) {
  if (not inDependencies.empty()) {
    std::set<TaskNumber> prunedDependencies = prune(inDependencies);
    switch (taskWaitType) {
      case TaskWaitType::Mixed:
        tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks(prunedDependencies);
        tarch::multicore::native::waitForTasks(prunedDependencies);
        break;
      case TaskWaitType::ExclusivelyFusedTasks:
        {
          bool checkNative = tarch::multicore::taskfusion::ensureHeldBackTasksAreMappedOntoNativeTasks(prunedDependencies);
          if (checkNative) {
            logDebug( "waitForTasks(...)", "have to tell runtime to wait for tasks " << toString(inDependencies) );
            tarch::multicore::native::waitForTasks(prunedDependencies);
          }
          else {
            logDebug( "waitForTasks(...)", "checked all fused tasks and will skip native ones" );
          }
        }
        break;
      case TaskWaitType::OnlyNativeTasks:
        tarch::multicore::native::waitForTasks(prunedDependencies);
        break;
    }
  }
}


void tarch::multicore::waitForTask(const int taskNumber, TaskWaitType taskWaitType) {
  if (taskNumber>=0) {
    std::set<int> tmp{taskNumber};
    waitForTasks(tmp, taskWaitType);
  }
}


void tarch::multicore::spawnAndWait(const std::vector<Task*>& tasks) {
  static tarch::logging::Log _log("tarch::multicore");

  if (not tasks.empty()) {
    static int                                nestedSpawnAndWaits = 0;
    static tarch::multicore::BooleanSemaphore nestingSemaphore;

    tarch::multicore::Lock nestingLock(nestingSemaphore, false);
    nestingLock.lock();
    nestedSpawnAndWaits++;
    const int localNestedSpawnAndWaits = nestedSpawnAndWaits;
    nestingLock.free();

    orchestrationStrategy->startBSPSection(localNestedSpawnAndWaits);

    switch (orchestrationStrategy->paralleliseForkJoinSection(localNestedSpawnAndWaits, tasks.size(), tasks[0]->getTaskType())) {
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially:
        for (auto& p : tasks) {
          p->run();
          delete p;
        }
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel:
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true);
        native::spawnAndWaitAsTaskLoop(tasks);
        ::tarch::logging::Statistics::getInstance().inc(BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true);
        break;
    }

    orchestrationStrategy->endBSPSection(localNestedSpawnAndWaits);

    nestingLock.lock();
    nestedSpawnAndWaits--;
    nestingLock.free();
  }
}


void tarch::multicore::waitForAllTasks() {
  tarch::multicore::taskfusion::processAllReadyTasks();
  native::waitForAllTasks();
}




void tarch::multicore::spawnTask(
  Task*                        task,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  assertion(task != nullptr);

  tarch::logging::Statistics::getInstance().inc(SpawnedTasksStatisticsIdentifier, 1, false, true);

  std::set<TaskNumber> prunedDependencies = prune(inDependencies);

  if (taskNumber==ExecuteImmediately) {
    waitForTasks(prunedDependencies);
    task->run();
    delete task;
  }
  else if (task->canFuse()) {
    tarch::logging::Statistics::getInstance().inc(SpawnedFusableTasksStatisticsIdentifier, 1, false, true);
    taskfusion::handleFusableTask(task, prunedDependencies, taskNumber);
  }
  else {
    native::spawnTask(task, prunedDependencies, taskNumber);
  }
}


#ifndef SharedMemoryParallelisation

void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<Task*>& tasks) {
  for (auto& p : tasks) {
    p->run();
    delete p;
  }
}


void tarch::multicore::native::spawnTask(
  Task* job,
  const std::set<TaskNumber>& /*inDependencies*/,
  const TaskNumber& /*taskNumber*/
) {
  job->run();
  delete job;
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& /*inDependencies*/) {}


void tarch::multicore::native::waitForAllTasks() {}

#endif
